import 'dart:async';
import 'dart:io';

/// Dispositivo trovato via discovery
class DiscoveredDevice {
  final String name;
  final String ip;
  final int port;
  final DateTime discoveredAt;

  DiscoveredDevice({
    required this.name,
    required this.ip,
    required this.port,
    DateTime? discoveredAt,
  }) : discoveredAt = discoveredAt ?? DateTime.now();

  @override
  String toString() => '$name ($ip:$port)';

  @override
  bool operator ==(Object other) =>
      other is DiscoveredDevice && other.ip == ip && other.port == port;

  @override
  int get hashCode => ip.hashCode ^ port.hashCode;
}

/// Informazioni su interfaccia di rete (uso interno)
class _NetworkInterface {
  final String name;
  final String ip;
  final InternetAddress address;

  _NetworkInterface({
    required this.name,
    required this.ip,
    required this.address,
  });

  @override
  String toString() => '$name: $ip';
}

/// Servizio per discovery automatico dispositivi LED Matrix
/// Supporta multiple interfacce di rete (WiFi + Ethernet)
class DiscoveryService {
  static const int discoveryPort = 5555;
  static const String magicString = 'LEDMATRIX_DISCOVER';
  static const String responsePrefix = 'LEDMATRIX_HERE';

  final _devicesController =
      StreamController<List<DiscoveredDevice>>.broadcast();
  final Map<String, DiscoveredDevice> _devices = {};
  bool _isScanning = false;

  /// Stream dei dispositivi trovati
  Stream<List<DiscoveredDevice>> get devicesStream => _devicesController.stream;

  /// Lista corrente dei dispositivi
  List<DiscoveredDevice> get devices => _devices.values.toList();

  /// Sta scansionando?
  bool get isScanning => _isScanning;

  /// Ottiene tutte le interfacce di rete disponibili
  Future<List<_NetworkInterface>> _getNetworkInterfaces() async {
    final interfaces = <_NetworkInterface>[];

    try {
      final networkInterfaces = await NetworkInterface.list(
        includeLinkLocal: false,
        type: InternetAddressType.IPv4,
      );

      for (var interface in networkInterfaces) {
        for (var addr in interface.addresses) {
          // Salta loopback
          if (addr.address == '127.0.0.1') continue;

          interfaces.add(
            _NetworkInterface(
              name: interface.name,
              ip: addr.address,
              address: addr,
            ),
          );
        }
      }
    } catch (e) {
      // Silenzioso - se fallisce useremo broadcast globale
    }

    return interfaces;
  }

  /// Testa una singola interfaccia di rete
  Future<int> _scanInterface(
    _NetworkInterface interface, {
    required Duration timeout,
    required Duration interval,
  }) async {
    RawDatagramSocket? socket;
    Timer? scanTimer;
    int devicesFound = 0;

    try {
      // Bind sull'IP specifico dell'interfaccia
      socket = await RawDatagramSocket.bind(interface.address, 0);
      socket.broadcastEnabled = true;

      // Ascolta risposte
      final subscription = socket.listen((event) {
        if (event == RawSocketEvent.read) {
          final datagram = socket!.receive();
          if (datagram != null) {
            final found = _handleResponse(datagram);
            if (found) devicesFound++;
          }
        }
      });

      // Calcola broadcast address (assumiamo /24)
      final ipParts = interface.ip.split('.');
      final broadcastIp = '${ipParts[0]}.${ipParts[1]}.${ipParts[2]}.255';
      final broadcastAddr = InternetAddress(broadcastIp);

      final message = magicString.codeUnits;
      final maxSends = (timeout.inMilliseconds / interval.inMilliseconds)
          .ceil();
      int sendCount = 0;

      // Invia broadcast periodicamente
      scanTimer = Timer.periodic(interval, (timer) {
        if (sendCount >= maxSends) {
          timer.cancel();
          return;
        }

        try {
          socket?.send(message, broadcastAddr, discoveryPort);
          sendCount++;
        } catch (e) {
          // Silenzioso
        }
      });

      // Aspetta timeout
      await Future.delayed(timeout);

      // Cleanup
      scanTimer.cancel();
      await subscription.cancel();
      socket.close();
    } catch (e) {
      scanTimer?.cancel();
      socket?.close();
    }

    return devicesFound;
  }

  /// Avvia scansione su tutte le interfacce
  /// Prova prima multi-interface, poi fallback su broadcast globale
  Future<List<DiscoveredDevice>> scan({
    Duration timeout = const Duration(seconds: 2),
    Duration interval = const Duration(milliseconds: 500),
  }) async {
    if (_isScanning) {
      return devices;
    }

    _isScanning = true;
    _devices.clear();
    _devicesController.add([]);

    try {
      // Ottieni tutte le interfacce
      final interfaces = await _getNetworkInterfaces();

      if (interfaces.isEmpty) {
        // Fallback: broadcast globale
        return await _scanGlobalBroadcast(timeout: timeout, interval: interval);
      }

      // Scansiona ogni interfaccia sequenzialmente
      for (var interface in interfaces) {
        await _scanInterface(interface, timeout: timeout, interval: interval);

        // Piccola pausa tra interfacce
        await Future.delayed(const Duration(milliseconds: 200));
      }

      // Se non ha trovato nulla, prova broadcast globale come fallback
      if (_devices.isEmpty) {
        await _scanGlobalBroadcast(timeout: timeout, interval: interval);
      }
    } catch (e) {
      // Se tutto fallisce, prova broadcast globale
      await _scanGlobalBroadcast(timeout: timeout, interval: interval);
    } finally {
      _isScanning = false;
    }

    return devices;
  }

  /// Scansione su broadcast globale (fallback per mobile o se multi-interface fallisce)
  Future<List<DiscoveredDevice>> _scanGlobalBroadcast({
    Duration timeout = const Duration(seconds: 3),
    Duration interval = const Duration(milliseconds: 500),
  }) async {
    RawDatagramSocket? socket;
    Timer? scanTimer;
    Timer? timeoutTimer;

    try {
      socket = await RawDatagramSocket.bind(InternetAddress.anyIPv4, 0);
      socket.broadcastEnabled = true;

      // Ascolta risposte
      socket.listen((event) {
        if (event == RawSocketEvent.read) {
          final datagram = socket!.receive();
          if (datagram != null) {
            _handleResponse(datagram);
          }
        }
      });

      final broadcastAddr = InternetAddress('255.255.255.255');
      final message = magicString.codeUnits;
      final maxSends = (timeout.inMilliseconds / interval.inMilliseconds)
          .ceil();
      int sendCount = 0;

      // Invia broadcast periodicamente
      scanTimer = Timer.periodic(interval, (timer) {
        if (sendCount >= maxSends) {
          timer.cancel();
          return;
        }

        try {
          socket?.send(message, broadcastAddr, discoveryPort);
          sendCount++;
        } catch (e) {
          // Silenzioso
        }
      });

      // Timer di timeout
      timeoutTimer = Timer(timeout, () {
        scanTimer?.cancel();
        socket?.close();
      });

      await Future.delayed(timeout + const Duration(milliseconds: 500));
    } catch (e) {
      // Silenzioso
    } finally {
      scanTimer?.cancel();
      timeoutTimer?.cancel();
      socket?.close();
    }

    return devices;
  }

  /// Gestisce risposta UDP
  bool _handleResponse(Datagram datagram) {
    try {
      final response = String.fromCharCodes(datagram.data).trim();
      final senderIp = datagram.address.address;

      // Formato: LEDMATRIX_HERE,nome,ip,porta
      if (response.startsWith(responsePrefix)) {
        final parts = response.split(',');

        if (parts.length >= 4) {
          var deviceName = parts[1].trim();
          var deviceIp = parts[2].trim();
          final devicePort = int.tryParse(parts[3].trim()) ?? 80;

          // Fix: Se IP è 0.0.0.0, usa IP mittente
          if (deviceIp == '0.0.0.0') {
            deviceIp = senderIp;
          }

          final device = DiscoveredDevice(
            name: deviceName,
            ip: deviceIp,
            port: devicePort,
          );

          // Aggiungi/aggiorna dispositivo
          final key = '${device.ip}:${device.port}';

          if (!_devices.containsKey(key)) {
            _devices[key] = device;
            _devicesController.add(devices);
            return true;
          }
        }
      }
    } catch (e) {
      // Silenzioso
    }

    return false;
  }

  /// Scansione rapida (1.5 secondi per interfaccia)
  Future<List<DiscoveredDevice>> quickScan() {
    return scan(
      timeout: const Duration(milliseconds: 1500),
      interval: const Duration(milliseconds: 400),
    );
  }

  /// Pulisci dispositivi vecchi
  void cleanOldDevices({Duration maxAge = const Duration(seconds: 30)}) {
    final now = DateTime.now();
    _devices.removeWhere(
      (key, device) => now.difference(device.discoveredAt) > maxAge,
    );
    _devicesController.add(devices);
  }

  /// Pulisci e chiudi tutto
  void dispose() {
    _devices.clear();
    _devicesController.close();
    _isScanning = false;
  }
}
