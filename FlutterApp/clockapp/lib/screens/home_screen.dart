import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'dart:async';
import 'dart:io';
import '../services/device_service.dart';

/// Informazioni su una rete WiFi
class WiFiNetwork {
  final String ssid;
  final int signalStrength;
  final bool isSecured;

  WiFiNetwork({
    required this.ssid,
    required this.signalStrength,
    this.isSecured = true,
  });

  String get signalIcon {
    if (signalStrength > 70) return '📶';
    if (signalStrength > 40) return '📶';
    return '📶';
  }
}

/// Scanner WiFi cross-platform
class WiFiScanner {
  static Future<List<WiFiNetwork>> scan() async {
    try {
      if (Platform.isLinux) {
        return await _scanLinux();
      } else if (Platform.isMacOS) {
        return await _scanMacOS();
      } else if (Platform.isWindows) {
        return await _scanWindows();
      }
    } catch (e) {
      print('WiFi scan error: $e');
    }
    return [];
  }

  static Future<List<WiFiNetwork>> _scanLinux() async {
    final result = await Process.run('nmcli', [
      '-t',
      '-f',
      'SSID,SIGNAL,SECURITY',
      'dev',
      'wifi',
      'list',
    ]);
    if (result.exitCode != 0) return [];

    final networks = <WiFiNetwork>[];
    final seen = <String>{};

    for (final line in (result.stdout as String).split('\n')) {
      if (line.trim().isEmpty) continue;
      final parts = line.split(':');
      if (parts.isEmpty || parts[0].isEmpty) continue;

      final ssid = parts[0];
      if (seen.contains(ssid)) continue;
      seen.add(ssid);

      final signal = parts.length > 1 ? int.tryParse(parts[1]) ?? 0 : 0;
      final security = parts.length > 2 ? parts[2] : '';

      networks.add(
        WiFiNetwork(
          ssid: ssid,
          signalStrength: signal,
          isSecured: security.isNotEmpty && security != '--',
        ),
      );
    }

    networks.sort((a, b) => b.signalStrength.compareTo(a.signalStrength));
    return networks;
  }

  static Future<List<WiFiNetwork>> _scanMacOS() async {
    final result = await Process.run(
      '/System/Library/PrivateFrameworks/Apple80211.framework/Versions/Current/Resources/airport',
      ['-s'],
    );
    if (result.exitCode != 0) return [];

    final networks = <WiFiNetwork>[];
    final seen = <String>{};
    final lines = (result.stdout as String).split('\n');

    for (int i = 1; i < lines.length; i++) {
      final line = lines[i].trim();
      if (line.isEmpty) continue;

      // Format: SSID BSSID RSSI CHANNEL HT CC SECURITY
      final match = RegExp(r'^(.+?)\s+([0-9a-f:]+)\s+(-?\d+)').firstMatch(line);
      if (match == null) continue;

      final ssid = match.group(1)!.trim();
      if (ssid.isEmpty || seen.contains(ssid)) continue;
      seen.add(ssid);

      final rssi = int.tryParse(match.group(3)!) ?? -100;
      final signal = ((rssi + 100) * 2).clamp(0, 100);

      networks.add(
        WiFiNetwork(
          ssid: ssid,
          signalStrength: signal,
          isSecured: line.contains('WPA') || line.contains('WEP'),
        ),
      );
    }

    networks.sort((a, b) => b.signalStrength.compareTo(a.signalStrength));
    return networks;
  }

  static Future<List<WiFiNetwork>> _scanWindows() async {
    final result = await Process.run('netsh', [
      'wlan',
      'show',
      'networks',
      'mode=Bssid',
    ]);
    if (result.exitCode != 0) return [];

    final networks = <WiFiNetwork>[];
    final seen = <String>{};
    String? currentSsid;
    int currentSignal = 0;
    bool currentSecured = false;

    for (final line in (result.stdout as String).split('\n')) {
      if (line.contains('SSID') && !line.contains('BSSID')) {
        if (currentSsid != null && !seen.contains(currentSsid)) {
          seen.add(currentSsid);
          networks.add(
            WiFiNetwork(
              ssid: currentSsid,
              signalStrength: currentSignal,
              isSecured: currentSecured,
            ),
          );
        }
        currentSsid = line.split(':').last.trim();
        currentSignal = 0;
        currentSecured = false;
      } else if (line.contains('Signal') || line.contains('Segnale')) {
        final match = RegExp(r'(\d+)%').firstMatch(line);
        if (match != null) {
          currentSignal = int.tryParse(match.group(1)!) ?? 0;
        }
      } else if (line.contains('Authentication') ||
          line.contains('Autenticazione')) {
        currentSecured = !line.contains('Open');
      }
    }

    if (currentSsid != null && !seen.contains(currentSsid)) {
      networks.add(
        WiFiNetwork(
          ssid: currentSsid,
          signalStrength: currentSignal,
          isSecured: currentSecured,
        ),
      );
    }

    networks.sort((a, b) => b.signalStrength.compareTo(a.signalStrength));
    return networks;
  }
}

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> with TickerProviderStateMixin {
  final DeviceService _device = DeviceService();

  // State
  bool _isConnected = false;
  DeviceStatus? _status;
  List<EffectInfo> _effects = [];
  DeviceSettings? _settings;

  // UI State
  final List<String> _consoleLog = [];
  final ScrollController _consoleScroll = ScrollController();

  // Subscriptions
  late List<StreamSubscription> _subscriptions;

  @override
  void initState() {
    super.initState();

    _subscriptions = [
      _device.connectionState.listen((state) {
        setState(() => _isConnected = state == DeviceConnectionState.connected);
        _addLog(
          state == DeviceConnectionState.connected
              ? '✓ Connesso a ${_device.connectedName} (${_device.connectionType.name})'
              : '✗ Disconnesso',
        );
      }),

      _device.statusStream.listen((status) {
        setState(() => _status = status);
      }),

      _device.effectsStream.listen((effects) {
        setState(() => _effects = effects);
      }),

      _device.settingsStream.listen((settings) {
        setState(() => _settings = settings);
      }),

      _device.rawDataStream.listen((data) {
        if (!data.startsWith('{')) {
          _addLog('← $data');
        }
      }),
    ];
  }

  @override
  void dispose() {
    for (final sub in _subscriptions) {
      sub.cancel();
    }
    _consoleScroll.dispose();
    super.dispose();
  }

  void _addLog(String message) {
    final now = DateTime.now();
    final time =
        '${now.hour.toString().padLeft(2, '0')}:${now.minute.toString().padLeft(2, '0')}:${now.second.toString().padLeft(2, '0')}';
    setState(() {
      _consoleLog.insert(0, '[$time] $message');
      if (_consoleLog.length > 50) _consoleLog.removeLast();
    });
  }

  Future<void> _showConnectionDialog() async {
    final result = await showDialog<bool>(
      context: context,
      builder: (context) => _ConnectionDialog(device: _device),
    );

    if (result == true && _isConnected) {
      // Connesso, richiedi dati
      _device.getEffects();
      _device.getSettings();
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: SafeArea(
        child: Column(
          children: [
            _buildHeader(),
            Expanded(
              child: ListView(
                padding: const EdgeInsets.all(16),
                children: [
                  _buildConnectionCard(),
                  const SizedBox(height: 16),
                  if (_isConnected) ...[
                    _buildStatusCard(),
                    const SizedBox(height: 16),
                    _buildTimeCard(),
                    const SizedBox(height: 16),
                    _buildEffectsCard(),
                    const SizedBox(height: 16),
                    _buildPlaybackCard(),
                    const SizedBox(height: 16),
                    _buildBrightnessCard(),
                    const SizedBox(height: 16),
                    _buildWiFiCard(),
                    const SizedBox(height: 16),
                  ],
                  _buildConsoleCard(),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildHeader() {
    return Container(
      padding: const EdgeInsets.all(20),
      child: Row(
        children: [
          Container(
            padding: const EdgeInsets.all(12),
            decoration: BoxDecoration(
              gradient: LinearGradient(
                colors: [
                  Theme.of(context).colorScheme.primary,
                  Theme.of(context).colorScheme.secondary,
                ],
              ),
              borderRadius: BorderRadius.circular(12),
            ),
            child: const Icon(Icons.grid_view_rounded, color: Colors.white),
          ),
          const SizedBox(width: 16),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const Text(
                  'LED Matrix',
                  style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold),
                ),
                Text(
                  _isConnected
                      ? 'Connesso via ${_device.connectionType.name}'
                      : 'Non connesso',
                  style: TextStyle(
                    color: _isConnected ? Colors.green[400] : Colors.grey,
                  ),
                ),
              ],
            ),
          ),
          Container(
            width: 12,
            height: 12,
            decoration: BoxDecoration(
              shape: BoxShape.circle,
              color: _isConnected ? Colors.green[400] : Colors.grey,
              boxShadow: _isConnected
                  ? [
                      BoxShadow(
                        color: Colors.green.withOpacity(0.4),
                        blurRadius: 8,
                        spreadRadius: 2,
                      ),
                    ]
                  : null,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildConnectionCard() {
    return _GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(
                _device.connectionType == ConnectionType.websocket
                    ? Icons.wifi
                    : Icons.usb_rounded,
                color: Theme.of(context).colorScheme.primary,
              ),
              const SizedBox(width: 12),
              const Text(
                'Connessione',
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
              ),
              const Spacer(),
              if (_isConnected &&
                  _device.connectionType == ConnectionType.websocket)
                Text(
                  _status?.ip ?? '',
                  style: TextStyle(color: Colors.grey[400], fontSize: 12),
                ),
            ],
          ),
          const SizedBox(height: 16),
          Row(
            children: [
              Expanded(
                child: _ActionButton(
                  icon: _isConnected ? Icons.link_off : Icons.link,
                  label: _isConnected ? 'Disconnetti' : 'Connetti',
                  color: _isConnected ? Colors.red[400]! : Colors.green[400]!,
                  onTap: () {
                    if (_isConnected) {
                      _device.disconnect();
                    } else {
                      _showConnectionDialog();
                    }
                  },
                ),
              ),
              if (_isConnected) ...[
                const SizedBox(width: 12),
                Expanded(
                  child: _ActionButton(
                    icon: Icons.refresh,
                    label: 'Refresh',
                    onTap: () {
                      _device.getStatus();
                      _device.getEffects();
                      _addLog('→ Refresh');
                      HapticFeedback.lightImpact();
                    },
                  ),
                ),
              ],
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildStatusCard() {
    if (_status == null) return const SizedBox.shrink();

    return _GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(
                Icons.info_outline,
                color: Theme.of(context).colorScheme.primary,
              ),
              const SizedBox(width: 12),
              const Text(
                'Stato',
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
              ),
            ],
          ),
          const SizedBox(height: 16),
          _StatusRow(label: 'Ora', value: _status!.time),
          _StatusRow(label: 'Data', value: _status!.date),
          _StatusRow(label: 'Effetto', value: _status!.effect),
          _StatusRow(label: 'FPS', value: _status!.fps.toStringAsFixed(1)),
          _StatusRow(
            label: 'DS3231',
            value: _status!.ds3231Available
                ? '✓ ${_status!.temperature?.toStringAsFixed(1)}°C'
                : '✗ Non trovato',
          ),
          _StatusRow(
            label: 'Auto-switch',
            value: _status!.autoSwitch ? 'ON' : 'OFF',
          ),
          _StatusRow(
            label: 'Luminosità',
            value:
                '${_status!.brightness} ${_status!.isNight ? '(notte)' : '(giorno)'}',
          ),
          if (_status!.wifiStatus.isNotEmpty)
            _StatusRow(label: 'WiFi', value: _status!.wifiStatus),
        ],
      ),
    );
  }

  Widget _buildTimeCard() {
    return _GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(
                Icons.access_time,
                color: Theme.of(context).colorScheme.primary,
              ),
              const SizedBox(width: 12),
              const Text(
                'Ora',
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
              ),
            ],
          ),
          const SizedBox(height: 16),
          Row(
            children: [
              Expanded(
                child: _ActionButton(
                  icon: Icons.sync,
                  label: 'Sincronizza',
                  onTap: () {
                    _device.syncNow();
                    _addLog('→ Sync ora');
                    HapticFeedback.lightImpact();
                  },
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: _ActionButton(
                  icon: Icons.schedule,
                  label: 'Imposta',
                  onTap: () => _showTimePicker(),
                ),
              ),
            ],
          ),
          const SizedBox(height: 12),
          Row(
            children: [
              Expanded(
                child: _ActionButton(
                  icon: Icons.timer,
                  label: 'RTC Mode',
                  color: _status?.timeMode.contains('RTC') == true
                      ? Colors.green[400]
                      : null,
                  onTap: () {
                    _device.setTimeMode('rtc');
                    _addLog('→ RTC mode');
                    HapticFeedback.lightImpact();
                  },
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: _ActionButton(
                  icon: Icons.fast_forward,
                  label: 'Fake Mode',
                  color: _status?.timeMode.contains('FAKE') == true
                      ? Colors.orange[400]
                      : null,
                  onTap: () {
                    _device.setTimeMode('fake');
                    _addLog('→ Fake mode');
                    HapticFeedback.lightImpact();
                  },
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Future<void> _showTimePicker() async {
    final time = await showTimePicker(
      context: context,
      initialTime: TimeOfDay.now(),
    );

    if (time != null) {
      final now = DateTime.now();
      _device.setDateTime(
        now.year,
        now.month,
        now.day,
        time.hour,
        time.minute,
        0,
      );
      _addLog('→ Set time ${time.hour}:${time.minute}');
    }
  }

  Widget _buildEffectsCard() {
    final effectsList = _effects.isNotEmpty ? _effects : _getDefaultEffects();

    return _GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(
                Icons.auto_awesome,
                color: Theme.of(context).colorScheme.primary,
              ),
              const SizedBox(width: 12),
              const Text(
                'Effetti',
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
              ),
              const Spacer(),
              if (_status != null)
                Container(
                  padding: const EdgeInsets.symmetric(
                    horizontal: 8,
                    vertical: 4,
                  ),
                  decoration: BoxDecoration(
                    color: Theme.of(
                      context,
                    ).colorScheme.primary.withOpacity(0.2),
                    borderRadius: BorderRadius.circular(8),
                  ),
                  child: Text(
                    _status!.effect,
                    style: TextStyle(
                      color: Theme.of(context).colorScheme.primary,
                      fontSize: 12,
                    ),
                  ),
                ),
            ],
          ),
          const SizedBox(height: 16),
          GridView.builder(
            shrinkWrap: true,
            physics: const NeverScrollableScrollPhysics(),
            gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
              crossAxisCount: 4,
              mainAxisSpacing: 8,
              crossAxisSpacing: 8,
              childAspectRatio: 1,
            ),
            itemCount: effectsList.length,
            itemBuilder: (context, index) {
              final effect = effectsList[index];
              final isSelected =
                  effect.isCurrent || (_status?.effectIndex == index);

              return _EffectButton(
                name: effect.name,
                index: index,
                isSelected: isSelected,
                onTap: () {
                  _device.selectEffect(index);
                  _addLog('→ Effect: ${effect.name}');
                  HapticFeedback.lightImpact();
                },
              );
            },
          ),
        ],
      ),
    );
  }

  List<EffectInfo> _getDefaultEffects() {
    final names = [
      'Scroll Text',
      'Plasma',
      'Pong',
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
    return names
        .asMap()
        .entries
        .map((e) => EffectInfo(index: e.key, name: e.value, isCurrent: false))
        .toList();
  }

  Widget _buildPlaybackCard() {
    return _GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(
                Icons.play_circle_outline,
                color: Theme.of(context).colorScheme.primary,
              ),
              const SizedBox(width: 12),
              const Text(
                'Controlli',
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
              ),
            ],
          ),
          const SizedBox(height: 16),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              _CircleButton(
                icon: Icons.pause,
                label: 'Pausa',
                isActive: _status?.autoSwitch == false,
                onTap: () {
                  _device.pause();
                  _addLog('→ Pause');
                  HapticFeedback.lightImpact();
                },
              ),
              _CircleButton(
                icon: Icons.play_arrow,
                label: 'Play',
                isActive: _status?.autoSwitch == true,
                onTap: () {
                  _device.resume();
                  _addLog('→ Resume');
                  HapticFeedback.lightImpact();
                },
              ),
              _CircleButton(
                icon: Icons.skip_next,
                label: 'Next',
                onTap: () {
                  _device.nextEffect();
                  _addLog('→ Next');
                  HapticFeedback.lightImpact();
                },
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildBrightnessCard() {
    return _GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(
                Icons.brightness_6,
                color: Theme.of(context).colorScheme.primary,
              ),
              const SizedBox(width: 12),
              const Text(
                'Luminosità',
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
              ),
            ],
          ),
          const SizedBox(height: 16),
          _BrightnessSlider(
            label: 'Giorno',
            icon: Icons.wb_sunny,
            value: _settings?.brightnessDay ?? 200,
            onChanged: (value) {
              _device.setBrightness(day: value);
            },
          ),
          const SizedBox(height: 12),
          _BrightnessSlider(
            label: 'Notte',
            icon: Icons.nightlight_round,
            value: _settings?.brightnessNight ?? 30,
            onChanged: (value) {
              _device.setBrightness(night: value);
            },
          ),
          const SizedBox(height: 16),
          Row(
            children: [
              Expanded(
                child: _ActionButton(
                  icon: Icons.save,
                  label: 'Salva',
                  onTap: () {
                    _device.saveSettings();
                    _addLog('→ Settings saved');
                    HapticFeedback.lightImpact();
                  },
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildWiFiCard() {
    return _GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(Icons.wifi, color: Theme.of(context).colorScheme.primary),
              const SizedBox(width: 12),
              const Text(
                'WiFi ESP32',
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
              ),
              const Spacer(),
              if (_status != null)
                Container(
                  padding: const EdgeInsets.symmetric(
                    horizontal: 8,
                    vertical: 4,
                  ),
                  decoration: BoxDecoration(
                    color: _status!.wifiStatus.contains('STA')
                        ? Colors.green.withOpacity(0.2)
                        : Colors.orange.withOpacity(0.2),
                    borderRadius: BorderRadius.circular(8),
                  ),
                  child: Text(
                    _status!.wifiStatus,
                    style: TextStyle(
                      color: _status!.wifiStatus.contains('STA')
                          ? Colors.green[400]
                          : Colors.orange[400],
                      fontSize: 12,
                    ),
                  ),
                ),
            ],
          ),
          const SizedBox(height: 16),

          // Info corrente
          if (_status != null) ...[
            _StatusRow(label: 'IP', value: _status!.ip),
            _StatusRow(
              label: 'SSID',
              value: _status!.ssid.isNotEmpty ? _status!.ssid : '(AP Mode)',
            ),
            if (_status!.rssi != null)
              _StatusRow(label: 'Segnale', value: '${_status!.rssi} dBm'),
            const SizedBox(height: 16),
          ],

          // Pulsante configura
          Row(
            children: [
              Expanded(
                child: _ActionButton(
                  icon: Icons.settings,
                  label: 'Configura WiFi',
                  onTap: () => _showWiFiConfigDialog(),
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: _ActionButton(
                  icon: Icons.restart_alt,
                  label: 'Riavvia',
                  color: Colors.orange[400],
                  onTap: () {
                    _showRestartConfirmDialog();
                  },
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Future<void> _showWiFiConfigDialog() async {
    final ssidController = TextEditingController(text: _settings?.ssid ?? '');
    final passwordController = TextEditingController();
    bool apMode = _settings?.apMode ?? true;
    List<WiFiNetwork> networks = [];
    bool scanning = false;
    String? selectedSsid;

    Future<void> scanNetworks(StateSetter setDialogState) async {
      setDialogState(() => scanning = true);
      try {
        networks = await WiFiScanner.scan();
        if (networks.isNotEmpty && selectedSsid == null) {
          selectedSsid = networks.first.ssid;
          ssidController.text = selectedSsid!;
        }
      } catch (e) {
        print('Scan error: $e');
      }
      setDialogState(() => scanning = false);
    }

    final result = await showDialog<bool>(
      context: context,
      builder: (context) => StatefulBuilder(
        builder: (context, setDialogState) {
          // Avvia scan automatico quando si apre in Station mode
          if (!apMode && networks.isEmpty && !scanning) {
            scanNetworks(setDialogState);
          }

          return Dialog(
            backgroundColor: const Color(0xFF1a1a2e),
            shape: RoundedRectangleBorder(
              borderRadius: BorderRadius.circular(16),
            ),
            child: Container(
              width: 420,
              padding: const EdgeInsets.all(24),
              child: Column(
                mainAxisSize: MainAxisSize.min,
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    children: [
                      const Icon(Icons.wifi, color: Color(0xFF8B5CF6)),
                      const SizedBox(width: 12),
                      const Text(
                        'Configura WiFi ESP32',
                        style: TextStyle(
                          fontSize: 20,
                          fontWeight: FontWeight.bold,
                        ),
                      ),
                      const Spacer(),
                      IconButton(
                        icon: const Icon(Icons.close),
                        onPressed: () => Navigator.of(context).pop(),
                      ),
                    ],
                  ),
                  const SizedBox(height: 24),

                  // Mode selector
                  Container(
                    decoration: BoxDecoration(
                      color: Colors.grey.withOpacity(0.1),
                      borderRadius: BorderRadius.circular(12),
                    ),
                    child: Row(
                      children: [
                        Expanded(
                          child: GestureDetector(
                            onTap: () {
                              setDialogState(() => apMode = false);
                              if (networks.isEmpty) {
                                scanNetworks(setDialogState);
                              }
                            },
                            child: Container(
                              padding: const EdgeInsets.symmetric(vertical: 12),
                              decoration: BoxDecoration(
                                color: !apMode
                                    ? const Color(0xFF8B5CF6)
                                    : Colors.transparent,
                                borderRadius: BorderRadius.circular(12),
                              ),
                              child: Row(
                                mainAxisAlignment: MainAxisAlignment.center,
                                children: [
                                  Icon(
                                    Icons.wifi,
                                    size: 18,
                                    color: !apMode ? Colors.white : Colors.grey,
                                  ),
                                  const SizedBox(width: 8),
                                  Text(
                                    'Station',
                                    style: TextStyle(
                                      color: !apMode
                                          ? Colors.white
                                          : Colors.grey,
                                      fontWeight: FontWeight.w500,
                                    ),
                                  ),
                                ],
                              ),
                            ),
                          ),
                        ),
                        Expanded(
                          child: GestureDetector(
                            onTap: () => setDialogState(() => apMode = true),
                            child: Container(
                              padding: const EdgeInsets.symmetric(vertical: 12),
                              decoration: BoxDecoration(
                                color: apMode
                                    ? const Color(0xFF8B5CF6)
                                    : Colors.transparent,
                                borderRadius: BorderRadius.circular(12),
                              ),
                              child: Row(
                                mainAxisAlignment: MainAxisAlignment.center,
                                children: [
                                  Icon(
                                    Icons.router,
                                    size: 18,
                                    color: apMode ? Colors.white : Colors.grey,
                                  ),
                                  const SizedBox(width: 8),
                                  Text(
                                    'Access Point',
                                    style: TextStyle(
                                      color: apMode
                                          ? Colors.white
                                          : Colors.grey,
                                      fontWeight: FontWeight.w500,
                                    ),
                                  ),
                                ],
                              ),
                            ),
                          ),
                        ),
                      ],
                    ),
                  ),

                  const SizedBox(height: 20),

                  if (!apMode) ...[
                    // Network selector
                    Row(
                      children: [
                        const Text(
                          'Rete WiFi:',
                          style: TextStyle(fontWeight: FontWeight.w500),
                        ),
                        const Spacer(),
                        IconButton(
                          icon: scanning
                              ? const SizedBox(
                                  width: 20,
                                  height: 20,
                                  child: CircularProgressIndicator(
                                    strokeWidth: 2,
                                  ),
                                )
                              : const Icon(Icons.refresh, size: 20),
                          onPressed: scanning
                              ? null
                              : () => scanNetworks(setDialogState),
                          tooltip: 'Scansiona reti',
                        ),
                      ],
                    ),
                    const SizedBox(height: 8),

                    if (networks.isNotEmpty)
                      Container(
                        decoration: BoxDecoration(
                          border: Border.all(
                            color: Colors.grey.withOpacity(0.3),
                          ),
                          borderRadius: BorderRadius.circular(8),
                        ),
                        child: DropdownButtonHideUnderline(
                          child: DropdownButton<String>(
                            value: networks.any((n) => n.ssid == selectedSsid)
                                ? selectedSsid
                                : null,
                            isExpanded: true,
                            padding: const EdgeInsets.symmetric(horizontal: 12),
                            dropdownColor: const Color(0xFF1a1a2e),
                            hint: const Text('Seleziona una rete'),
                            items: networks.map((network) {
                              return DropdownMenuItem(
                                value: network.ssid,
                                child: Row(
                                  children: [
                                    Icon(
                                      network.isSecured
                                          ? Icons.lock
                                          : Icons.lock_open,
                                      size: 16,
                                      color: Colors.grey[400],
                                    ),
                                    const SizedBox(width: 8),
                                    Expanded(
                                      child: Text(
                                        network.ssid,
                                        overflow: TextOverflow.ellipsis,
                                      ),
                                    ),
                                    const SizedBox(width: 8),
                                    _SignalIndicator(
                                      strength: network.signalStrength,
                                    ),
                                  ],
                                ),
                              );
                            }).toList(),
                            onChanged: (value) {
                              setDialogState(() {
                                selectedSsid = value;
                                ssidController.text = value ?? '';
                              });
                            },
                          ),
                        ),
                      )
                    else if (scanning)
                      Container(
                        padding: const EdgeInsets.all(16),
                        decoration: BoxDecoration(
                          color: Colors.grey.withOpacity(0.1),
                          borderRadius: BorderRadius.circular(8),
                        ),
                        child: const Row(
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: [
                            SizedBox(
                              width: 16,
                              height: 16,
                              child: CircularProgressIndicator(strokeWidth: 2),
                            ),
                            SizedBox(width: 12),
                            Text('Scansione reti in corso...'),
                          ],
                        ),
                      )
                    else
                      TextField(
                        controller: ssidController,
                        decoration: InputDecoration(
                          hintText: 'Inserisci SSID manualmente',
                          filled: true,
                          fillColor: Colors.grey.withOpacity(0.1),
                          border: OutlineInputBorder(
                            borderRadius: BorderRadius.circular(8),
                            borderSide: BorderSide.none,
                          ),
                          prefixIcon: const Icon(Icons.wifi),
                        ),
                      ),

                    const SizedBox(height: 16),
                    const Text(
                      'Password:',
                      style: TextStyle(fontWeight: FontWeight.w500),
                    ),
                    const SizedBox(height: 8),
                    TextField(
                      controller: passwordController,
                      obscureText: true,
                      decoration: InputDecoration(
                        hintText: 'Password WiFi',
                        filled: true,
                        fillColor: Colors.grey.withOpacity(0.1),
                        border: OutlineInputBorder(
                          borderRadius: BorderRadius.circular(8),
                          borderSide: BorderSide.none,
                        ),
                        prefixIcon: const Icon(Icons.lock),
                      ),
                    ),
                  ] else ...[
                    Container(
                      padding: const EdgeInsets.all(16),
                      decoration: BoxDecoration(
                        color: Colors.blue.withOpacity(0.1),
                        borderRadius: BorderRadius.circular(8),
                        border: Border.all(color: Colors.blue.withOpacity(0.3)),
                      ),
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Row(
                            children: [
                              Icon(
                                Icons.info_outline,
                                color: Colors.blue[400],
                                size: 20,
                              ),
                              const SizedBox(width: 8),
                              const Text(
                                'Modalità Access Point',
                                style: TextStyle(fontWeight: FontWeight.bold),
                              ),
                            ],
                          ),
                          const SizedBox(height: 8),
                          Text(
                            'L\'ESP32 creerà una rete WiFi propria.\n'
                            'SSID: ${_settings?.deviceName ?? "ledmatrix"}\n'
                            'Password: ledmatrix123\n'
                            'IP: 192.168.4.1',
                            style: TextStyle(
                              color: Colors.grey[400],
                              fontSize: 13,
                            ),
                          ),
                        ],
                      ),
                    ),
                  ],

                  const SizedBox(height: 24),

                  SizedBox(
                    width: double.infinity,
                    height: 48,
                    child: ElevatedButton(
                      onPressed: () => Navigator.of(context).pop(true),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: const Color(0xFF8B5CF6),
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(12),
                        ),
                      ),
                      child: const Text(
                        'Salva e Riavvia',
                        style: TextStyle(
                          fontSize: 16,
                          fontWeight: FontWeight.bold,
                        ),
                      ),
                    ),
                  ),
                ],
              ),
            ),
          );
        },
      ),
    );

    if (result == true) {
      _device.setWiFi(
        ssidController.text,
        passwordController.text,
        apMode: apMode,
      );
      _device.saveSettings();
      _addLog('→ WiFi config saved');

      // Chiedi se riavviare
      _showRestartConfirmDialog();
    }

    ssidController.dispose();
    passwordController.dispose();
  }

  Future<void> _showRestartConfirmDialog() async {
    final result = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        backgroundColor: const Color(0xFF1a1a2e),
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
        title: const Row(
          children: [
            Icon(Icons.restart_alt, color: Colors.orange),
            SizedBox(width: 12),
            Text('Riavviare?'),
          ],
        ),
        content: const Text(
          'L\'ESP32 si riavvierà per applicare le nuove impostazioni WiFi.\n\n'
          'Dovrai riconnetterti dopo il riavvio.',
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(false),
            child: const Text('Annulla'),
          ),
          ElevatedButton(
            onPressed: () => Navigator.of(context).pop(true),
            style: ElevatedButton.styleFrom(backgroundColor: Colors.orange),
            child: const Text('Riavvia'),
          ),
        ],
      ),
    );

    if (result == true) {
      _device.restart();
      _addLog('→ Restarting...');
    }
  }

  Widget _buildConsoleCard() {
    return _GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(
                Icons.terminal,
                color: Theme.of(context).colorScheme.primary,
              ),
              const SizedBox(width: 12),
              const Text(
                'Console',
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
              ),
              const Spacer(),
              IconButton(
                icon: const Icon(Icons.delete_outline, size: 20),
                onPressed: () => setState(() => _consoleLog.clear()),
                tooltip: 'Pulisci',
              ),
            ],
          ),
          const SizedBox(height: 12),
          Container(
            height: 200,
            decoration: BoxDecoration(
              color: Colors.black.withOpacity(0.3),
              borderRadius: BorderRadius.circular(8),
            ),
            child: ListView.builder(
              controller: _consoleScroll,
              padding: const EdgeInsets.all(12),
              itemCount: _consoleLog.length,
              itemBuilder: (context, index) {
                final log = _consoleLog[index];
                Color color = Colors.grey[400]!;
                if (log.contains('→')) color = Colors.blue[300]!;
                if (log.contains('←')) color = Colors.green[300]!;
                if (log.contains('✓')) color = Colors.green[400]!;
                if (log.contains('✗')) color = Colors.red[400]!;
                if (log.contains('⚠')) color = Colors.orange[400]!;

                return Padding(
                  padding: const EdgeInsets.symmetric(vertical: 2),
                  child: Text(
                    log,
                    style: TextStyle(
                      fontFamily: 'monospace',
                      fontSize: 12,
                      color: color,
                    ),
                  ),
                );
              },
            ),
          ),
        ],
      ),
    );
  }
}

// ═══════════════════════════════════════════
// Widgets
// ═══════════════════════════════════════════

class _GlassCard extends StatelessWidget {
  final Widget child;

  const _GlassCard({required this.child});

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.05),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: Colors.white.withOpacity(0.1)),
      ),
      child: child,
    );
  }
}

class _ActionButton extends StatelessWidget {
  final IconData icon;
  final String label;
  final VoidCallback? onTap;
  final Color? color;
  final bool enabled;

  const _ActionButton({
    required this.icon,
    required this.label,
    this.onTap,
    this.color,
    this.enabled = true,
  });

  @override
  Widget build(BuildContext context) {
    return Material(
      color: Colors.transparent,
      child: InkWell(
        onTap: enabled ? onTap : null,
        borderRadius: BorderRadius.circular(12),
        child: Container(
          padding: const EdgeInsets.symmetric(vertical: 12, horizontal: 16),
          decoration: BoxDecoration(
            color: (color ?? Theme.of(context).colorScheme.primary).withOpacity(
              enabled ? 0.15 : 0.05,
            ),
            borderRadius: BorderRadius.circular(12),
            border: Border.all(
              color: (color ?? Theme.of(context).colorScheme.primary)
                  .withOpacity(enabled ? 0.3 : 0.1),
            ),
          ),
          child: Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Icon(
                icon,
                size: 20,
                color: enabled
                    ? (color ?? Theme.of(context).colorScheme.primary)
                    : Colors.grey,
              ),
              const SizedBox(width: 8),
              Text(
                label,
                style: TextStyle(
                  color: enabled ? Colors.white : Colors.grey,
                  fontWeight: FontWeight.w500,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}

class _CircleButton extends StatelessWidget {
  final IconData icon;
  final String label;
  final VoidCallback? onTap;
  final bool isActive;

  const _CircleButton({
    required this.icon,
    required this.label,
    this.onTap,
    this.isActive = false,
  });

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        Material(
          color: Colors.transparent,
          child: InkWell(
            onTap: onTap,
            customBorder: const CircleBorder(),
            child: Container(
              width: 56,
              height: 56,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                color: isActive
                    ? Theme.of(context).colorScheme.primary
                    : Theme.of(context).colorScheme.primary.withOpacity(0.15),
                border: Border.all(
                  color: Theme.of(context).colorScheme.primary.withOpacity(0.3),
                ),
              ),
              child: Icon(
                icon,
                color: isActive
                    ? Colors.white
                    : Theme.of(context).colorScheme.primary,
              ),
            ),
          ),
        ),
        const SizedBox(height: 8),
        Text(label, style: TextStyle(fontSize: 12, color: Colors.grey[400])),
      ],
    );
  }
}

class _EffectButton extends StatelessWidget {
  final String name;
  final int index;
  final bool isSelected;
  final VoidCallback? onTap;

  const _EffectButton({
    required this.name,
    required this.index,
    required this.isSelected,
    this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    return Material(
      color: Colors.transparent,
      child: InkWell(
        onTap: onTap,
        borderRadius: BorderRadius.circular(12),
        child: Container(
          decoration: BoxDecoration(
            color: isSelected
                ? Theme.of(context).colorScheme.primary.withOpacity(0.3)
                : Colors.white.withOpacity(0.05),
            borderRadius: BorderRadius.circular(12),
            border: Border.all(
              color: isSelected
                  ? Theme.of(context).colorScheme.primary
                  : Colors.white.withOpacity(0.1),
              width: isSelected ? 2 : 1,
            ),
          ),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Text(
                '$index',
                style: TextStyle(
                  fontWeight: FontWeight.bold,
                  color: isSelected
                      ? Theme.of(context).colorScheme.primary
                      : Colors.white,
                ),
              ),
              const SizedBox(height: 4),
              Text(
                name.length > 8 ? '${name.substring(0, 8)}...' : name,
                style: TextStyle(fontSize: 9, color: Colors.grey[400]),
                textAlign: TextAlign.center,
                maxLines: 1,
              ),
            ],
          ),
        ),
      ),
    );
  }
}

class _StatusRow extends StatelessWidget {
  final String label;
  final String value;

  const _StatusRow({required this.label, required this.value});

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4),
      child: Row(
        children: [
          Text(label, style: TextStyle(color: Colors.grey[400])),
          const Spacer(),
          Text(value, style: const TextStyle(fontWeight: FontWeight.w500)),
        ],
      ),
    );
  }
}

class _BrightnessSlider extends StatefulWidget {
  final String label;
  final IconData icon;
  final int value;
  final Function(int) onChanged;

  const _BrightnessSlider({
    required this.label,
    required this.icon,
    required this.value,
    required this.onChanged,
  });

  @override
  State<_BrightnessSlider> createState() => _BrightnessSliderState();
}

class _BrightnessSliderState extends State<_BrightnessSlider> {
  late double _value;
  Timer? _debounce;

  @override
  void initState() {
    super.initState();
    _value = widget.value.toDouble();
  }

  @override
  void didUpdateWidget(_BrightnessSlider oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.value != widget.value) {
      _value = widget.value.toDouble();
    }
  }

  @override
  void dispose() {
    _debounce?.cancel();
    super.dispose();
  }

  void _onChanged(double value) {
    setState(() => _value = value);

    _debounce?.cancel();
    _debounce = Timer(const Duration(milliseconds: 300), () {
      widget.onChanged(value.round());
    });
  }

  @override
  Widget build(BuildContext context) {
    return Row(
      children: [
        Icon(widget.icon, size: 20, color: Colors.grey[400]),
        const SizedBox(width: 12),
        Text(widget.label, style: TextStyle(color: Colors.grey[400])),
        Expanded(
          child: Slider(value: _value, min: 0, max: 255, onChanged: _onChanged),
        ),
        SizedBox(
          width: 40,
          child: Text(
            _value.round().toString(),
            textAlign: TextAlign.right,
            style: const TextStyle(fontWeight: FontWeight.w500),
          ),
        ),
      ],
    );
  }
}

// ═══════════════════════════════════════════
// Connection Dialog
// ═══════════════════════════════════════════

class _ConnectionDialog extends StatefulWidget {
  final DeviceService device;

  const _ConnectionDialog({required this.device});

  @override
  State<_ConnectionDialog> createState() => _ConnectionDialogState();
}

class _ConnectionDialogState extends State<_ConnectionDialog>
    with SingleTickerProviderStateMixin {
  late TabController _tabController;

  // Serial
  List<SerialDevice> _serialDevices = [];
  SerialDevice? _selectedDevice;
  bool _scanningSerial = false;

  // WebSocket
  final _hostController = TextEditingController(text: 'ledmatrix.local');
  final _portController = TextEditingController(text: '80');

  bool _connecting = false;
  String? _error;

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 2, vsync: this);
    _scanSerialDevices();
  }

  @override
  void dispose() {
    _tabController.dispose();
    _hostController.dispose();
    _portController.dispose();
    super.dispose();
  }

  void _scanSerialDevices() {
    setState(() => _scanningSerial = true);

    try {
      _serialDevices = widget.device.getSerialDevices();
      if (_serialDevices.isNotEmpty) {
        _selectedDevice = _serialDevices.first;
      }
    } catch (e) {
      print('Error scanning serial: $e');
    }

    setState(() => _scanningSerial = false);
  }

  Future<void> _connectSerial() async {
    if (_selectedDevice == null) return;

    setState(() {
      _connecting = true;
      _error = null;
    });

    final success = await widget.device.connectSerial(_selectedDevice!);

    if (mounted) {
      if (success) {
        Navigator.of(context).pop(true);
      } else {
        setState(() {
          _connecting = false;
          _error = 'Impossibile connettersi alla porta seriale';
        });
      }
    }
  }

  Future<void> _connectWebSocket() async {
    final host = _hostController.text.trim();
    final port = int.tryParse(_portController.text) ?? 80;

    if (host.isEmpty) {
      setState(() => _error = 'Inserisci un indirizzo host');
      return;
    }

    setState(() {
      _connecting = true;
      _error = null;
    });

    final success = await widget.device.connectWebSocket(host, port: port);

    if (mounted) {
      if (success) {
        Navigator.of(context).pop(true);
      } else {
        setState(() {
          _connecting = false;
          _error = 'Impossibile connettersi a $host:$port';
        });
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Dialog(
      backgroundColor: const Color(0xFF1a1a2e),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
      child: Container(
        width: 400,
        padding: const EdgeInsets.all(24),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            // Header
            Row(
              children: [
                const Icon(Icons.cable, color: Color(0xFF8B5CF6)),
                const SizedBox(width: 12),
                const Text(
                  'Connetti Dispositivo',
                  style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
                ),
                const Spacer(),
                IconButton(
                  icon: const Icon(Icons.close),
                  onPressed: () => Navigator.of(context).pop(),
                ),
              ],
            ),
            const SizedBox(height: 16),

            // Tabs
            TabBar(
              controller: _tabController,
              indicatorColor: const Color(0xFF8B5CF6),
              labelColor: const Color(0xFF8B5CF6),
              unselectedLabelColor: Colors.grey,
              tabs: const [
                Tab(icon: Icon(Icons.usb), text: 'Seriale'),
                Tab(icon: Icon(Icons.wifi), text: 'WiFi'),
              ],
            ),
            const SizedBox(height: 24),

            // Tab content
            SizedBox(
              height: 200,
              child: TabBarView(
                controller: _tabController,
                children: [_buildSerialTab(), _buildWebSocketTab()],
              ),
            ),

            // Error
            if (_error != null) ...[
              const SizedBox(height: 16),
              Container(
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: Colors.red.withOpacity(0.1),
                  borderRadius: BorderRadius.circular(8),
                  border: Border.all(color: Colors.red.withOpacity(0.3)),
                ),
                child: Row(
                  children: [
                    const Icon(
                      Icons.error_outline,
                      color: Colors.red,
                      size: 20,
                    ),
                    const SizedBox(width: 8),
                    Expanded(
                      child: Text(
                        _error!,
                        style: const TextStyle(color: Colors.red),
                      ),
                    ),
                  ],
                ),
              ),
            ],

            const SizedBox(height: 24),

            // Connect button
            SizedBox(
              width: double.infinity,
              height: 48,
              child: ElevatedButton(
                onPressed: _connecting
                    ? null
                    : () {
                        if (_tabController.index == 0) {
                          _connectSerial();
                        } else {
                          _connectWebSocket();
                        }
                      },
                style: ElevatedButton.styleFrom(
                  backgroundColor: const Color(0xFF8B5CF6),
                  shape: RoundedRectangleBorder(
                    borderRadius: BorderRadius.circular(12),
                  ),
                ),
                child: _connecting
                    ? const SizedBox(
                        width: 24,
                        height: 24,
                        child: CircularProgressIndicator(
                          strokeWidth: 2,
                          valueColor: AlwaysStoppedAnimation(Colors.white),
                        ),
                      )
                    : const Text(
                        'Connetti',
                        style: TextStyle(
                          fontSize: 16,
                          fontWeight: FontWeight.bold,
                        ),
                      ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildSerialTab() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          children: [
            const Text(
              'Porta seriale:',
              style: TextStyle(fontWeight: FontWeight.w500),
            ),
            const Spacer(),
            IconButton(
              icon: _scanningSerial
                  ? const SizedBox(
                      width: 20,
                      height: 20,
                      child: CircularProgressIndicator(strokeWidth: 2),
                    )
                  : const Icon(Icons.refresh),
              onPressed: _scanningSerial ? null : _scanSerialDevices,
              tooltip: 'Aggiorna lista',
            ),
          ],
        ),
        const SizedBox(height: 8),

        if (_serialDevices.isEmpty)
          Container(
            padding: const EdgeInsets.all(16),
            decoration: BoxDecoration(
              color: Colors.grey.withOpacity(0.1),
              borderRadius: BorderRadius.circular(8),
            ),
            child: const Center(
              child: Text(
                'Nessun dispositivo trovato',
                style: TextStyle(color: Colors.grey),
              ),
            ),
          )
        else
          Container(
            decoration: BoxDecoration(
              border: Border.all(color: Colors.grey.withOpacity(0.3)),
              borderRadius: BorderRadius.circular(8),
            ),
            child: DropdownButtonHideUnderline(
              child: DropdownButton<SerialDevice>(
                value: _selectedDevice,
                isExpanded: true,
                padding: const EdgeInsets.symmetric(horizontal: 12),
                dropdownColor: const Color(0xFF1a1a2e),
                items: _serialDevices.map((device) {
                  return DropdownMenuItem(
                    value: device,
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        Text(device.displayName),
                        Text(
                          device.port,
                          style: TextStyle(
                            fontSize: 12,
                            color: Colors.grey[400],
                          ),
                        ),
                      ],
                    ),
                  );
                }).toList(),
                onChanged: (device) => setState(() => _selectedDevice = device),
              ),
            ),
          ),

        const SizedBox(height: 16),
        Text(
          'Collega l\'ESP32 via USB e seleziona la porta seriale.',
          style: TextStyle(fontSize: 12, color: Colors.grey[400]),
        ),
      ],
    );
  }

  Widget _buildWebSocketTab() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        const Text('Host:', style: TextStyle(fontWeight: FontWeight.w500)),
        const SizedBox(height: 8),
        TextField(
          controller: _hostController,
          decoration: InputDecoration(
            hintText: 'es. ledmatrix.local o 192.168.1.100',
            filled: true,
            fillColor: Colors.grey.withOpacity(0.1),
            border: OutlineInputBorder(
              borderRadius: BorderRadius.circular(8),
              borderSide: BorderSide.none,
            ),
            prefixIcon: const Icon(Icons.dns),
          ),
        ),
        const SizedBox(height: 16),

        const Text('Porta:', style: TextStyle(fontWeight: FontWeight.w500)),
        const SizedBox(height: 8),
        TextField(
          controller: _portController,
          keyboardType: TextInputType.number,
          decoration: InputDecoration(
            hintText: '80',
            filled: true,
            fillColor: Colors.grey.withOpacity(0.1),
            border: OutlineInputBorder(
              borderRadius: BorderRadius.circular(8),
              borderSide: BorderSide.none,
            ),
            prefixIcon: const Icon(Icons.numbers),
          ),
        ),

        const SizedBox(height: 16),
        Text(
          'Connettiti via WiFi usando mDNS o indirizzo IP.',
          style: TextStyle(fontSize: 12, color: Colors.grey[400]),
        ),
      ],
    );
  }
}

/// Widget per mostrare la potenza del segnale WiFi
class _SignalIndicator extends StatelessWidget {
  final int strength;

  const _SignalIndicator({required this.strength});

  @override
  Widget build(BuildContext context) {
    final bars = (strength / 25).ceil().clamp(1, 4);

    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        for (int i = 1; i <= 4; i++)
          Container(
            width: 3,
            height: 4.0 + (i * 3),
            margin: const EdgeInsets.only(left: 1),
            decoration: BoxDecoration(
              color: i <= bars
                  ? (bars >= 3
                        ? Colors.green
                        : (bars >= 2 ? Colors.orange : Colors.red))
                  : Colors.grey.withOpacity(0.3),
              borderRadius: BorderRadius.circular(1),
            ),
          ),
        const SizedBox(width: 4),
        Text(
          '$strength%',
          style: TextStyle(fontSize: 11, color: Colors.grey[400]),
        ),
      ],
    );
  }
}
