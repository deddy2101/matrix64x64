import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'dart:async';
import 'package:file_picker/file_picker.dart';
import '../services/device_service.dart';
import '../services/ota_service.dart';
import '../services/firmware_update_service.dart';
import '../models/firmware_version.dart';
import '../widgets/common/common_widgets.dart';
import '../widgets/home/home_widgets.dart';
import '../dialogs/wifi_config_dialog.dart';
import '../dialogs/restart_confirm_dialog.dart';
import 'device_discovery_screen.dart';
import 'image_management_screen.dart';
import 'game_controller_screen.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> with TickerProviderStateMixin {
  final DeviceService _device = DeviceService();
  late final OtaService _otaService;
  final FirmwareUpdateService _firmwareUpdateService = FirmwareUpdateService();

  // State
  bool _isConnected = false;
  DeviceStatus? _status;
  List<EffectInfo> _effects = [];
  DeviceSettings? _settings;
  FirmwareVersion? _currentFirmwareVersion;
  FirmwareManifest? _firmwareManifest;
  bool _isLoadingVersions = false;

  // UI State
  final List<String> _consoleLog = [];
  final ScrollController _consoleScroll = ScrollController();
  final TextEditingController _scrollTextController = TextEditingController();

  // Subscriptions
  late List<StreamSubscription> _subscriptions;

  @override
  void initState() {
    super.initState();

    _otaService = OtaService(_device);

    _subscriptions = [
      _device.connectionState.listen((state) {
        final wasConnected = _isConnected;
        setState(() => _isConnected = state == DeviceConnectionState.connected);

        // Non loggare durante WiFi scan per evitare spam
        if (!_device.isWifiScanning) {
          _addLog(
            state == DeviceConnectionState.connected
                ? '✓ Connesso a ${_device.connectedName} (${_device.connectionType.name})'
                : '✗ Disconnesso',
          );
        }

        // Se era connesso e ora non lo è più, torna alla discovery
        // MA NON durante WiFi scan (la connessione può cadere temporaneamente)
        if (wasConnected && state == DeviceConnectionState.disconnected && !_device.isWifiScanning) {
          _addLog('Ritorno alla schermata di ricerca...');
          Future.delayed(const Duration(milliseconds: 500), () {
            if (mounted && !_device.isWifiScanning) {
              Navigator.of(context).pushReplacement(
                MaterialPageRoute(
                  builder: (_) => const DeviceDiscoveryScreen(),
                ),
              );
            }
          });
        }
      }),

      _device.statusStream.listen((status) {
        setState(() => _status = status);
      }),

      _device.effectsStream.listen((effects) {
        setState(() => _effects = effects);
      }),

      _device.settingsStream.listen((settings) {
        setState(() => _settings = settings);
        // Aggiorna il controller del testo scroll se diverso
        if (settings.scrollText.isNotEmpty &&
            _scrollTextController.text != settings.scrollText) {
          _scrollTextController.text = settings.scrollText;
        }
      }),

      _device.rawDataStream.listen((data) {
        if (!data.startsWith('{')) {
          _addLog('← $data');

          // Parse VERSION response
          if (data.startsWith('VERSION,')) {
            try {
              final version = FirmwareVersion.fromCsvResponse(data);
              setState(() => _currentFirmwareVersion = version);
              _addLog('✓ Versione firmware: ${version.fullVersion}');
              // Load manifest after getting current version
              _loadFirmwareManifest();
            } catch (e) {
              _addLog('⚠ Errore parsing versione: $e');
            }
          }
        }
      }),
    ];

    // Controlla stato iniziale
    _isConnected = _device.isConnected;
    if (_isConnected) {
      _addLog(
        '✓ Già connesso a ${_device.connectedName} (${_device.connectionType.name})',
      );
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (mounted && _isConnected) {
          _device.getStatus();
          _device.getEffects();
          _device.getSettings();
          _device.getVersion();
        }
      });
    }
  }

  @override
  void dispose() {
    for (final sub in _subscriptions) {
      sub.cancel();
    }
    _consoleScroll.dispose();
    _scrollTextController.dispose();
    super.dispose();
  }

  void _addLog(String message) {
    if (!mounted) return;
    final now = DateTime.now();
    final time =
        '${now.hour.toString().padLeft(2, '0')}:${now.minute.toString().padLeft(2, '0')}:${now.second.toString().padLeft(2, '0')}';
    setState(() {
      _consoleLog.insert(0, '[$time] $message');
      if (_consoleLog.length > 50) _consoleLog.removeLast();
    });
  }

  Future<void> _loadFirmwareManifest() async {
    if (_isLoadingVersions) return;

    setState(() => _isLoadingVersions = true);

    try {
      final manifest = await _firmwareUpdateService.fetchManifest();
      if (mounted) {
        setState(() {
          _firmwareManifest = manifest;
          _isLoadingVersions = false;
        });
        _addLog('✓ Manifest caricato: ${manifest.releases.length} versioni disponibili');
      }
    } catch (e) {
      if (mounted) {
        setState(() => _isLoadingVersions = false);
        _addLog('⚠ Errore caricamento manifest: $e');
      }
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
                    _buildScrollTextCard(),
                    const SizedBox(height: 16),
                    _buildPlaybackCard(),
                    const SizedBox(height: 16),
                    _buildBrightnessCard(),
                    const SizedBox(height: 16),
                    _buildWiFiCard(),
                    const SizedBox(height: 16),
                    _buildNTPCard(),
                    const SizedBox(height: 16),
                    _buildOTACard(),
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
          if (_isConnected) ...[
            IconButton(
              icon: const Icon(Icons.sports_esports),
              tooltip: 'Games (Pong & Snake)',
              onPressed: () {
                Navigator.push(
                  context,
                  MaterialPageRoute(
                    builder: (context) => GameControllerScreen(
                      deviceService: _device,
                    ),
                  ),
                );
              },
            ),
            IconButton(
              icon: const Icon(Icons.image),
              tooltip: 'Gestione Immagini',
              onPressed: () {
                Navigator.push(
                  context,
                  MaterialPageRoute(
                    builder: (context) => ImageManagementScreen(
                      deviceService: _device,
                    ),
                  ),
                );
              },
            ),
            const SizedBox(width: 8),
          ],
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
    return GlassCard(
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
                child: ActionButton(
                  icon: _isConnected ? Icons.link_off : Icons.link,
                  label: _isConnected ? 'Disconnetti' : 'Connetti',
                  color: _isConnected ? Colors.red[400]! : Colors.green[400]!,
                  onTap: () {
                    if (_isConnected) {
                      _device.disconnect();
                    } else {
                      // Torna alla discovery
                      Navigator.of(context).pushReplacement(
                        MaterialPageRoute(
                          builder: (_) => const DeviceDiscoveryScreen(),
                        ),
                      );
                    }
                  },
                ),
              ),
              if (_isConnected) ...[
                const SizedBox(width: 12),
                Expanded(
                  child: ActionButton(
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

    return GlassCard(
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
          StatusRow(label: 'Ora', value: _status!.time),
          StatusRow(label: 'Data', value: _status!.date),
          StatusRow(label: 'Effetto', value: _status!.effect),
          StatusRow(label: 'FPS', value: _status!.fps.toStringAsFixed(1)),
          StatusRow(
            label: 'DS3231',
            value: _status!.ds3231Available
                ? '✓ ${_status!.temperature?.toStringAsFixed(1)}°C'
                : '✗ Non trovato',
          ),
          StatusRow(
            label: 'Auto-switch',
            value: _status!.autoSwitch ? 'ON' : 'OFF',
          ),
          StatusRow(
            label: 'Luminosità',
            value:
                '${_status!.brightness} ${_status!.isNight ? '(notte)' : '(giorno)'}',
          ),
          if (_status!.wifiStatus.isNotEmpty)
            StatusRow(label: 'WiFi', value: _status!.wifiStatus),
        ],
      ),
    );
  }

  Widget _buildTimeCard() {
    return GlassCard(
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
                child: ActionButton(
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
                child: ActionButton(
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
                child: ActionButton(
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
                child: ActionButton(
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

    return GlassCard(
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

              return EffectButton(
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

  Widget _buildScrollTextCard() {
    return GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(
                Icons.text_fields,
                color: Theme.of(context).colorScheme.primary,
              ),
              const SizedBox(width: 12),
              const Text(
                'Testo Scorrevole',
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
              ),
            ],
          ),
          const SizedBox(height: 16),
          TextField(
            controller: _scrollTextController,
            maxLength: 127,
            decoration: InputDecoration(
              hintText: 'Inserisci il testo da scorrere...',
              filled: true,
              fillColor: Colors.white.withOpacity(0.1),
              border: OutlineInputBorder(
                borderRadius: BorderRadius.circular(12),
                borderSide: BorderSide.none,
              ),
              counterStyle: TextStyle(color: Colors.grey[500]),
              suffixIcon: IconButton(
                icon: const Icon(Icons.clear),
                onPressed: () {
                  _scrollTextController.clear();
                },
              ),
            ),
            style: const TextStyle(fontSize: 14),
          ),
          const SizedBox(height: 12),
          Row(
            children: [
              Expanded(
                child: ActionButton(
                  icon: Icons.send,
                  label: 'Applica',
                  onTap: () {
                    final text = _scrollTextController.text.trim();
                    if (text.isNotEmpty) {
                      _device.setScrollText(text);
                      _addLog('→ ScrollText: $text');
                      HapticFeedback.lightImpact();
                    }
                  },
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: ActionButton(
                  icon: Icons.save,
                  label: 'Salva',
                  onTap: () {
                    final text = _scrollTextController.text.trim();
                    if (text.isNotEmpty) {
                      _device.setScrollText(text);
                      _device.saveSettings();
                      _addLog('→ ScrollText salvato: $text');
                      HapticFeedback.lightImpact();
                    }
                  },
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildPlaybackCard() {
    return GlassCard(
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
              CircleButton(
                icon: Icons.pause,
                label: 'Pausa',
                isActive: _status?.autoSwitch == false,
                onTap: () {
                  _device.pause();
                  _addLog('→ Pause');
                  HapticFeedback.lightImpact();
                },
              ),
              CircleButton(
                icon: Icons.play_arrow,
                label: 'Play',
                isActive: _status?.autoSwitch == true,
                onTap: () {
                  _device.resume();
                  _addLog('→ Resume');
                  HapticFeedback.lightImpact();
                },
              ),
              CircleButton(
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
    final nightStart = _settings?.nightStartHour ?? 22;
    final nightEnd = _settings?.nightEndHour ?? 7;

    return GlassCard(
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
              const Spacer(),
              if (_status != null)
                Container(
                  padding: const EdgeInsets.symmetric(
                    horizontal: 8,
                    vertical: 4,
                  ),
                  decoration: BoxDecoration(
                    color: _status!.isNight
                        ? Colors.indigo.withOpacity(0.2)
                        : Colors.orange.withOpacity(0.2),
                    borderRadius: BorderRadius.circular(8),
                  ),
                  child: Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Icon(
                        _status!.isNight
                            ? Icons.nightlight_round
                            : Icons.wb_sunny,
                        size: 14,
                        color: _status!.isNight
                            ? Colors.indigo[300]
                            : Colors.orange[300],
                      ),
                      const SizedBox(width: 4),
                      Text(
                        _status!.isNight ? 'Notte' : 'Giorno',
                        style: TextStyle(
                          color: _status!.isNight
                              ? Colors.indigo[300]
                              : Colors.orange[300],
                          fontSize: 12,
                        ),
                      ),
                    ],
                  ),
                ),
            ],
          ),
          const SizedBox(height: 16),

          // Slider luminosità giorno
          BrightnessSlider(
            label: 'Giorno',
            icon: Icons.wb_sunny,
            value: _settings?.brightnessDay ?? 200,
            onChanged: (value) {
              _device.setBrightness(day: value);
            },
          ),
          const SizedBox(height: 12),

          // Slider luminosità notte
          BrightnessSlider(
            label: 'Notte',
            icon: Icons.nightlight_round,
            value: _settings?.brightnessNight ?? 30,
            onChanged: (value) {
              _device.setBrightness(night: value);
            },
          ),

          const SizedBox(height: 20),

          // Orari notte
          Row(
            children: [
              Icon(Icons.schedule, size: 20, color: Colors.grey[400]),
              const SizedBox(width: 12),
              Text(
                'Orario notturno:',
                style: TextStyle(color: Colors.grey[400]),
              ),
            ],
          ),
          const SizedBox(height: 12),

          Row(
            children: [
              Expanded(
                child: TimePickerButton(
                  label: 'Dalle',
                  hour: nightStart,
                  icon: Icons.nightlight_round,
                  onTap: () => _showNightTimeStartPicker(nightStart, nightEnd),
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: TimePickerButton(
                  label: 'Alle',
                  hour: nightEnd,
                  icon: Icons.wb_sunny,
                  onTap: () => _showNightTimeEndPicker(nightStart, nightEnd),
                ),
              ),
            ],
          ),

          const SizedBox(height: 16),
          Row(
            children: [
              Expanded(
                child: ActionButton(
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

  Future<void> _showNightTimeStartPicker(
    int currentStart,
    int currentEnd,
  ) async {
    final time = await showTimePicker(
      context: context,
      initialTime: TimeOfDay(hour: currentStart, minute: 0),
      helpText: 'Inizio modalità notte',
      builder: (context, child) {
        return MediaQuery(
          data: MediaQuery.of(context).copyWith(alwaysUse24HourFormat: true),
          child: child!,
        );
      },
    );

    if (time != null) {
      _device.setNightTime(time.hour, currentEnd);
      _device.getSettings();
      _addLog('→ Night start: ${time.hour}:00');
      HapticFeedback.lightImpact();
    }
  }

  Future<void> _showNightTimeEndPicker(int currentStart, int currentEnd) async {
    final time = await showTimePicker(
      context: context,
      initialTime: TimeOfDay(hour: currentEnd, minute: 0),
      helpText: 'Fine modalità notte',
      builder: (context, child) {
        return MediaQuery(
          data: MediaQuery.of(context).copyWith(alwaysUse24HourFormat: true),
          child: child!,
        );
      },
    );

    if (time != null) {
      _device.setNightTime(currentStart, time.hour);
      _device.getSettings();
      _addLog('→ Night end: ${time.hour}:00');
      HapticFeedback.lightImpact();
    }
  }

  Widget _buildWiFiCard() {
    return GlassCard(
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
            StatusRow(label: 'IP', value: _status!.ip),
            StatusRow(
              label: 'SSID',
              value: _status!.ssid.isNotEmpty ? _status!.ssid : '(AP Mode)',
            ),
            if (_status!.rssi != null)
              StatusRow(label: 'Segnale', value: '${_status!.rssi} dBm'),
            const SizedBox(height: 16),
          ],

          // Pulsanti
          Row(
            children: [
              Expanded(
                child: ActionButton(
                  icon: Icons.settings,
                  label: 'Configura WiFi',
                  onTap: () => _showWiFiConfigDialog(),
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: ActionButton(
                  icon: Icons.restart_alt,
                  label: 'Riavvia',
                  color: Colors.orange[400],
                  onTap: () => _showRestartConfirmDialog(),
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Future<void> _showWiFiConfigDialog() async {
    final result = await showDialog<Map<String, dynamic>>(
      context: context,
      builder: (context) => WiFiConfigDialog(
        currentSsid: _settings?.ssid,
        currentApMode: _settings?.apMode ?? false,
        deviceName: _settings?.deviceName,
      ),
    );

    if (result != null) {
      _device.setWiFi(
        result['ssid'],
        result['password'],
        apMode: result['apMode'],
      );
      _device.saveSettings();
      _addLog('→ WiFi config saved');

      // Chiedi se riavviare
      _showRestartConfirmDialog();
    }
  }

  Future<void> _showRestartConfirmDialog() async {
    final result = await showDialog<bool>(
      context: context,
      builder: (context) => const RestartConfirmDialog(),
    );

    if (result == true) {
      _device.restart();
      _addLog('→ Restarting...');
    }
  }

  Widget _buildNTPCard() {
    final ntpEnabled = _settings?.ntpEnabled ?? true;
    final timezone = _settings?.timezone ?? 'CET-1CEST,M3.5.0,M10.5.0/3';
    final ntpSynced = _status?.ntpSynced ?? false;

    return GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(
                Icons.access_time_filled,
                color: Theme.of(context).colorScheme.primary,
              ),
              const SizedBox(width: 12),
              const Text(
                'NTP / Timezone',
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
              ),
              const Spacer(),
              Container(
                padding: const EdgeInsets.symmetric(
                  horizontal: 8,
                  vertical: 4,
                ),
                decoration: BoxDecoration(
                  color: ntpSynced
                      ? Colors.green.withOpacity(0.2)
                      : ntpEnabled
                          ? Colors.orange.withOpacity(0.2)
                          : Colors.grey.withOpacity(0.2),
                  borderRadius: BorderRadius.circular(8),
                ),
                child: Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Icon(
                      ntpSynced
                          ? Icons.cloud_done
                          : ntpEnabled
                              ? Icons.cloud_sync
                              : Icons.cloud_off,
                      size: 14,
                      color: ntpSynced
                          ? Colors.green[400]
                          : ntpEnabled
                              ? Colors.orange[400]
                              : Colors.grey[400],
                    ),
                    const SizedBox(width: 4),
                    Text(
                      ntpSynced
                          ? 'Sincronizzato'
                          : ntpEnabled
                              ? 'In attesa'
                              : 'Disabilitato',
                      style: TextStyle(
                        color: ntpSynced
                            ? Colors.green[400]
                            : ntpEnabled
                                ? Colors.orange[400]
                                : Colors.grey[400],
                        fontSize: 12,
                      ),
                    ),
                  ],
                ),
              ),
            ],
          ),
          const SizedBox(height: 16),

          // Toggle NTP
          Row(
            children: [
              Icon(Icons.sync, size: 20, color: Colors.grey[400]),
              const SizedBox(width: 12),
              Expanded(
                child: Text(
                  'Sincronizzazione automatica',
                  style: TextStyle(color: Colors.grey[300]),
                ),
              ),
              Switch(
                value: ntpEnabled,
                onChanged: (value) {
                  if (value) {
                    _device.ntpEnable();
                    _addLog('→ NTP abilitato');
                  } else {
                    _device.ntpDisable();
                    _addLog('→ NTP disabilitato');
                  }
                  HapticFeedback.lightImpact();
                  // Refresh settings
                  Future.delayed(const Duration(milliseconds: 200), () {
                    _device.getSettings();
                    _device.getStatus();
                  });
                },
              ),
            ],
          ),
          const SizedBox(height: 8),

          // Timezone info
          StatusRow(
            label: 'Timezone',
            value: _getTimezoneDisplayName(timezone),
          ),

          const SizedBox(height: 16),

          // Pulsanti
          Row(
            children: [
              Expanded(
                child: ActionButton(
                  icon: Icons.sync,
                  label: 'Sync ora',
                  enabled: ntpEnabled,
                  onTap: () {
                    _device.ntpSync();
                    _addLog('→ NTP sync richiesto');
                    HapticFeedback.lightImpact();
                    // Refresh dopo sync
                    Future.delayed(const Duration(seconds: 2), () {
                      _device.getStatus();
                    });
                  },
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: ActionButton(
                  icon: Icons.public,
                  label: 'Timezone',
                  onTap: () => _showTimezoneDialog(),
                ),
              ),
            ],
          ),
          const SizedBox(height: 12),
          Row(
            children: [
              Expanded(
                child: ActionButton(
                  icon: Icons.save,
                  label: 'Salva',
                  onTap: () {
                    _device.saveSettings();
                    _addLog('→ Impostazioni NTP salvate');
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

  String _getTimezoneDisplayName(String tz) {
    // Converti timezone POSIX in nome leggibile
    if (tz.contains('CET')) return 'Europa/Roma (CET)';
    if (tz.contains('WET')) return 'Europa/Londra (WET)';
    if (tz.contains('EET')) return 'Europa/Atene (EET)';
    if (tz.contains('EST')) return 'America/New York (EST)';
    if (tz.contains('CST')) return 'America/Chicago (CST)';
    if (tz.contains('MST')) return 'America/Denver (MST)';
    if (tz.contains('PST')) return 'America/Los Angeles (PST)';
    if (tz.contains('JST')) return 'Asia/Tokyo (JST)';
    return tz;
  }

  Future<void> _showTimezoneDialog() async {
    final timezones = [
      {'name': 'Europa/Roma (Italia)', 'tz': 'CET-1CEST,M3.5.0,M10.5.0/3'},
      {'name': 'Europa/Londra (UK)', 'tz': 'GMT0BST,M3.5.0/1,M10.5.0'},
      {'name': 'Europa/Berlino (Germania)', 'tz': 'CET-1CEST,M3.5.0,M10.5.0/3'},
      {'name': 'Europa/Parigi (Francia)', 'tz': 'CET-1CEST,M3.5.0,M10.5.0/3'},
      {'name': 'Europa/Madrid (Spagna)', 'tz': 'CET-1CEST,M3.5.0,M10.5.0/3'},
      {'name': 'Europa/Atene (Grecia)', 'tz': 'EET-2EEST,M3.5.0/3,M10.5.0/4'},
      {'name': 'America/New York (EST)', 'tz': 'EST5EDT,M3.2.0,M11.1.0'},
      {'name': 'America/Chicago (CST)', 'tz': 'CST6CDT,M3.2.0,M11.1.0'},
      {'name': 'America/Denver (MST)', 'tz': 'MST7MDT,M3.2.0,M11.1.0'},
      {'name': 'America/Los Angeles (PST)', 'tz': 'PST8PDT,M3.2.0,M11.1.0'},
      {'name': 'Asia/Tokyo (Giappone)', 'tz': 'JST-9'},
      {'name': 'Asia/Shanghai (Cina)', 'tz': 'CST-8'},
      {'name': 'Australia/Sydney', 'tz': 'AEST-10AEDT,M10.1.0,M4.1.0/3'},
      {'name': 'UTC (Nessun offset)', 'tz': 'UTC0'},
    ];

    final currentTz = _settings?.timezone ?? 'CET-1CEST,M3.5.0,M10.5.0/3';

    final result = await showDialog<String>(
      context: context,
      builder: (context) => AlertDialog(
        backgroundColor: const Color(0xFF1a1a2e),
        title: Row(
          children: [
            Icon(Icons.public, color: Theme.of(context).colorScheme.primary),
            const SizedBox(width: 12),
            const Text('Seleziona Timezone'),
          ],
        ),
        content: SizedBox(
          width: double.maxFinite,
          height: 400,
          child: ListView.builder(
            itemCount: timezones.length,
            itemBuilder: (context, index) {
              final tz = timezones[index];
              final isSelected = tz['tz'] == currentTz;
              return ListTile(
                leading: Icon(
                  isSelected ? Icons.radio_button_checked : Icons.radio_button_off,
                  color: isSelected
                      ? Theme.of(context).colorScheme.primary
                      : Colors.grey,
                ),
                title: Text(
                  tz['name']!,
                  style: TextStyle(
                    color: isSelected ? Colors.white : Colors.grey[300],
                    fontWeight: isSelected ? FontWeight.bold : FontWeight.normal,
                  ),
                ),
                onTap: () => Navigator.of(context).pop(tz['tz']),
              );
            },
          ),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(),
            child: const Text('Annulla'),
          ),
        ],
      ),
    );

    if (result != null) {
      _device.setTimezone(result);
      _addLog('→ Timezone: $result');
      HapticFeedback.lightImpact();
      // Refresh settings
      Future.delayed(const Duration(milliseconds: 500), () {
        _device.getSettings();
        _device.getStatus();
      });
    }
  }

  Widget _buildOTACard() {
    return GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(
                Icons.system_update_alt,
                color: Theme.of(context).colorScheme.primary,
              ),
              const SizedBox(width: 12),
              const Expanded(
                child: Text(
                  'Firmware Update (OTA)',
                  style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
                ),
              ),
              IconButton(
                icon: _isLoadingVersions
                    ? const SizedBox(
                        width: 20,
                        height: 20,
                        child: CircularProgressIndicator(strokeWidth: 2),
                      )
                    : const Icon(Icons.refresh),
                onPressed: _isLoadingVersions ? null : _loadFirmwareManifest,
                tooltip: 'Ricarica versioni',
              ),
            ],
          ),
          const SizedBox(height: 16),

          // Current version
          if (_currentFirmwareVersion != null) ...[
            _buildVersionInfo('Versione corrente', _currentFirmwareVersion!, Colors.amber),
            const SizedBox(height: 12),
          ],

          // Latest version
          if (_firmwareManifest?.latestRelease != null) ...[
            _buildVersionInfo(
              'Ultima versione disponibile',
              null,
              Colors.green,
              release: _firmwareManifest!.latestRelease!,
            ),
            const SizedBox(height: 12),
          ],

          // All available versions
          if (_firmwareManifest != null && _firmwareManifest!.releases.length > 1) ...[
            ExpansionTile(
              title: Text(
                'Tutte le versioni (${_firmwareManifest!.releases.length})',
                style: const TextStyle(fontSize: 14, fontWeight: FontWeight.w500),
              ),
              children: _firmwareManifest!.releases.map((release) {
                final isCurrent = _currentFirmwareVersion != null &&
                    release.version == _currentFirmwareVersion!.version &&
                    release.buildNumber == _currentFirmwareVersion!.buildNumber;
                final isLatest = release.version == _firmwareManifest!.latestVersion;
                final color = isLatest
                    ? Colors.green
                    : isCurrent
                        ? Colors.amber
                        : Colors.grey;

                return ListTile(
                  dense: true,
                  leading: Icon(Icons.circle, size: 12, color: color),
                  title: Text(
                    release.fullVersion,
                    style: TextStyle(
                      fontSize: 13,
                      color: isCurrent || isLatest ? color : Colors.grey[400],
                      fontWeight: isCurrent || isLatest ? FontWeight.bold : FontWeight.normal,
                    ),
                  ),
                  subtitle: release.description != null
                      ? Text(
                          release.description!,
                          style: TextStyle(fontSize: 11, color: Colors.grey[600]),
                        )
                      : null,
                  trailing: isCurrent
                      ? const Chip(
                          label: Text('Installata', style: TextStyle(fontSize: 10)),
                          padding: EdgeInsets.symmetric(horizontal: 4),
                        )
                      : isLatest
                          ? const Chip(
                              label: Text('Più recente', style: TextStyle(fontSize: 10)),
                              padding: EdgeInsets.symmetric(horizontal: 4),
                            )
                          : null,
                );
              }).toList(),
            ),
            const SizedBox(height: 12),
          ],

          // Loading indicator
          if (_isLoadingVersions) ...[
            const Center(
              child: Padding(
                padding: EdgeInsets.all(12.0),
                child: CircularProgressIndicator(),
              ),
            ),
            const SizedBox(height: 12),
          ],

          const SizedBox(height: 4),

          // Progress indicator se update in corso
          if (_otaService.isUpdating) ...[
            Column(
              children: [
                LinearProgressIndicator(
                  value: _otaService.progress / 100,
                  backgroundColor: Colors.grey[800],
                  color: Colors.green[400],
                  minHeight: 8,
                ),
                const SizedBox(height: 12),
                Text(
                  '${_otaService.status} (${_otaService.progress}%)',
                  style: TextStyle(color: Colors.grey[400], fontSize: 14),
                ),
                const SizedBox(height: 16),
                ActionButton(
                  icon: Icons.cancel,
                  label: 'Annulla',
                  color: Colors.red[400],
                  onTap: () {
                    _otaService.abortUpdate();
                    _addLog('⚠ OTA annullato');
                    setState(() {});
                  },
                ),
              ],
            ),
          ] else ...[
            // Pulsante per installare ultima versione dal server
            if (_firmwareManifest?.latestRelease != null && _hasNewerVersion()) ...[
              ActionButton(
                icon: Icons.cloud_download,
                label: 'Installa ${_firmwareManifest!.latestRelease!.fullVersion}',
                color: Colors.green[400],
                onTap: () => _downloadAndInstallFirmware(_firmwareManifest!.latestRelease!),
              ),
              const SizedBox(height: 12),
            ],
            // Pulsante per selezionare file
            ActionButton(
              icon: Icons.upload_file,
              label: 'Seleziona file .bin',
              color: Colors.purple[400],
              onTap: () => _selectAndUploadFirmware(),
            ),
          ],
        ],
      ),
    );
  }

  Widget _buildVersionInfo(
    String label,
    FirmwareVersion? version,
    Color color, {
    FirmwareRelease? release,
  }) {
    final displayVersion = release != null ? release.fullVersion : version?.fullVersion ?? 'N/A';
    final buildDate = release != null
        ? release.releaseDate
        : version != null
            ? '${version.buildDate} ${version.buildTime}'
            : null;

    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: color.withOpacity(0.1),
        borderRadius: BorderRadius.circular(8),
        border: Border.all(color: color.withOpacity(0.3)),
      ),
      child: Row(
        children: [
          Icon(Icons.circle, size: 12, color: color),
          const SizedBox(width: 12),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  label,
                  style: TextStyle(
                    color: Colors.grey[400],
                    fontSize: 11,
                    fontWeight: FontWeight.w500,
                  ),
                ),
                const SizedBox(height: 4),
                Text(
                  displayVersion,
                  style: TextStyle(
                    color: color,
                    fontSize: 14,
                    fontWeight: FontWeight.bold,
                  ),
                ),
                if (buildDate != null) ...[
                  const SizedBox(height: 2),
                  Text(
                    buildDate,
                    style: TextStyle(
                      color: Colors.grey[600],
                      fontSize: 10,
                    ),
                  ),
                ],
              ],
            ),
          ),
        ],
      ),
    );
  }

  /// Verifica se c'è una versione più recente disponibile
  bool _hasNewerVersion() {
    if (_currentFirmwareVersion == null || _firmwareManifest?.latestRelease == null) {
      return false;
    }
    return _firmwareManifest!.latestRelease!.isNewerThan(_currentFirmwareVersion!);
  }

  /// Scarica e installa firmware dal server
  Future<void> _downloadAndInstallFirmware(FirmwareRelease release) async {
    try {
      _addLog('→ Download firmware ${release.fullVersion}...');
      setState(() {});

      // Scarica firmware
      final bytes = await _firmwareUpdateService.downloadFirmwareBytes(
        release,
        onProgress: (received, total) {
          final percent = (received * 100 / total).round();
          if (percent % 10 == 0) {
            _addLog('↓ Download: $percent%');
          }
        },
      );

      _addLog('✓ Download completato (${bytes.length} bytes)');
      _addLog('→ Inizio OTA update...');

      setState(() {}); // Aggiorna UI per mostrare progress

      // Esegui update
      final success = await _otaService.updateFirmwareFromBytes(
        Uint8List.fromList(bytes),
        expectedMd5: release.md5,
      );

      if (success) {
        _addLog('✓ Firmware aggiornato con successo!');
        _addLog('→ Il dispositivo si riavvierà automaticamente');

        if (mounted) {
          showDialog(
            context: context,
            builder: (context) => AlertDialog(
              backgroundColor: const Color(0xFF1a1a2e),
              title: Row(
                children: [
                  Icon(Icons.check_circle, color: Colors.green[400]),
                  const SizedBox(width: 12),
                  const Text('Update Completato'),
                ],
              ),
              content: Text(
                'Firmware ${release.fullVersion} installato con successo.\n\n'
                'Il dispositivo si riavvierà automaticamente tra pochi secondi.',
              ),
              actions: [
                TextButton(
                  onPressed: () => Navigator.pop(context),
                  child: const Text('OK'),
                ),
              ],
            ),
          );
        }
      } else {
        _addLog('✗ Errore durante l\'update: ${_otaService.status}');

        if (mounted) {
          showDialog(
            context: context,
            builder: (context) => AlertDialog(
              backgroundColor: const Color(0xFF1a1a2e),
              title: Row(
                children: [
                  Icon(Icons.error, color: Colors.red[400]),
                  const SizedBox(width: 12),
                  const Text('Errore Update'),
                ],
              ),
              content: Text(_otaService.status),
              actions: [
                TextButton(
                  onPressed: () => Navigator.pop(context),
                  child: const Text('OK'),
                ),
              ],
            ),
          );
        }
      }

      setState(() {}); // Aggiorna UI finale

    } catch (e) {
      _addLog('✗ Errore: $e');
      if (mounted) {
        showDialog(
          context: context,
          builder: (context) => AlertDialog(
            backgroundColor: const Color(0xFF1a1a2e),
            title: Row(
              children: [
                Icon(Icons.error, color: Colors.red[400]),
                const SizedBox(width: 12),
                const Text('Errore Download'),
              ],
            ),
            content: Text('$e'),
            actions: [
              TextButton(
                onPressed: () => Navigator.pop(context),
                child: const Text('OK'),
              ),
            ],
          ),
        );
      }
    }
  }

  Future<void> _selectAndUploadFirmware() async {
    try {
      // Seleziona file
      final result = await FilePicker.platform.pickFiles(
        type: FileType.custom,
        allowedExtensions: ['bin'],
      );

      if (result == null || result.files.isEmpty) {
        return;
      }

      final filePath = result.files.first.path;
      if (filePath == null) {
        _addLog('⚠ Impossibile leggere il file');
        return;
      }

      _addLog('→ Inizio OTA update...');
      _addLog('→ File: ${result.files.first.name}');

      setState(() {}); // Aggiorna UI per mostrare progress

      // Esegui update
      final success = await _otaService.updateFirmware(filePath);

      if (success) {
        _addLog('✓ Firmware aggiornato con successo!');
        _addLog('→ Il dispositivo si riavvierà automaticamente');

        // Mostra dialog di successo
        if (mounted) {
          showDialog(
            context: context,
            builder: (context) => AlertDialog(
              backgroundColor: const Color(0xFF1a1a2e),
              title: Row(
                children: [
                  Icon(Icons.check_circle, color: Colors.green[400]),
                  const SizedBox(width: 12),
                  const Text('Update Completato'),
                ],
              ),
              content: const Text(
                'Il firmware è stato aggiornato con successo.\n\n'
                'Il dispositivo si riavvierà automaticamente tra pochi secondi.',
              ),
              actions: [
                TextButton(
                  onPressed: () => Navigator.pop(context),
                  child: const Text('OK'),
                ),
              ],
            ),
          );
        }
      } else {
        _addLog('✗ Errore durante l\'update: ${_otaService.status}');

        // Mostra dialog di errore
        if (mounted) {
          showDialog(
            context: context,
            builder: (context) => AlertDialog(
              backgroundColor: const Color(0xFF1a1a2e),
              title: Row(
                children: [
                  Icon(Icons.error, color: Colors.red[400]),
                  const SizedBox(width: 12),
                  const Text('Errore Update'),
                ],
              ),
              content: Text(_otaService.status),
              actions: [
                TextButton(
                  onPressed: () => Navigator.pop(context),
                  child: const Text('OK'),
                ),
              ],
            ),
          );
        }
      }

      setState(() {}); // Aggiorna UI finale

    } catch (e) {
      _addLog('✗ Errore: $e');
    }
  }

  Widget _buildConsoleCard() {
    return GlassCard(
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
