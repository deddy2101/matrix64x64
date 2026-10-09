import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';
import 'package:universal_ble/universal_ble.dart';
import 'ble_linux_agent.dart';

/// Dispositivo BLE trovato durante la scansione
class BleScanResult {
  final String id;
  final String name;
  final int? rssi;

  const BleScanResult({required this.id, required this.name, this.rssi});
}

/// Trasporto BLE verso il firmware: nasconde i dettagli GATT a DeviceService.
///
/// Protocollo (vedi docs/BLE_MIGRATION_PLAN.md): righe di testo terminate da
/// '\n', scambiate su due caratteristiche (RX = app→ESP, TX = ESP→app).
/// Una riga può arrivare spezzata su più notify e viene riassemblata qui.
class BleLink {
  static const String serviceUuid = '6f1a0001-8b5c-4d2e-9a3f-1c2d3e4f5a6b';
  static const String rxUuid = '6f1a0002-8b5c-4d2e-9a3f-1c2d3e4f5a6b'; // app → ESP
  static const String txUuid = '6f1a0003-8b5c-4d2e-9a3f-1c2d3e4f5a6b'; // ESP → app
  static const String unlockUuid = '6f1a0004-8b5c-4d2e-9a3f-1c2d3e4f5a6b'; // PIN statico

  /// Il pairing richiede che l'utente digiti il PIN mostrato sulla matrice:
  /// tempo largo per le operazioni che restano in attesa del pairing.
  static const Duration _pairingTimeout = Duration(seconds: 90);

  /// MTU richiesto (il firmware lo accetta fino a 247)
  static const int _requestedMtu = 247;

  /// Se true, le righe più lunghe di un pacchetto usano write con risposta
  /// (più lento ma garantito). Da attivare solo se i test mostrano perdite
  /// di pacchetti negli invii lunghi (OTA, upload immagini).
  static const bool bulkWithResponse = false;

  String? _deviceId;
  int _mtu = 23;
  bool _ready = false;
  String? _lastError;

  final _linesController = StreamController<String>.broadcast();
  final _connectionController = StreamController<bool>.broadcast();
  final _scanController = StreamController<BleScanResult>.broadcast();

  StreamSubscription? _valueSub;
  StreamSubscription? _unlockSub;
  StreamSubscription? _connSub;

  // Sblocco con PIN statico: messaggi dell'ESP sulla caratteristica UNLOCK
  // (KNOWN, LOCKED, OK, ERR,wrong,N, ERR,locked,S) e arrivo del WELCOME
  final List<String> _unlockInbox = [];
  bool _welcomeSeen = false;
  bool _linkUp = false;
  bool _retryable = false; // ultimo fallimento: vale la pena riprovare da soli
  StreamSubscription? _scanSub;

  // Riassemblaggio righe (byte, così i caratteri UTF-8 spezzati tra due
  // notify non si rovinano)
  final List<int> _rxBuffer = [];

  // Invii serializzati: due send() concorrenti non devono mescolare i byte
  Future<void> _sendChain = Future.value();
  final Map<String, String> _pendingCoalesced = {};

  /// Righe ricevute dal dispositivo
  Stream<String> get lines => _linesController.stream;

  /// true quando il collegamento è pronto, false quando cade
  Stream<bool> get connectionState => _connectionController.stream;

  /// Dispositivi trovati durante la scansione
  Stream<BleScanResult> get scanResults => _scanController.stream;

  /// Collegato, associato e pronto a scambiare comandi
  bool get isReady => _ready;
  String? get deviceId => _deviceId;

  /// Motivo dell'ultimo fallimento di connect()/startScan(), per la UI
  String? get lastError => _lastError;

  // ═══════════════════════════════════════════
  // Disponibilità e scansione
  // ═══════════════════════════════════════════

  /// Stato del Bluetooth del telefono
  Future<AvailabilityState> getAvailability() =>
      UniversalBle.getBluetoothAvailabilityState();

  Stream<AvailabilityState> get availabilityStream =>
      UniversalBle.availabilityStream;

  /// Avvia la scansione dei display (filtrata sull'UUID del servizio).
  /// Ritorna false se mancano permessi o il Bluetooth è spento.
  Future<bool> startScan() async {
    _lastError = null;
    try {
      await UniversalBle.requestPermissions();

      final state = await UniversalBle.getBluetoothAvailabilityState();
      if (state != AvailabilityState.poweredOn) {
        _lastError = state == AvailabilityState.unauthorized
            ? 'Permesso Bluetooth negato'
            : state == AvailabilityState.unsupported
                ? 'Bluetooth non supportato su questo dispositivo'
                : 'Bluetooth spento';
        return false;
      }

      _scanSub?.cancel();
      _scanSub = UniversalBle.scanStream.listen((device) {
        // Alcune piattaforme ignorano il filtro: ricontrolla l'UUID
        final hasService = device.services
            .any((s) => s.toLowerCase() == serviceUuid.toLowerCase());
        if (!hasService) return;
        _scanController.add(BleScanResult(
          id: device.deviceId,
          name: device.name ?? device.rawName ?? 'LED Matrix',
          rssi: device.rssi,
        ));
      });

      await UniversalBle.startScan(
        scanFilter: ScanFilter(withServices: [serviceUuid]),
      );
      return true;
    } catch (e) {
      _lastError = 'Errore scansione Bluetooth: $e';
      print('[BLE] startScan error: $e');
      return false;
    }
  }

  Future<void> stopScan() async {
    _scanSub?.cancel();
    _scanSub = null;
    try {
      await UniversalBle.stopScan();
    } catch (e) {
      print('[BLE] stopScan error: $e');
    }
  }

  // ═══════════════════════════════════════════
  // Connessione
  // ═══════════════════════════════════════════

  /// Collega il dispositivo.
  ///
  /// Accesso: un telefono già associato entra da solo. Uno nuovo deve dare il
  /// PIN STATICO (chiesto tramite [providePin]; ritorna null per annullare;
  /// [retry] = true dopo un PIN sbagliato) e poi digitare nel sistema il PIN
  /// DINAMICO mostrato dal display: per questo l'attesa può durare fino a 90 s.
  /// Il WELCOME del firmware (inviato solo a collegamento cifrato e
  /// autenticato) segna la fine dell'accesso.
  ///
  /// [providePasskey] serve solo su Linux, dove il sistema non chiede da sé il
  /// PIN dinamico (vedi [BleLinuxAgent]); ritorna null per annullare.
  ///
  /// Se il display chiude la connessione prima che l'utente abbia digitato
  /// qualcosa (tipicamente un'associazione rimasta a metà, che il firmware
  /// cancella da solo) riprova una volta in automatico.
  Future<bool> connect(
    String deviceId, {
    Future<String?> Function(bool retry)? providePin,
    Future<String?> Function()? providePasskey,
  }) async {
    if (Platform.isLinux) {
      await BleLinuxAgent.instance.ensureRegistered();
      BleLinuxAgent.instance.onPasskey = providePasskey;
    }
    try {
      if (await _connectOnce(deviceId, providePin)) return true;
      if (!_retryable) return false;
      print('[BLE] Link dropped before access, retrying once');
      await Future.delayed(const Duration(seconds: 1));
      return await _connectOnce(deviceId, providePin);
    } finally {
      if (Platform.isLinux) BleLinuxAgent.instance.onPasskey = null;
    }
  }

  Future<bool> _connectOnce(
    String deviceId,
    Future<String?> Function(bool retry)? providePin,
  ) async {
    await disconnect();
    _lastError = null;
    _retryable = false;
    _deviceId = deviceId;
    _rxBuffer.clear();
    _unlockInbox.clear();
    _welcomeSeen = false;

    try {
      await stopScan();

      _connSub = UniversalBle.connectionStream(deviceId).listen((connected) {
        _linkUp = connected;
        if (!connected) _onDisconnected();
      });

      await UniversalBle.connect(deviceId, timeout: const Duration(seconds: 20));
      _linkUp = true;

      _valueSub = UniversalBle.characteristicValueStream(deviceId, txUuid)
          .listen(_onBytes);
      _unlockSub = UniversalBle.characteristicValueStream(deviceId, unlockUuid)
          .listen((bytes) => _unlockInbox
              .add(utf8.decode(bytes, allowMalformed: true).trim()));

      await UniversalBle.discoverServices(deviceId, timeout: _pairingTimeout);

      try {
        _mtu = await UniversalBle.requestMtu(deviceId, _requestedMtu);
      } catch (e) {
        print('[BLE] requestMtu failed (using default): $e');
      }

      await UniversalBle.subscribeNotifications(
        deviceId,
        serviceUuid,
        txUuid,
        timeout: _pairingTimeout,
      );
      // Iscrivendosi a UNLOCK l'ESP risponde KNOWN o LOCKED
      await UniversalBle.subscribeNotifications(
        deviceId,
        serviceUuid,
        unlockUuid,
        timeout: _pairingTimeout,
      );

      String? msg = await _waitWelcomeOrUnlock(const Duration(seconds: 15));
      String? prev; // ultimo messaggio ricevuto, per spiegare un fallimento

      for (int step = 0; step < 10; step++) {
        if (msg == null) {
          _lastError = _failureReason(prev);
          break;
        }
        prev = msg;

        if (msg == 'WELCOME') {
          _ready = true;
          _connectionController.add(true);
          print('[BLE] Ready (mtu $_mtu)');
          return true;
        }

        if (msg == 'KNOWN') {
          // Già associato: cifratura con le chiavi salvate. Se il telefono ha
          // dimenticato l'associazione arriverà LOCKED.
          msg = await _waitWelcomeOrUnlock(const Duration(seconds: 25));
        } else if (msg == 'OK') {
          // PIN statico accettato: ora l'utente digita il PIN dinamico
          msg = await _waitWelcomeOrUnlock(_pairingTimeout);
        } else if (msg == 'LOCKED' || msg.startsWith('ERR,wrong')) {
          final pin = providePin == null
              ? null
              : await providePin(msg.startsWith('ERR,wrong'));
          if (pin == null) {
            _lastError = 'PIN di accesso richiesto';
            break;
          }
          await UniversalBle.write(
            deviceId,
            serviceUuid,
            unlockUuid,
            Uint8List.fromList(utf8.encode(pin)),
          );
          msg = await _waitWelcomeOrUnlock(const Duration(seconds: 15));
        } else if (msg.startsWith('ERR,locked')) {
          final seconds = int.tryParse(msg.split(',').last) ?? 300;
          _lastError = 'Troppi tentativi errati. Riprova tra '
              '${(seconds / 60).ceil()} minuti.';
          break;
        } else {
          msg = await _waitWelcomeOrUnlock(const Duration(seconds: 15));
        }
      }

      _lastError ??= 'Accesso non completato: controlla i PIN';
    } on TimeoutException {
      _lastError = 'Connessione Bluetooth scaduta';
    } catch (e) {
      _lastError = 'Connessione Bluetooth fallita: $e';
    }

    print('[BLE] connect failed: $_lastError');
    await disconnect();
    return false;
  }

  /// Spiega perché l'accesso si è fermato. [prev] = ultimo messaggio UNLOCK
  /// ricevuto prima del timeout o della caduta del collegamento.
  String _failureReason(String? prev) {
    if (_linkUp) {
      return prev == 'OK'
          ? 'PIN del display non inserito in tempo. Riprova.'
          : 'Il display non risponde. Riprova avvicinandoti.';
    }
    // Collegamento chiuso durante l'accesso
    if (prev == 'OK') {
      return 'PIN del display errato o inserito troppo tardi. Riprova.';
    }
    if (prev != null && prev != 'KNOWN') {
      return 'Il display ha chiuso la connessione. Riprova.';
    }
    // Prima che l'utente digitasse qualcosa: quasi sempre un'associazione
    // vecchia o rimasta a metà (il firmware la cancella da solo)
    _retryable = true;
    return 'Il display ha rifiutato l\'associazione. Riprova; se continua, '
        'rimuovi "ledmatrix" dai dispositivi Bluetooth del telefono.';
  }

  /// Aspetta il WELCOME o il prossimo messaggio UNLOCK. null = timeout o
  /// collegamento caduto.
  Future<String?> _waitWelcomeOrUnlock(Duration timeout) async {
    final end = DateTime.now().add(timeout);
    while (DateTime.now().isBefore(end)) {
      if (_welcomeSeen) return 'WELCOME';
      if (_unlockInbox.isNotEmpty) return _unlockInbox.removeAt(0);
      if (!_linkUp) return null;
      await Future.delayed(const Duration(milliseconds: 100));
    }
    return null;
  }

  Future<void> disconnect() async {
    final id = _deviceId;
    _valueSub?.cancel();
    _valueSub = null;
    _unlockSub?.cancel();
    _unlockSub = null;
    _linkUp = false;
    _connSub?.cancel();
    _connSub = null;
    final wasReady = _ready;
    _ready = false;
    _deviceId = null;
    _pendingCoalesced.clear();
    _rxBuffer.clear();

    if (id != null) {
      try {
        await UniversalBle.disconnect(id);
      } catch (e) {
        print('[BLE] disconnect error: $e');
      }
    }
    if (wasReady) _connectionController.add(false);
  }

  void _onDisconnected() {
    print('[BLE] Disconnected');
    final wasReady = _ready;
    _ready = false;
    if (wasReady) _connectionController.add(false);
  }

  // ═══════════════════════════════════════════
  // Ricezione
  // ═══════════════════════════════════════════

  void _onBytes(Uint8List data) {
    for (final b in data) {
      if (b == 0x0A) {
        if (_rxBuffer.isNotEmpty) {
          final line =
              utf8.decode(_rxBuffer, allowMalformed: true).trim();
          _rxBuffer.clear();
          if (line.startsWith('WELCOME')) _welcomeSeen = true;
          if (line.isNotEmpty) _linesController.add(line);
        }
      } else {
        _rxBuffer.add(b);
      }
    }
  }

  // ═══════════════════════════════════════════
  // Invio
  // ═══════════════════════════════════════════

  /// Invia una riga. Gli invii sono serializzati.
  ///
  /// [coalesceKey]: per comandi di movimento a raffica (es. posizione della
  /// racchetta) tiene in coda solo l'ultimo valore invece di accumularli.
  Future<void> sendLine(String line, {String? coalesceKey}) {
    if (coalesceKey != null) {
      if (_pendingCoalesced.containsKey(coalesceKey)) {
        _pendingCoalesced[coalesceKey] = line; // sostituisce quello in attesa
        return Future.value();
      }
      _pendingCoalesced[coalesceKey] = line;
    }

    final f = _sendChain.then((_) {
      final toSend =
          coalesceKey != null ? _pendingCoalesced.remove(coalesceKey)! : line;
      return _writeLine(toSend);
    });
    _sendChain = f.catchError((_) {});
    return f;
  }

  Future<void> _writeLine(String line) async {
    final id = _deviceId;
    if (id == null || !_ready) return;

    final bytes = Uint8List.fromList(utf8.encode('$line\n'));
    final payload = (_mtu - 3).clamp(20, 244);
    final multiPacket = bytes.length > payload;
    final withoutResponse = !(bulkWithResponse && multiPacket);

    for (int offset = 0; offset < bytes.length; offset += payload) {
      final end =
          (offset + payload < bytes.length) ? offset + payload : bytes.length;
      await UniversalBle.write(
        id,
        serviceUuid,
        rxUuid,
        Uint8List.sublistView(bytes, offset, end),
        withoutResponse: withoutResponse,
      );
    }
  }

  void dispose() {
    disconnect();
    _scanSub?.cancel();
    _linesController.close();
    _connectionController.close();
    _scanController.close();
  }
}
