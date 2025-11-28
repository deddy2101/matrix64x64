// Serial implementation - cross platform
// Su desktop usa Process per comunicare via /dev/ttyUSB
// Su mobile è disabilitato

import 'dart:async';
import 'dart:io';
import 'dart:typed_data';
import 'device_service.dart';

/// Verifica se siamo su desktop
bool get _isDesktop {
  try {
    return Platform.isLinux || Platform.isMacOS || Platform.isWindows;
  } catch (_) {
    return false;
  }
}

/// Lista dispositivi seriali disponibili (solo Linux per ora)
List<SerialDevice> getAvailableDevices() {
  if (!_isDesktop) return [];

  try {
    if (Platform.isLinux) {
      return _getLinuxDevices();
    } else if (Platform.isMacOS) {
      return _getMacDevices();
    } else if (Platform.isWindows) {
      return _getWindowsDevices();
    }
  } catch (e) {
    print('Error getting serial devices: $e');
  }
  return [];
}

List<SerialDevice> _getLinuxDevices() {
  final devices = <SerialDevice>[];

  try {
    // Cerca in /dev per dispositivi seriali
    final devDir = Directory('/dev');
    if (devDir.existsSync()) {
      for (final entity in devDir.listSync()) {
        final name = entity.path.split('/').last;
        if (name.startsWith('ttyUSB') ||
            name.startsWith('ttyACM') ||
            name.startsWith('ttyS')) {
          devices.add(
            SerialDevice(
              name: name,
              port: entity.path,
              description: _getDeviceDescription(entity.path),
            ),
          );
        }
      }
    }
  } catch (e) {
    print('Error scanning Linux devices: $e');
  }

  return devices;
}

List<SerialDevice> _getMacDevices() {
  final devices = <SerialDevice>[];

  try {
    final devDir = Directory('/dev');
    if (devDir.existsSync()) {
      for (final entity in devDir.listSync()) {
        final name = entity.path.split('/').last;
        if (name.startsWith('cu.usb') || name.startsWith('tty.usb')) {
          devices.add(
            SerialDevice(name: name, port: entity.path, description: name),
          );
        }
      }
    }
  } catch (e) {
    print('Error scanning Mac devices: $e');
  }

  return devices;
}

List<SerialDevice> _getWindowsDevices() {
  // Su Windows è più complesso, richiederebbe WMI o registro
  // Per ora ritorna lista vuota - usa WebSocket su Windows
  return [];
}

String? _getDeviceDescription(String path) {
  try {
    // Prova a leggere info dal sysfs su Linux
    final deviceName = path.split('/').last;
    final sysPath = '/sys/class/tty/$deviceName/device/interface';
    final file = File(sysPath);
    if (file.existsSync()) {
      return file.readAsStringSync().trim();
    }

    // Prova manufacturer
    final mfgPath = '/sys/class/tty/$deviceName/device/../manufacturer';
    final mfgFile = File(mfgPath);
    if (mfgFile.existsSync()) {
      return mfgFile.readAsStringSync().trim();
    }
  } catch (_) {}
  return null;
}

/// Classe per gestire connessione seriale tramite Process (stty + cat/echo)
class _SerialConnection {
  final String port;
  final int baudRate;
  Process? _readProcess;
  IOSink? _writeSink;
  StreamSubscription? _subscription;
  final void Function(Uint8List)? onData;
  final void Function(dynamic)? onError;
  final void Function()? onDone;

  _SerialConnection({
    required this.port,
    required this.baudRate,
    this.onData,
    this.onError,
    this.onDone,
  });

  Future<bool> open() async {
    try {
      // Configura porta seriale con stty
      final sttyResult = await Process.run('stty', [
        '-F',
        port,
        '$baudRate',
        'raw',
        '-echo',
        '-echoe',
        '-echok',
      ]);

      if (sttyResult.exitCode != 0) {
        print('stty error: ${sttyResult.stderr}');
        return false;
      }

      // Avvia processo di lettura
      _readProcess = await Process.start('cat', [port]);

      _subscription = _readProcess!.stdout.listen(
        (data) => onData?.call(Uint8List.fromList(data)),
        onError: (e) {
          onError?.call(e);
          close();
        },
        onDone: () {
          onDone?.call();
          close();
        },
      );

      // Apri file per scrittura
      final file = File(port);
      _writeSink = file.openWrite(mode: FileMode.writeOnlyAppend);

      return true;
    } catch (e) {
      print('Error opening serial: $e');
      onError?.call(e);
      return false;
    }
  }

  void write(String data) {
    try {
      _writeSink?.write(data);
    } catch (e) {
      print('Error writing to serial: $e');
    }
  }

  void close() {
    _subscription?.cancel();
    _readProcess?.kill();
    _writeSink?.close();
    _readProcess = null;
    _writeSink = null;
  }
}

// Mappa delle connessioni attive
final Map<String, _SerialConnection> _connections = {};

/// Connetti a porta seriale
Future<Map<String, dynamic>?> connect(
  String portName, {
  int baudRate = 115200,
  void Function(Uint8List)? onData,
  void Function(dynamic)? onError,
  void Function()? onDone,
}) async {
  if (!_isDesktop) return null;

  try {
    final conn = _SerialConnection(
      port: portName,
      baudRate: baudRate,
      onData: onData,
      onError: onError,
      onDone: onDone,
    );

    final success = await conn.open();
    if (!success) return null;

    _connections[portName] = conn;

    return {'port': portName, 'connection': conn};
  } catch (e) {
    print('Serial connection error: $e');
    return null;
  }
}

/// Chiudi connessione
void closePort(dynamic port, dynamic reader) {
  if (port is String) {
    _connections[port]?.close();
    _connections.remove(port);
  }
}

/// Scrivi su porta seriale
void write(dynamic port, String data) {
  if (port is String) {
    _connections[port]?.write(data);
  }
}
