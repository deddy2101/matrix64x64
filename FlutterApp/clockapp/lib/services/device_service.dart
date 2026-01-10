import 'dart:async';
import 'dart:io';
import 'dart:typed_data';
import 'package:web_socket_channel/web_socket_channel.dart';

// Serial support (cross-platform, no external dependencies)
import 'serial_native.dart' as serial;
import 'pong_device_interface.dart';

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
  final bool ntpSynced;

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
    this.ntpSynced = false,
  });

  /// Parse STATUS response
  /// STATUS,time,date,mode,ds3231,temp,effect,idx,fps,auto,count,bright,night,wifi,ip,ssid,rssi,uptime,heap,ntpSynced
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
      ntpSynced: parts.length > 19 ? parts[19] == '1' : false,
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

/// Rete WiFi scansionata dall'ESP32
class ScannedWiFiNetwork {
  final String ssid;
  final int rssi;
  final bool isSecured;

  ScannedWiFiNetwork({
    required this.ssid,
    required this.rssi,
    this.isSecured = true,
  });

  /// Converte RSSI in percentuale segnale (0-100)
  int get signalStrength {
    // RSSI tipicamente va da -100 (debole) a -30 (forte)
    if (rssi >= -30) return 100;
    if (rssi <= -100) return 0;
    return ((rssi + 100) * 100 / 70).round().clamp(0, 100);
  }
}

/// Stato gioco Pong
class PongState {
  final String gameState;  // waiting, playing, paused, gameover
  final int score1;
  final int score2;
  final String player1Mode;  // human, ai
  final String player2Mode;
  final int ballX;
  final int ballY;

  PongState({
    this.gameState = 'waiting',
    this.score1 = 0,
    this.score2 = 0,
    this.player1Mode = 'ai',
    this.player2Mode = 'ai',
    this.ballX = 32,
    this.ballY = 32,
  });

  bool get isWaiting => gameState == 'waiting';
  bool get isPlaying => gameState == 'playing';
  bool get isPaused => gameState == 'paused';
  bool get isGameOver => gameState == 'gameover';
  bool get player1IsHuman => player1Mode == 'human';
  bool get player2IsHuman => player2Mode == 'human';

  /// Parse PONG_STATE response
  /// PONG_STATE,state,score1,score2,p1Mode,p2Mode,ballX,ballY
  factory PongState.fromResponse(String response) {
    final parts = response.split(',');
    if (parts.length < 8 || parts[0] != 'PONG_STATE') {
      return PongState();
    }

    return PongState(
      gameState: parts[1],
      score1: int.tryParse(parts[2]) ?? 0,
      score2: int.tryParse(parts[3]) ?? 0,
      player1Mode: parts[4],
      player2Mode: parts[5],
      ballX: int.tryParse(parts[6]) ?? 32,
      ballY: int.tryParse(parts[7]) ?? 32,
    );
  }
}

/// Stato gioco Snake
class SnakeState {
  final String gameState;  // waiting, playing, paused, gameover
  final int score;
  final int highScore;
  final int level;
  final int length;
  final int foodX;
  final int foodY;
  final int foodType;  // 0=normal, 1=bonus, 2=super
  final String direction;  // up, down, left, right
  final bool playerJoined;

  SnakeState({
    this.gameState = 'waiting',
    this.score = 0,
    this.highScore = 0,
    this.level = 1,
    this.length = 3,
    this.foodX = 8,
    this.foodY = 8,
    this.foodType = 0,
    this.direction = 'right',
    this.playerJoined = false,
  });

  bool get isWaiting => gameState == 'waiting';
  bool get isPlaying => gameState == 'playing';
  bool get isPaused => gameState == 'paused';
  bool get isGameOver => gameState == 'gameover';

  /// Parse SNAKE_STATE response
  /// SNAKE_STATE,state,score,highScore,level,length,foodX,foodY,foodType,direction,playerJoined
  factory SnakeState.fromResponse(String response) {
    final parts = response.split(',');
    if (parts.length < 11 || parts[0] != 'SNAKE_STATE') {
      return SnakeState();
    }

    return SnakeState(
      gameState: parts[1],
      score: int.tryParse(parts[2]) ?? 0,
      highScore: int.tryParse(parts[3]) ?? 0,
      level: int.tryParse(parts[4]) ?? 1,
      length: int.tryParse(parts[5]) ?? 3,
      foodX: int.tryParse(parts[6]) ?? 8,
      foodY: int.tryParse(parts[7]) ?? 8,
      foodType: int.tryParse(parts[8]) ?? 0,
      direction: parts[9],
      playerJoined: parts[10] == '1',
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
  final String scrollText;
  final bool ntpEnabled;
  final String timezone;

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
    this.ntpEnabled = true,
    this.timezone = 'CET-1CEST,M3.5.0,M10.5.0/3',
  });

  /// Parse SETTINGS response
  /// SETTINGS,ssid,apMode,brightDay,brightNight,nightStart,nightEnd,duration,auto,effect,deviceName,scrollText,ntpEnabled,timezone
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
      ntpEnabled: parts.length > 12 ? parts[12] == '1' : true,
      timezone: parts.length > 13 ? parts[13] : 'CET-1CEST,M3.5.0,M10.5.0/3',
    );
  }
}

/// Servizio unificato per comunicazione con LED Matrix
/// Supporta sia connessione Seriale che WebSocket
/// Protocollo: Stringhe CSV-like (no JSON)
class DeviceService implements IPongDevice {
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
  int? _wsPort;

  // WiFi scan state - durante la scansione la connessione può cadere temporaneamente
  bool _isWifiScanning = false;
  Timer? _wifiScanTimeoutTimer;
  static const Duration _wifiScanTimeout = Duration(seconds: 15);

  // Controllers
  final _connectionController =
      StreamController<DeviceConnectionState>.broadcast();
  final _statusController = StreamController<DeviceStatus>.broadcast();
  final _effectsController = StreamController<List<EffectInfo>>.broadcast();
  final _settingsController = StreamController<DeviceSettings>.broadcast();
  final _pongController = StreamController<PongState>.broadcast();
  final _snakeController = StreamController<SnakeState>.broadcast();
  final _dataController = StreamController<String>.broadcast();
  final _wifiScanController =
      StreamController<List<ScannedWiFiNetwork>>.broadcast();

  // Cache
  DeviceStatus? _lastStatus;
  List<EffectInfo>? _lastEffects;
  DeviceSettings? _lastSettings;
  PongState? _lastPongState;
  SnakeState? _lastSnakeState;

  // Streams
  Stream<DeviceConnectionState> get connectionState =>
      _connectionController.stream;
  Stream<DeviceStatus> get statusStream => _statusController.stream;
  Stream<List<EffectInfo>> get effectsStream => _effectsController.stream;
  Stream<DeviceSettings> get settingsStream => _settingsController.stream;
  Stream<PongState> get pongStream => _pongController.stream;
  Stream<SnakeState> get snakeStream => _snakeController.stream;
  Stream<String> get rawDataStream => _dataController.stream;
  Stream<List<ScannedWiFiNetwork>> get wifiScanStream =>
      _wifiScanController.stream;

  // Getters
  DeviceConnectionState get state => _state;
  ConnectionType get connectionType => _connectionType;
  bool get isConnected => _state == DeviceConnectionState.connected;
  String? get connectedName => _connectedName;
  bool get isWifiScanning => _isWifiScanning;
  DeviceStatus? get lastStatus => _lastStatus;
  List<EffectInfo>? get lastEffects => _lastEffects;
  DeviceSettings? get lastSettings => _lastSettings;
  PongState? get lastPongState => _lastPongState;
  SnakeState? get lastSnakeState => _lastSnakeState;

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
    // Non disconnettere se stiamo riconnettendo durante WiFi scan
    if (!_isWifiScanning) {
      disconnect();
    } else {
      // Pulisci solo WebSocket, mantieni stato
      _wsSubscription?.cancel();
      _wsChannel?.sink.close();
      _wsChannel = null;
    }

    _setState(DeviceConnectionState.connecting);
    _connectionType = ConnectionType.websocket;
    _wsHost = host;
    _wsPort = port;

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
          _handleWebSocketDisconnect();
        },
        onDone: () {
          print('WebSocket closed');
          _handleWebSocketDisconnect();
        },
      );

      _startPing();

      return true;
    } catch (e) {
      print('WebSocket connection error: $e');
      if (!_isWifiScanning) {
        _setState(DeviceConnectionState.disconnected);
      }
      _scheduleReconnect();
      return false;
    }
  }

  /// Gestisce disconnessione WebSocket - tiene conto dello stato WiFi scan
  void _handleWebSocketDisconnect() {
    if (_isWifiScanning) {
      // Durante WiFi scan, non notificare disconnessione, solo riconnetti silenziosamente
      print('WebSocket disconnected during WiFi scan - reconnecting silently...');
      _scheduleReconnect(quick: true);
    } else {
      _setState(DeviceConnectionState.disconnected);
      _scheduleReconnect();
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

  void _scheduleReconnect({bool quick = false}) {
    if (_wsHost == null) return;

    _reconnectTimer?.cancel();
    final delay = quick ? const Duration(milliseconds: 500) : const Duration(seconds: 3);
    _reconnectTimer = Timer(delay, () {
      if (_wsHost != null &&
          (_state == DeviceConnectionState.disconnected ||
           _state == DeviceConnectionState.connecting ||
           _isWifiScanning)) {
        connectWebSocket(_wsHost!, port: _wsPort ?? 80);
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
    } else if (response.startsWith('PONG_STATE,')) {
      _lastPongState = PongState.fromResponse(response);
      _pongController.add(_lastPongState!);
    } else if (response.startsWith('SNAKE_STATE,')) {
      _lastSnakeState = SnakeState.fromResponse(response);
      _snakeController.add(_lastSnakeState!);
    } else if (response.startsWith('WIFI_SCAN,')) {
      _endWifiScan();  // Scan completato, termina modalità scan
      final networks = _parseWiFiScan(response);
      _wifiScanController.add(networks);
    } else if (response.startsWith('WELCOME,')) {
      print('Connected: $response');
    }
  }

  List<ScannedWiFiNetwork> _parseWiFiScan(String response) {
    // Formato: WIFI_SCAN,count,ssid1,rssi1,secured1,ssid2,rssi2,secured2,...
    final parts = response.split(',');
    if (parts.length < 2 || parts[0] != 'WIFI_SCAN') return [];

    final count = int.tryParse(parts[1]) ?? 0;
    if (count <= 0) return [];

    final networks = <ScannedWiFiNetwork>[];
    final seen = <String>{};

    // Ogni rete ha 3 campi: ssid, rssi, secured
    for (int i = 0; i < count && (2 + i * 3 + 2) < parts.length; i++) {
      final baseIdx = 2 + i * 3;
      final ssid = parts[baseIdx];
      final rssi = int.tryParse(parts[baseIdx + 1]) ?? -100;
      final secured = parts[baseIdx + 2] == '1';

      // Salta duplicati
      if (ssid.isEmpty || seen.contains(ssid)) continue;
      seen.add(ssid);

      networks.add(ScannedWiFiNetwork(
        ssid: ssid,
        rssi: rssi,
        isSecured: secured,
      ));
    }

    // Ordina per segnale (più forte prima)
    networks.sort((a, b) => b.rssi.compareTo(a.rssi));

    return networks;
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

  /// Richiede all'ESP32 di scansionare le reti WiFi disponibili
  /// Il risultato arriverà via wifiScanStream
  /// Durante la scansione, la connessione WebSocket può cadere temporaneamente
  void requestWiFiScan() {
    _startWifiScan();
    send('wifiScan');
  }

  /// Inizia la modalità WiFi scan - previene la disconnessione durante lo scan
  void _startWifiScan() {
    _isWifiScanning = true;
    _wifiScanTimeoutTimer?.cancel();
    _wifiScanTimeoutTimer = Timer(_wifiScanTimeout, () {
      _endWifiScan();
    });
  }

  /// Termina la modalità WiFi scan
  void _endWifiScan() {
    _isWifiScanning = false;
    _wifiScanTimeoutTimer?.cancel();
    _wifiScanTimeoutTimer = null;
  }

  void setDeviceName(String name) => send('devicename,$name');

  // Scroll Text
  void setScrollText(String text) => send('scrolltext,$text');

  // ═══════════════════════════════════════════
  // Pong Multiplayer
  // ═══════════════════════════════════════════

  @override
  void pongJoin(int player) => send('pong,join,$player');
  @override
  void pongLeave(int player) => send('pong,leave,$player');
  @override
  void pongMove(int player, String direction) => send('pong,move,$player,$direction');
  @override
  void pongSetPosition(int player, int percentage) => send('pong,setpos,$player,$percentage');
  @override
  void pongStart() => send('pong,start');
  @override
  void pongPause() => send('pong,pause');
  @override
  void pongResume() => send('pong,resume');
  @override
  void pongReset() => send('pong,reset');
  @override
  void pongGetState() => send('pong,state');

  // ═══════════════════════════════════════════
  // Snake Game
  // ═══════════════════════════════════════════

  void snakeJoin() => send('snake,join');
  void snakeLeave() => send('snake,leave');
  void snakeSetDirection(String direction) => send('snake,dir,$direction');
  void snakeStart() => send('snake,start');
  void snakePause() => send('snake,pause');
  void snakeResume() => send('snake,resume');
  void snakeReset() => send('snake,reset');
  void snakeGetState() => send('snake,state');

  // ═══════════════════════════════════════════
  // NTP / Timezone
  // ═══════════════════════════════════════════

  void ntpEnable() => send('ntp,enable');
  void ntpDisable() => send('ntp,disable');
  void ntpSync() => send('ntp,sync');
  void setTimezone(String tz) => send('timezone,$tz');

  void dispose() {
    _wifiScanTimeoutTimer?.cancel();
    disconnect();
    _connectionController.close();
    _statusController.close();
    _effectsController.close();
    _settingsController.close();
    _pongController.close();
    _snakeController.close();
    _dataController.close();
    _wifiScanController.close();
  }
}
