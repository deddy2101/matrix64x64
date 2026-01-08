import 'dart:async';
import 'device_service.dart';

/// Servizio per gestire la modalità demo
/// Fornisce dati simulati quando non c'è un dispositivo reale
class DemoService {
  static final DemoService _instance = DemoService._internal();
  factory DemoService() => _instance;
  DemoService._internal();

  bool _isDemoMode = false;
  Timer? _statusTimer;

  // Controllers per simulare stream
  final _statusController = StreamController<DeviceStatus>.broadcast();
  final _effectsController = StreamController<List<EffectInfo>>.broadcast();
  final _settingsController = StreamController<DeviceSettings>.broadcast();
  final _connectionController = StreamController<DeviceConnectionState>.broadcast();
  final _pongController = StreamController<PongState>.broadcast();

  // Stato simulato
  int _currentEffectIndex = 0;
  int _simulatedBrightness = 200;
  bool _isNight = false;
  DateTime _simulatedTime = DateTime.now();

  // Stato Pong simulato
  String _pongGameState = 'waiting';
  int _pongScore1 = 0;
  int _pongScore2 = 0;
  bool _pongPlayer1IsHuman = false;
  bool _pongPlayer2IsHuman = false;

  // Getters
  bool get isDemoMode => _isDemoMode;
  Stream<DeviceStatus> get statusStream => _statusController.stream;
  Stream<List<EffectInfo>> get effectsStream => _effectsController.stream;
  Stream<DeviceSettings> get settingsStream => _settingsController.stream;
  Stream<DeviceConnectionState> get connectionState => _connectionController.stream;
  Stream<PongState> get pongStream => _pongController.stream;

  /// Lista effetti demo
  static const List<String> demoEffects = [
    'Scroll Text',
    'Plasma',
    'Pong Clock',
    'Matrix Rain',
    'Fire',
    'Starfield',
    'Paese',
    'Pokemon',
    'Mario Clock',
    'Andre',
    'Mario',
    'Luigi',
    'Cave',
    'Fox',
  ];

  /// Attiva modalità demo
  void startDemo() {
    _isDemoMode = true;
    _connectionController.add(DeviceConnectionState.connected);

    // Emetti effetti iniziali
    _emitEffects();
    _emitSettings();

    // Aggiorna status periodicamente
    _statusTimer = Timer.periodic(const Duration(seconds: 1), (_) {
      _simulatedTime = DateTime.now();
      _emitStatus();
    });

    // Emetti status iniziale
    _emitStatus();
  }

  /// Disattiva modalità demo
  void stopDemo() {
    _isDemoMode = false;
    _statusTimer?.cancel();
    _statusTimer = null;
    _connectionController.add(DeviceConnectionState.disconnected);
  }

  void _emitStatus() {
    final hour = _simulatedTime.hour;
    _isNight = hour >= 22 || hour < 7;

    final status = DeviceStatus(
      time: '${_simulatedTime.hour.toString().padLeft(2, '0')}:${_simulatedTime.minute.toString().padLeft(2, '0')}:${_simulatedTime.second.toString().padLeft(2, '0')}',
      date: '${_simulatedTime.year}/${_simulatedTime.month.toString().padLeft(2, '0')}/${_simulatedTime.day.toString().padLeft(2, '0')}',
      timeMode: 'DEMO',
      ds3231Available: true,
      temperature: 23.5,
      effect: demoEffects[_currentEffectIndex],
      effectIndex: _currentEffectIndex,
      fps: 60.0,
      autoSwitch: true,
      effectCount: demoEffects.length,
      brightness: _isNight ? 30 : _simulatedBrightness,
      isNight: _isNight,
      wifiStatus: 'DEMO',
      ip: '192.168.1.100',
      ssid: 'Demo Network',
      rssi: -55,
      uptime: DateTime.now().difference(DateTime(2024, 1, 1)).inSeconds % 86400,
      freeHeap: 150000,
    );

    _statusController.add(status);
  }

  void _emitEffects() {
    final effects = demoEffects.asMap().entries.map((e) => EffectInfo(
      index: e.key,
      name: e.value,
      isCurrent: e.key == _currentEffectIndex,
    )).toList();

    _effectsController.add(effects);
  }

  void _emitSettings() {
    final settings = DeviceSettings(
      ssid: 'Demo Network',
      apMode: false,
      brightnessDay: 200,
      brightnessNight: 30,
      nightStartHour: 22,
      nightEndHour: 7,
      effectDuration: 10000,
      autoSwitch: true,
      currentEffect: _currentEffectIndex,
      deviceName: 'LED Matrix Demo',
    );

    _settingsController.add(settings);
  }

  // Comandi simulati
  void selectEffect(int index) {
    if (index >= 0 && index < demoEffects.length) {
      _currentEffectIndex = index;
      _emitEffects();
      _emitStatus();
    }
  }

  void nextEffect() {
    _currentEffectIndex = (_currentEffectIndex + 1) % demoEffects.length;
    _emitEffects();
    _emitStatus();
  }

  void setBrightness(int value) {
    _simulatedBrightness = value.clamp(0, 255);
    _emitStatus();
  }

  void dispose() {
    stopDemo();
    _statusController.close();
    _effectsController.close();
    _settingsController.close();
    _connectionController.close();
    _pongController.close();
  }

  // ═══════════════════════════════════════════
  // Pong Demo Methods
  // ═══════════════════════════════════════════

  void _emitPongState() {
    final pongState = PongState(
      gameState: _pongGameState,
      score1: _pongScore1,
      score2: _pongScore2,
      player1Mode: _pongPlayer1IsHuman ? 'human' : 'ai',
      player2Mode: _pongPlayer2IsHuman ? 'human' : 'ai',
      ballX: 32,
      ballY: 32,
    );
    _pongController.add(pongState);
  }

  void pongJoin(int player) {
    if (player == 1 && !_pongPlayer1IsHuman) {
      _pongPlayer1IsHuman = true;
    } else if (player == 2 && !_pongPlayer2IsHuman) {
      _pongPlayer2IsHuman = true;
    }
    _emitPongState();
  }

  void pongLeave(int player) {
    if (player == 1 && _pongPlayer1IsHuman) {
      _pongPlayer1IsHuman = false;
    } else if (player == 2 && _pongPlayer2IsHuman) {
      _pongPlayer2IsHuman = false;
    }
    _emitPongState();
  }

  void pongMove(int player, String direction) {
    // In demo, non facciamo nulla di particolare
    // Il movimento è gestito dall'ESP nel caso reale
  }

  void pongSetPosition(int player, int percentage) {
    // In demo, non facciamo nulla di particolare
    // La posizione è gestita dall'ESP nel caso reale
  }

  void pongStart() {
    _pongGameState = 'playing';
    _pongScore1 = 0;
    _pongScore2 = 0;
    _emitPongState();

    // Simula un punto dopo 3 secondi
    Future.delayed(const Duration(seconds: 3), () {
      if (_pongGameState == 'playing') {
        _pongScore1 = 1;
        _emitPongState();
      }
    });

    // Simula un altro punto dopo 6 secondi
    Future.delayed(const Duration(seconds: 6), () {
      if (_pongGameState == 'playing') {
        _pongScore2 = 1;
        _emitPongState();
      }
    });
  }

  void pongPause() {
    _pongGameState = 'paused';
    _emitPongState();
  }

  void pongResume() {
    _pongGameState = 'playing';
    _emitPongState();
  }

  void pongReset() {
    _pongGameState = 'waiting';
    _pongScore1 = 0;
    _pongScore2 = 0;
    _emitPongState();
  }

  void pongGetState() {
    _emitPongState();
  }
}

/// Dispositivo demo fittizio
class DemoDevice {
  static const String name = 'LED Matrix (Demo)';
  static const String ip = 'demo.local';
  static const int port = 80;
  static const String description = 'Dispositivo di dimostrazione per testare l\'app senza hardware reale';
}
