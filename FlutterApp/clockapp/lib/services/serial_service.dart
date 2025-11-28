import 'dart:async';
import 'dart:typed_data';
import 'package:flutter_libserialport/flutter_libserialport.dart';

class SerialDevice {
  final String name;
  final String port;
  final String? description;
  final String? manufacturer;

  SerialDevice({
    required this.name,
    required this.port,
    this.description,
    this.manufacturer,
  });
}

class SerialService {
  static final SerialService _instance = SerialService._internal();
  factory SerialService() => _instance;
  SerialService._internal();

  SerialPort? _port;
  SerialPortReader? _reader;
  
  final _connectionController = StreamController<bool>.broadcast();
  final _dataController = StreamController<String>.broadcast();
  
  Stream<bool> get connectionState => _connectionController.stream;
  Stream<String> get dataStream => _dataController.stream;
  
  bool get isConnected => _port?.isOpen ?? false;
  String? connectedDeviceName;

  String _buffer = '';

  List<SerialDevice> getDevices() {
    final ports = SerialPort.availablePorts;
    return ports.map((portName) {
      final port = SerialPort(portName);
      final device = SerialDevice(
        name: port.name ?? portName,
        port: portName,
        description: port.description,
        manufacturer: port.manufacturer,
      );
      port.dispose();
      return device;
    }).toList();
  }

  bool connect(SerialDevice device, {int baudRate = 115200}) {
    try {
      _port = SerialPort(device.port);
      
      if (!_port!.openReadWrite()) {
        print('Failed to open port: ${SerialPort.lastError}');
        return false;
      }

      _port!.config = SerialPortConfig()
        ..baudRate = baudRate
        ..bits = 8
        ..stopBits = 1
        ..parity = SerialPortParity.none
        ..setFlowControl(SerialPortFlowControl.none);

      // Start reading
      _reader = SerialPortReader(_port!);
      _reader!.stream.listen((data) {
        _buffer += String.fromCharCodes(data);
        
        // Process complete lines
        while (_buffer.contains('\n')) {
          int index = _buffer.indexOf('\n');
          String line = _buffer.substring(0, index).trim();
          _buffer = _buffer.substring(index + 1);
          
          if (line.isNotEmpty) {
            _dataController.add(line);
          }
        }
      }, onError: (error) {
        print('Serial read error: $error');
        disconnect();
      }, onDone: () {
        disconnect();
      });

      connectedDeviceName = device.description ?? device.name;
      _connectionController.add(true);
      return true;
    } catch (e) {
      print('Connection error: $e');
      return false;
    }
  }

  void disconnect() {
    _reader?.close();
    _reader = null;
    _port?.close();
    _port?.dispose();
    _port = null;
    _buffer = '';
    connectedDeviceName = null;
    _connectionController.add(false);
  }

  void send(String command) {
    if (_port?.isOpen ?? false) {
      _port!.write(Uint8List.fromList('$command\n'.codeUnits));
    }
  }

  // ═══════════════════════════════════════════
  // Comandi TimeManager
  // ═══════════════════════════════════════════
  
  void setTime(int hour, int minute, int second) {
    final h = hour.toString().padLeft(2, '0');
    final m = minute.toString().padLeft(2, '0');
    final s = second.toString().padLeft(2, '0');
    send('T$h:$m:$s');
  }

  void setDateTime(int year, int month, int day, int hour, int minute, int second) {
    final y = year.toString();
    final mo = month.toString().padLeft(2, '0');
    final d = day.toString().padLeft(2, '0');
    final h = hour.toString().padLeft(2, '0');
    final m = minute.toString().padLeft(2, '0');
    final s = second.toString().padLeft(2, '0');
    send('D$y/$mo/$d $h:$m:$s');
  }

  void syncNow() {
    final now = DateTime.now();
    setDateTime(now.year, now.month, now.day, now.hour, now.minute, now.second);
  }

  void syncFromEpoch() {
    final epoch = DateTime.now().millisecondsSinceEpoch ~/ 1000;
    send('E$epoch');
  }

  void setModeRtc() => send('Mrtc');
  void setModeFake() => send('Mfake');
  void getStatus() => send('S');
  void getHelp() => send('?');

  // ═══════════════════════════════════════════
  // Comandi EffectManager
  // ═══════════════════════════════════════════
  
  void pause() => send('p');
  void resume() => send('r');
  void nextEffect() => send('n');
  void selectEffect(int index) => send('$index');

  void dispose() {
    disconnect();
    _connectionController.close();
    _dataController.close();
  }
}