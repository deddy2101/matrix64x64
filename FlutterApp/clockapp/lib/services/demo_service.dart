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

  // Stato simulato
  int _currentEffectIndex = 0;
  int _simulatedBrightness = 200;
  bool _isNight = false;
  DateTime _simulatedTime = DateTime.now();

  // Getters
  bool get isDemoMode => _isDemoMode;
  Stream<DeviceStatus> get statusStream => _statusController.stream;
  Stream<List<EffectInfo>> get effectsStream => _effectsController.stream;
  Stream<DeviceSettings> get settingsStream => _settingsController.stream;
  Stream<DeviceConnectionState> get connectionState => _connectionController.stream;

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
  }
}

/// Dispositivo demo fittizio
class DemoDevice {
  static const String name = 'LED Matrix (Demo)';
  static const String ip = 'demo.local';
  static const int port = 80;
  static const String description = 'Dispositivo di dimostrazione per testare l\'app senza hardware reale';
}
