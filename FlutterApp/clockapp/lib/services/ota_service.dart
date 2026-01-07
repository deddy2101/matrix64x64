import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';
import 'package:crypto/crypto.dart';
import 'device_service.dart';

/// Servizio per gestire OTA (Over-The-Air) firmware update dell'ESP32
/// Implementa protocollo con ACK per sincronizzazione chunk
class OtaService {
  final DeviceService _device;

  bool _isUpdating = false;
  int _progress = 0;
  String _status = '';

  // Per gestire ACK
  Completer<String>? _ackCompleter;
  StreamSubscription? _dataSubscription;

  OtaService(this._device);

  bool get isUpdating => _isUpdating;
  int get progress => _progress;
  String get status => _status;

  /// Aspetta una risposta specifica dal device con timeout
  Future<String?> _waitForResponse(String expectedPrefix, {Duration timeout = const Duration(seconds: 5)}) async {
    _ackCompleter = Completer<String>();

    // Sottoscrivi ai dati raw
    _dataSubscription?.cancel();
    _dataSubscription = _device.rawDataStream.listen((data) {
      print('[OTA] Received: $data');
      if (data.startsWith(expectedPrefix) || data.startsWith('OTA_') || data.startsWith('ERR,')) {
        if (_ackCompleter != null && !_ackCompleter!.isCompleted) {
          _ackCompleter!.complete(data);
        }
      }
    });

    try {
      final result = await _ackCompleter!.future.timeout(timeout);
      return result;
    } on TimeoutException {
      print('[OTA] Timeout waiting for: $expectedPrefix');
      return null;
    } finally {
      _dataSubscription?.cancel();
      _dataSubscription = null;
      _ackCompleter = null;
    }
  }

  /// Invia firmware update via WebSocket
  ///
  /// [firmwarePath] - Percorso del file .bin
  /// Returns true se l'update è completato con successo
  Future<bool> updateFirmware(String firmwarePath) async {
    if (!_device.isConnected) {
      _status = 'Dispositivo non connesso';
      return false;
    }

    if (_isUpdating) {
      _status = 'Update già in corso';
      return false;
    }

    try {
      _isUpdating = true;
      _progress = 0;
      _status = 'Lettura firmware...';

      // Leggi file
      final file = File(firmwarePath);
      if (!await file.exists()) {
        _status = 'File non trovato';
        return false;
      }

      final bytes = await file.readAsBytes();
      return await _performOtaUpdate(Uint8List.fromList(bytes), null);

    } catch (e) {
      _status = 'Errore: $e';
      print('[OTA] Error: $e');
      return false;
    } finally {
      _isUpdating = false;
      _dataSubscription?.cancel();
    }
  }

  /// Annulla update in corso
  void abortUpdate() {
    if (_isUpdating) {
      _device.send('ota,abort');
      _isUpdating = false;
      _status = 'Update annullato';
      _dataSubscription?.cancel();
      if (_ackCompleter != null && !_ackCompleter!.isCompleted) {
        _ackCompleter!.completeError('Aborted');
      }
    }
  }

  /// Invia firmware update da bytes (per download da server)
  ///
  /// [bytes] - Contenuto del firmware
  /// [expectedMd5] - MD5 atteso per verifica (opzionale)
  /// Returns true se l'update è completato con successo
  Future<bool> updateFirmwareFromBytes(Uint8List bytes, {String? expectedMd5}) async {
    if (!_device.isConnected) {
      _status = 'Dispositivo non connesso';
      return false;
    }

    if (_isUpdating) {
      _status = 'Update già in corso';
      return false;
    }

    try {
      _isUpdating = true;
      _progress = 0;
      return await _performOtaUpdate(bytes, expectedMd5);
    } catch (e) {
      _status = 'Errore: $e';
      print('[OTA] Error: $e');
      return false;
    } finally {
      _isUpdating = false;
      _dataSubscription?.cancel();
    }
  }

  /// Esegue l'update OTA con protocollo ACK
  Future<bool> _performOtaUpdate(Uint8List bytes, String? expectedMd5) async {
    final size = bytes.length;

    _status = 'Calcolo MD5...';
    final md5Hash = md5.convert(bytes).toString();

    // Verifica MD5 se fornito
    if (expectedMd5 != null && md5Hash.toLowerCase() != expectedMd5.toLowerCase()) {
      _status = 'Errore: MD5 non corrisponde';
      print('[OTA] MD5 mismatch: expected $expectedMd5, got $md5Hash');
      return false;
    }

    print('[OTA] Size: $size bytes, MD5: $md5Hash');

    // 1. Invia comando start e aspetta OTA_READY
    _status = 'Inizializzazione OTA...';
    _device.send('ota,start,$size');

    final startResponse = await _waitForResponse('OTA_', timeout: const Duration(seconds: 10));
    if (startResponse == null) {
      _status = 'Errore: timeout inizializzazione';
      return false;
    }

    if (!startResponse.startsWith('OTA_READY')) {
      _status = 'Errore: $startResponse';
      print('[OTA] Start failed: $startResponse');
      return false;
    }

    print('[OTA] Device ready, starting transfer...');

    // 2. Invia dati in chunk
    _status = 'Invio firmware...';
    const chunkSize = 3072; // 3KB chunks (~4KB base64) - ottimizzato con gestione frammentazione
    int offset = 0;
    int chunkNumber = 0;
    int retryCount = 0;
    const maxRetries = 3;

    while (offset < size) {
      final end = (offset + chunkSize < size) ? offset + chunkSize : size;
      final chunk = bytes.sublist(offset, end);

      // Converti chunk in base64
      final base64Chunk = base64.encode(chunk);

      // Invia chunk con numero sequenza
      _device.send('ota,data,$chunkNumber,$base64Chunk');

      // Aspetta ACK per questo chunk (completamente async, senza delay)
      final ackResponse = await _waitForResponse('OTA_', timeout: const Duration(seconds: 15));

      if (ackResponse == null) {
        // Timeout - riprova
        retryCount++;
        if (retryCount > maxRetries) {
          _status = 'Errore: troppi timeout';
          _device.send('ota,abort');
          return false;
        }
        print('[OTA] Timeout chunk $chunkNumber, retry $retryCount/$maxRetries');
        continue; // Riprova stesso chunk
      }

      if (ackResponse.startsWith('OTA_ACK,$chunkNumber') || ackResponse.startsWith('OTA_ACK,')) {
        // ACK ricevuto, passa al prossimo chunk
        retryCount = 0;
        offset = end;
        chunkNumber++;
        _progress = ((offset * 100) / size).round();

        // Log ogni 10%
        if (_progress % 10 == 0 || offset == size) {
          print('[OTA] Progress: $_progress% ($offset/$size bytes)');
        }
      } else if (ackResponse.startsWith('OTA_NACK') || ackResponse.startsWith('ERR,')) {
        // NACK o errore - riprova
        retryCount++;
        if (retryCount > maxRetries) {
          _status = 'Errore: $ackResponse';
          _device.send('ota,abort');
          return false;
        }
        print('[OTA] NACK chunk $chunkNumber: $ackResponse, retry $retryCount/$maxRetries');
      } else if (ackResponse.startsWith('OTA_COMPLETE')) {
        // Completato prematuramente?
        print('[OTA] Unexpected OTA_COMPLETE at chunk $chunkNumber');
        break;
      } else {
        // Risposta sconosciuta, ignora e riprova
        print('[OTA] Unknown response: $ackResponse');
      }
    }

    // 3. Finalizza con MD5
    _status = 'Verifica integrità...';
    _device.send('ota,end,$md5Hash');

    final endResponse = await _waitForResponse('OTA_', timeout: const Duration(seconds: 30));

    if (endResponse == null) {
      _status = 'Errore: timeout verifica';
      return false;
    }

    if (endResponse.startsWith('OTA_SUCCESS') || endResponse.startsWith('OTA_COMPLETE')) {
      _status = 'Update completato!';
      _progress = 100;
      print('[OTA] Update successful!');
      return true;
    } else {
      _status = 'Errore verifica: $endResponse';
      print('[OTA] Verification failed: $endResponse');
      return false;
    }
  }
}
