import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';
import 'package:flutter_libserialport/flutter_libserialport.dart';
import 'package:web_socket_channel/web_socket_channel.dart';

/// Tipo di connessione
enum ConnectionType { none, serial, websocket }

/// Stato connessione
enum DeviceConnectionState { disconnected, connecting, connected }

/// Dispositivo seriale
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

  String get displayName => description ?? name;
}

/// Stato del dispositivo LED Matrix
class DeviceStatus {
  final String time;
  final String date;
  final String timeMode;
  final bool ds3231Available;
  final double? temperature;
  final String effect;
  final int effectIndex;
  final double fps;
  final bool autoSwitch;
  final int effectCount;
  final int brightness;
  final bool isNight;
  final String wifiStatus;
  final String ip;
  final String ssid;
  final int? rssi;
  final int uptime;
  final int freeHeap;

  DeviceStatus({
    this.time = '--:--:--',
    this.date = '----/--/--',
    this.timeMode = 'unknown',
    this.ds3231Available = false,
    this.temperature,
    this.effect = 'unknown',
    this.effectIndex = -1,
    this.fps = 0,
    this.autoSwitch = false,
    this.effectCount = 0,
    this.brightness = 0,
    this.isNight = false,
    this.wifiStatus = 'unknown',
    this.ip = '0.0.0.0',
    this.ssid = '',
    this.rssi,
    this.uptime = 0,
    this.freeHeap = 0,
  });

  factory DeviceStatus.fromJson(Map<String, dynamic> json) {
    return DeviceStatus(
      time: json['time'] ?? '--:--:--',
      date: json['date'] ?? '----/--/--',
      timeMode: json['timeMode'] ?? 'unknown',
      ds3231Available: json['ds3231'] ?? false,
      temperature: json['temperature']?.toDouble(),
      effect: json['effect'] ?? 'unknown',
      effectIndex: json['effectIndex'] ?? -1,
      fps: (json['fps'] ?? 0).toDouble(),
      autoSwitch: json['autoSwitch'] ?? false,
      effectCount: json['effectCount'] ?? 0,
      brightness: json['brightness'] ?? 0,
      isNight: json['isNight'] ?? false,
      wifiStatus: json['wifiStatus'] ?? 'unknown',
      ip: json['ip'] ?? '0.0.0.0',
      ssid: json['ssid'] ?? '',
      rssi: json['rssi'],
      uptime: json['uptime'] ?? 0,
      freeHeap: json['freeHeap'] ?? 0,
    );
  }
}

/// Informazioni effetto
class EffectInfo {
  final int index;
  final String name;
  final bool isCurrent;

  EffectInfo({
    required this.index,
    required this.name,
    required this.isCurrent,
  });

  factory EffectInfo.fromJson(Map<String, dynamic> json) {
    return EffectInfo(
      index: json['index'] ?? 0,
      name: json['name'] ?? 'unknown',
      isCurrent: json['current'] ?? false,
    );
  }
}

/// Impostazioni dispositivo
class DeviceSettings {
  final String ssid;
  final bool apMode;
  final int brightnessDay;
  final int brightnessNight;
  final int nightStartHour;
  final int nightEndHour;
  final int effectDuration;
  final bool autoSwitch;
  final int currentEffect;
  final String deviceName;

  DeviceSettings({
    this.ssid = '',
    this.apMode = true,
    this.brightnessDay = 200,
    this.brightnessNight = 30,
    this.nightStartHour = 22,
    this.nightEndHour = 7,
    this.effectDuration = 10000,
    this.autoSwitch = true,
    this.currentEffect = -1,
    this.deviceName = 'ledmatrix',
  });

  factory DeviceSettings.fromJson(Map<String, dynamic> json) {
    final data = json['data'] ?? json;
    final wifi = data['wifi'] ?? {};
    final display = data['display'] ?? {};
    final effects = data['effects'] ?? {};
    final device = data['device'] ?? {};

    return DeviceSettings(
      ssid: wifi['ssid'] ?? '',
      apMode: wifi['apMode'] ?? true,
      brightnessDay: display['brightnessDay'] ?? 200,
      brightnessNight: display['brightnessNight'] ?? 30,
      nightStartHour: display['nightStartHour'] ?? 22,
      nightEndHour: display['nightEndHour'] ?? 7,
      effectDuration: effects['duration'] ?? 10000,
      autoSwitch: effects['autoSwitch'] ?? true,
      currentEffect: effects['currentEffect'] ?? -1,
      deviceName: device['name'] ?? 'ledmatrix',
    );
  }
}

/// Servizio unificato per comunicazione con LED Matrix
/// Supporta sia connessione Seriale che WebSocket
class DeviceService {
  static final DeviceService _instance = DeviceService._internal();
  factory DeviceService() => _instance;
  DeviceService._internal();

  // Connessione
  ConnectionType _connectionType = ConnectionType.none;
  DeviceConnectionState _state = DeviceConnectionState.disconnected;
  String? _connectedName;

  // Serial
  SerialPort? _serialPort;
  SerialPortReader? _serialReader;
  String _serialBuffer = '';

  // WebSocket
  WebSocketChannel? _wsChannel;
  StreamSubscription? _wsSubscription;
  Timer? _reconnectTimer;
  Timer? _pingTimer;
  String? _wsHost;

  // Controllers
  final _connectionController =
      StreamController<DeviceConnectionState>.broadcast();
  final _statusController = StreamController<DeviceStatus>.broadcast();
  final _effectsController = StreamController<List<EffectInfo>>.broadcast();
  final _settingsController = StreamController<DeviceSettings>.broadcast();
  final _dataController = StreamController<String>.broadcast();

  // Cache
  DeviceStatus? _lastStatus;
  List<EffectInfo>? _lastEffects;
  DeviceSettings? _lastSettings;

  // Streams
  Stream<DeviceConnectionState> get connectionState =>
      _connectionController.stream;
  Stream<DeviceStatus> get statusStream => _statusController.stream;
  Stream<List<EffectInfo>> get effectsStream => _effectsController.stream;
  Stream<DeviceSettings> get settingsStream => _settingsController.stream;
  Stream<String> get rawDataStream => _dataController.stream;

  // Getters
  DeviceConnectionState get state => _state;
  ConnectionType get connectionType => _connectionType;
  bool get isConnected => _state == DeviceConnectionState.connected;
  String? get connectedName => _connectedName;
  DeviceStatus? get lastStatus => _lastStatus;
  List<EffectInfo>? get lastEffects => _lastEffects;
  DeviceSettings? get lastSettings => _lastSettings;

  // ═══════════════════════════════════════════
  // Connessione
  // ═══════════════════════════════════════════

  /// Lista dispositivi seriali disponibili
  List<SerialDevice> getSerialDevices() {
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

  /// Connetti via seriale
  Future<bool> connectSerial(
    SerialDevice device, {
    int baudRate = 115200,
  }) async {
    disconnect();

    _setState(DeviceConnectionState.connecting);
    _connectionType = ConnectionType.serial;

    try {
      _serialPort = SerialPort(device.port);

      if (!_serialPort!.openReadWrite()) {
        print('Failed to open port: ${SerialPort.lastError}');
        _setState(DeviceConnectionState.disconnected);
        return false;
      }

      _serialPort!.config = SerialPortConfig()
        ..baudRate = baudRate
        ..bits = 8
        ..stopBits = 1
        ..parity = SerialPortParity.none
        ..setFlowControl(SerialPortFlowControl.none);

      _serialReader = SerialPortReader(_serialPort!);
      _serialReader!.stream.listen(
        _onSerialData,
        onError: (e) {
          print('Serial error: $e');
          disconnect();
        },
        onDone: disconnect,
      );

      _connectedName = device.displayName;
      _setState(DeviceConnectionState.connected);

      // Richiedi stato
      Future.delayed(const Duration(milliseconds: 500), getStatus);

      return true;
    } catch (e) {
      print('Serial connection error: $e');
      _setState(DeviceConnectionState.disconnected);
      return false;
    }
  }

  /// Connetti via WebSocket
  Future<bool> connectWebSocket(String host, {int port = 80}) async {
    disconnect();

    _setState(DeviceConnectionState.connecting);
    _connectionType = ConnectionType.websocket;
    _wsHost = host;

    try {
      final uri = Uri.parse('ws://$host:$port/ws');
      _wsChannel = WebSocketChannel.connect(uri);

      await _wsChannel!.ready;

      _connectedName = host;
      _setState(DeviceConnectionState.connected);

      _wsSubscription = _wsChannel!.stream.listen(
        _onWebSocketMessage,
        onError: (e) {
          print('WebSocket error: $e');
          _setState(DeviceConnectionState.disconnected);
          _scheduleReconnect();
        },
        onDone: () {
          print('WebSocket closed');
          _setState(DeviceConnectionState.disconnected);
          _scheduleReconnect();
        },
      );

      _startPing();
      getStatus();

      return true;
    } catch (e) {
      print('WebSocket connection error: $e');
      _setState(DeviceConnectionState.disconnected);
      return false;
    }
  }

  /// Disconnetti
  void disconnect() {
    _pingTimer?.cancel();
    _reconnectTimer?.cancel();

    // Serial
    _serialReader?.close();
    _serialReader = null;
    _serialPort?.close();
    _serialPort?.dispose();
    _serialPort = null;
    _serialBuffer = '';

    // WebSocket
    _wsSubscription?.cancel();
    _wsChannel?.sink.close();
    _wsChannel = null;

    _connectedName = null;
    _connectionType = ConnectionType.none;
    _setState(DeviceConnectionState.disconnected);
  }

  void _setState(DeviceConnectionState newState) {
    _state = newState;
    _connectionController.add(newState);
  }

  void _scheduleReconnect() {
    if (_wsHost == null) return;

    _reconnectTimer?.cancel();
    _reconnectTimer = Timer(const Duration(seconds: 3), () {
      if (_state == DeviceConnectionState.disconnected && _wsHost != null) {
        connectWebSocket(_wsHost!);
      }
    });
  }

  void _startPing() {
    _pingTimer?.cancel();
    _pingTimer = Timer.periodic(const Duration(seconds: 30), (_) {
      if (isConnected) getStatus();
    });
  }

  // ═══════════════════════════════════════════
  // Gestione dati
  // ═══════════════════════════════════════════

  void _onSerialData(Uint8List data) {
    _serialBuffer += String.fromCharCodes(data);

    while (_serialBuffer.contains('\n')) {
      final index = _serialBuffer.indexOf('\n');
      final line = _serialBuffer.substring(0, index).trim();
      _serialBuffer = _serialBuffer.substring(index + 1);

      if (line.isNotEmpty) {
        _dataController.add(line);
        _parseSerialLine(line);
      }
    }
  }

  void _parseSerialLine(String line) {
    // Prova a parsare come JSON (risposte WebSocket-style via serial)
    if (line.startsWith('{')) {
      try {
        final json = jsonDecode(line) as Map<String, dynamic>;
        _handleJsonResponse(json);
      } catch (_) {
        // Non è JSON valido, ignora
      }
    }
  }

  void _onWebSocketMessage(dynamic message) {
    try {
      final json = jsonDecode(message as String) as Map<String, dynamic>;
      _dataController.add(message);
      _handleJsonResponse(json);
    } catch (e) {
      print('Error parsing WebSocket message: $e');
    }
  }

  void _handleJsonResponse(Map<String, dynamic> json) {
    final type = json['type'] as String?;

    switch (type) {
      case 'status':
        _lastStatus = DeviceStatus.fromJson(json);
        _statusController.add(_lastStatus!);
        break;

      case 'settings':
        _lastSettings = DeviceSettings.fromJson(json);
        _settingsController.add(_lastSettings!);
        break;

      case 'effects':
        _lastEffects = (json['list'] as List)
            .map((e) => EffectInfo.fromJson(e))
            .toList();
        _effectsController.add(_lastEffects!);
        break;

      case 'effectChange':
        getStatus();
        break;

      case 'welcome':
        print('Connected: ${json['message']}');
        break;
    }
  }

  // ═══════════════════════════════════════════
  // Invio comandi
  // ═══════════════════════════════════════════

  /// Invia comando raw (seriale)
  void sendRaw(String command) {
    if (!isConnected) return;

    if (_connectionType == ConnectionType.serial &&
        _serialPort?.isOpen == true) {
      _serialPort!.write(Uint8List.fromList('$command\n'.codeUnits));
    }
  }

  /// Invia comando JSON (WebSocket o seriale)
  void sendJson(Map<String, dynamic> command) {
    if (!isConnected) return;

    if (_connectionType == ConnectionType.websocket && _wsChannel != null) {
      _wsChannel!.sink.add(jsonEncode(command));
    } else if (_connectionType == ConnectionType.serial) {
      // Per seriale, usa comandi legacy
      _sendSerialCommand(command);
    }
  }

  /// Converte comando JSON in comando seriale legacy
  void _sendSerialCommand(Map<String, dynamic> cmd) {
    final command = cmd['cmd'] as String?;

    switch (command) {
      case 'setDateTime':
        final y = cmd['year'];
        final mo = (cmd['month'] as int).toString().padLeft(2, '0');
        final d = (cmd['day'] as int).toString().padLeft(2, '0');
        final h = (cmd['hour'] as int).toString().padLeft(2, '0');
        final m = (cmd['minute'] as int).toString().padLeft(2, '0');
        final s = (cmd['second'] as int? ?? 0).toString().padLeft(2, '0');
        sendRaw('D$y/$mo/$d $h:$m:$s');
        break;

      case 'setTime':
        final h = (cmd['hour'] as int).toString().padLeft(2, '0');
        final m = (cmd['minute'] as int).toString().padLeft(2, '0');
        final s = (cmd['second'] as int? ?? 0).toString().padLeft(2, '0');
        sendRaw('T$h:$m:$s');
        break;

      case 'setMode':
        sendRaw('M${cmd['mode']}');
        break;

      case 'effect':
        final action = cmd['action'];
        if (action == 'next')
          sendRaw('n');
        else if (action == 'pause')
          sendRaw('p');
        else if (action == 'resume')
          sendRaw('r');
        else if (action == 'select' && cmd.containsKey('index')) {
          sendRaw('${cmd['index']}');
        }
        break;

      case 'getStatus':
        sendRaw('S');
        break;

      case 'setBrightness':
        if (cmd.containsKey('value')) {
          sendRaw('B${cmd['value']}');
        }
        break;
    }
  }

  // ═══════════════════════════════════════════
  // Comandi high-level
  // ═══════════════════════════════════════════

  void getStatus() => sendJson({'cmd': 'getStatus'});
  void getSettings() => sendJson({'cmd': 'getSettings'});
  void getEffects() => sendJson({'cmd': 'getEffects'});
  void saveSettings() => sendJson({'cmd': 'saveSettings'});
  void restart() => sendJson({'cmd': 'restart'});

  // Time
  void setTime(int hour, int minute, [int second = 0]) {
    sendJson({
      'cmd': 'setTime',
      'hour': hour,
      'minute': minute,
      'second': second,
    });
  }

  void setDateTime(
    int year,
    int month,
    int day,
    int hour,
    int minute, [
    int second = 0,
  ]) {
    sendJson({
      'cmd': 'setDateTime',
      'year': year,
      'month': month,
      'day': day,
      'hour': hour,
      'minute': minute,
      'second': second,
    });
  }

  void syncNow() {
    final now = DateTime.now();
    setDateTime(now.year, now.month, now.day, now.hour, now.minute, now.second);
  }

  void setTimeMode(String mode) => sendJson({'cmd': 'setMode', 'mode': mode});

  // Effects
  void nextEffect() => sendJson({'cmd': 'effect', 'action': 'next'});
  void pause() => sendJson({'cmd': 'effect', 'action': 'pause'});
  void resume() => sendJson({'cmd': 'effect', 'action': 'resume'});
  void selectEffect(int index) =>
      sendJson({'cmd': 'effect', 'action': 'select', 'index': index});
  void selectEffectByName(String name) =>
      sendJson({'cmd': 'effect', 'action': 'select', 'name': name});
  void setEffectDuration(int ms) =>
      sendJson({'cmd': 'setEffectDuration', 'ms': ms});
  void setAutoSwitch(bool enabled) =>
      sendJson({'cmd': 'setAutoSwitch', 'enabled': enabled});

  // Display
  void setBrightness({int? day, int? night, int? value}) {
    final cmd = <String, dynamic>{'cmd': 'setBrightness'};
    if (day != null) cmd['day'] = day;
    if (night != null) cmd['night'] = night;
    if (value != null) cmd['value'] = value;
    sendJson(cmd);
  }

  void setNightTime(int startHour, int endHour) {
    sendJson({'cmd': 'setNightTime', 'start': startHour, 'end': endHour});
  }

  // WiFi
  void setWiFi(String ssid, String password, {bool apMode = false}) {
    sendJson({
      'cmd': 'setWiFi',
      'ssid': ssid,
      'password': password,
      'apMode': apMode,
    });
  }

  void dispose() {
    disconnect();
    _connectionController.close();
    _statusController.close();
    _effectsController.close();
    _settingsController.close();
    _dataController.close();
  }
}
