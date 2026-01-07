import 'dart:async';
import 'dart:io';
import 'dart:typed_data';
import 'package:web_socket_channel/web_socket_channel.dart';

// Serial support (cross-platform, no external dependencies)
import 'serial_native.dart' as serial;

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

/// Helper per verificare se siamo su piattaforma desktop
bool get isDesktopPlatform {
  try {
    return Platform.isLinux || Platform.isMacOS || Platform.isWindows;
  } catch (_) {
    return false;
  }
}

/// Helper per verificare se siamo su piattaforma mobile
bool get isMobilePlatform {
  try {
    return Platform.isAndroid || Platform.isIOS;
  } catch (_) {
    return false;
  }
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

  /// Parse STATUS response
  /// STATUS,time,date,mode,ds3231,temp,effect,idx,fps,auto,count,bright,night,wifi,ip,ssid,rssi,uptime,heap
  factory DeviceStatus.fromResponse(String response) {
    final parts = response.split(',');
    if (parts.length < 19 || parts[0] != 'STATUS') {
      return DeviceStatus();
    }

    return DeviceStatus(
      time: parts[1],
      date: parts[2],
      timeMode: parts[3],
      ds3231Available: parts[4] == '1',
      temperature: double.tryParse(parts[5]),
      effect: parts[6],
      effectIndex: int.tryParse(parts[7]) ?? -1,
      fps: double.tryParse(parts[8]) ?? 0,
      autoSwitch: parts[9] == '1',
      effectCount: int.tryParse(parts[10]) ?? 0,
      brightness: int.tryParse(parts[11]) ?? 0,
      isNight: parts[12] == '1',
      wifiStatus: parts[13],
      ip: parts[14],
      ssid: parts[15],
      rssi: int.tryParse(parts[16]),
      uptime: int.tryParse(parts[17]) ?? 0,
      freeHeap: int.tryParse(parts[18]) ?? 0,
    );
  }
}

/// Informazioni effetto
class EffectInfo {
  final int index;
  final String name;
  final bool isCurrent;

  EffectInfo({required this.index, required this.name, this.isCurrent = false});
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
  final String scrollText;

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
    this.scrollText = '',
  });

  /// Parse SETTINGS response
  /// SETTINGS,ssid,apMode,brightDay,brightNight,nightStart,nightEnd,duration,auto,effect,deviceName,scrollText
  factory DeviceSettings.fromResponse(String response) {
    final parts = response.split(',');
    if (parts.length < 11 || parts[0] != 'SETTINGS') {
      return DeviceSettings();
    }

    return DeviceSettings(
      ssid: parts[1],
      apMode: parts[2] == '1',
      brightnessDay: int.tryParse(parts[3]) ?? 200,
      brightnessNight: int.tryParse(parts[4]) ?? 30,
      nightStartHour: int.tryParse(parts[5]) ?? 22,
      nightEndHour: int.tryParse(parts[6]) ?? 7,
      effectDuration: int.tryParse(parts[7]) ?? 10000,
      autoSwitch: parts[8] == '1',
      currentEffect: int.tryParse(parts[9]) ?? -1,
      deviceName: parts[10],
      scrollText: parts.length > 11 ? parts[11] : '',
    );
  }
}

/// Servizio unificato per comunicazione con LED Matrix
/// Supporta sia connessione Seriale che WebSocket
/// Protocollo: Stringhe CSV-like (no JSON)
class DeviceService {
  static final DeviceService _instance = DeviceService._internal();
  factory DeviceService() => _instance;
  DeviceService._internal();

  // Connessione
  ConnectionType _connectionType = ConnectionType.none;
  DeviceConnectionState _state = DeviceConnectionState.disconnected;
  String? _connectedName;

  // Serial (solo desktop)
  dynamic _serialPort;
  dynamic _serialReader;
  StreamSubscription? _serialSubscription;
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

  /// Verifica se la connessione seriale è disponibile (solo desktop)
  bool get isSerialAvailable => isDesktopPlatform;

  // ═══════════════════════════════════════════
  // Connessione
  // ═══════════════════════════════════════════

  /// Lista dispositivi seriali disponibili (solo desktop)
  List<SerialDevice> getSerialDevices() {
    if (!isDesktopPlatform) return [];
    return serial.getAvailableDevices();
  }

  /// Connetti via seriale (solo desktop)
  Future<bool> connectSerial(
    SerialDevice device, {
    int baudRate = 115200,
  }) async {
    if (!isDesktopPlatform) {
      print('Serial not available on this platform');
      return false;
    }

    disconnect();

    _setState(DeviceConnectionState.connecting);
    _connectionType = ConnectionType.serial;

    try {
      final result = await serial.connect(
        device.port,
        baudRate: baudRate,
        onData: _onSerialData,
        onError: (e) {
          print('Serial error: $e');
          disconnect();
        },
        onDone: disconnect,
      );

      if (result == null) {
        print('Failed to open serial port');
        _setState(DeviceConnectionState.disconnected);
        return false;
      }

      _serialPort = result['port'];
      _serialReader = result['reader'];
      _serialSubscription = result['subscription'];

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

    // Serial (solo desktop)
    _serialSubscription?.cancel();
    _serialSubscription = null;
    if (isDesktopPlatform && _serialPort != null) {
      serial.closePort(_serialPort, _serialReader);
    }
    _serialReader = null;
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
        _parseResponse(line);
      }
    }
  }

  void _onWebSocketMessage(dynamic message) {
    final line = (message as String).trim();
    if (line.isNotEmpty) {
      _dataController.add(line);
      _parseResponse(line);
    }
  }

  void _parseResponse(String response) {
    if (response.startsWith('STATUS,')) {
      _lastStatus = DeviceStatus.fromResponse(response);
      _statusController.add(_lastStatus!);
    } else if (response.startsWith('EFFECTS,')) {
      _lastEffects = _parseEffects(response);
      _effectsController.add(_lastEffects!);
    } else if (response.startsWith('SETTINGS,')) {
      _lastSettings = DeviceSettings.fromResponse(response);
      _settingsController.add(_lastSettings!);
    } else if (response.startsWith('EFFECT,')) {
      // Notifica cambio effetto: EFFECT,index,name
      getStatus();
    } else if (response.startsWith('WELCOME,')) {
      print('Connected: $response');
    }
  }

  List<EffectInfo> _parseEffects(String response) {
    // EFFECTS,name1,name2,name3,...
    final parts = response.split(',');
    if (parts.isEmpty || parts[0] != 'EFFECTS') return [];

    final effects = <EffectInfo>[];
    for (int i = 1; i < parts.length; i++) {
      effects.add(
        EffectInfo(
          index: i - 1,
          name: parts[i],
          isCurrent: _lastStatus?.effectIndex == i - 1,
        ),
      );
    }
    return effects;
  }

  // ═══════════════════════════════════════════
  // Invio comandi
  // ═══════════════════════════════════════════

  /// Invia comando stringa
  void send(String command) {
    if (!isConnected) return;

    if (_connectionType == ConnectionType.websocket && _wsChannel != null) {
      _wsChannel!.sink.add(command);
    } else if (_connectionType == ConnectionType.serial &&
        _serialPort != null) {
      serial.write(_serialPort, '$command\n');
    }
  }

  // ═══════════════════════════════════════════
  // Comandi high-level
  // ═══════════════════════════════════════════

  void getStatus() => send('getStatus');
  void getSettings() => send('getSettings');
  void getEffects() => send('getEffects');
  void getVersion() => send('getVersion');
  void saveSettings() => send('save');
  void restart() => send('restart');

  // Time
  void setTime(int hour, int minute, [int second = 0]) {
    send('setTime,$hour,$minute,$second');
  }

  void setDateTime(
    int year,
    int month,
    int day,
    int hour,
    int minute, [
    int second = 0,
  ]) {
    send('setDateTime,$year,$month,$day,$hour,$minute,$second');
  }

  void syncNow() {
    final now = DateTime.now();
    setDateTime(now.year, now.month, now.day, now.hour, now.minute, now.second);
  }

  void setTimeMode(String mode) => send('setMode,$mode');

  // Effects
  void nextEffect() => send('effect,next');
  void pause() => send('effect,pause');
  void resume() => send('effect,resume');
  void selectEffect(int index) => send('effect,select,$index');
  void selectEffectByName(String name) => send('effect,name,$name');
  void setEffectDuration(int ms) => send('duration,$ms');
  void setAutoSwitch(bool enabled) => send('autoswitch,${enabled ? 1 : 0}');

  // Display
  void setBrightness({int? day, int? night, int? value}) {
    if (value != null) {
      send('brightness,$value');
    } else if (day != null) {
      send('brightness,day,$day');
    } else if (night != null) {
      send('brightness,night,$night');
    }
  }

  void setNightTime(int startHour, int endHour) {
    send('nighttime,$startHour,$endHour');
  }

  // WiFi
  void setWiFi(String ssid, String password, {bool apMode = false}) {
    send('wifi,$ssid,$password,${apMode ? 1 : 0}');
  }

  void setDeviceName(String name) => send('devicename,$name');

  // Scroll Text
  void setScrollText(String text) => send('scrolltext,$text');

  void dispose() {
    disconnect();
    _connectionController.close();
    _statusController.close();
    _effectsController.close();
    _settingsController.close();
    _dataController.close();
  }
}
