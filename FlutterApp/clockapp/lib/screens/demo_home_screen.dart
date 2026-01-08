import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'dart:async';
import '../services/demo_service.dart';
import '../services/device_service.dart';
import '../services/demo_device_adapter.dart';
import '../widgets/common/common_widgets.dart';
import '../widgets/home/home_widgets.dart';
import 'device_discovery_screen.dart';
import 'pong_controller_screen.dart';

/// Schermata Home in modalità Demo
/// Simula il comportamento della HomeScreen ma con dati fittizi
class DemoHomeScreen extends StatefulWidget {
  const DemoHomeScreen({super.key});

  @override
  State<DemoHomeScreen> createState() => _DemoHomeScreenState();
}

class _DemoHomeScreenState extends State<DemoHomeScreen> {
  final DemoService _demo = DemoService();

  DeviceStatus? _status;
  List<EffectInfo> _effects = [];
  DeviceSettings? _settings;

  final List<String> _consoleLog = [];
  final ScrollController _consoleScroll = ScrollController();

  late List<StreamSubscription> _subscriptions;

  @override
  void initState() {
    super.initState();

    _subscriptions = [
      _demo.statusStream.listen((status) {
        setState(() => _status = status);
      }),
      _demo.effectsStream.listen((effects) {
        setState(() => _effects = effects);
      }),
      _demo.settingsStream.listen((settings) {
        setState(() => _settings = settings);
      }),
    ];

    _addLog('✓ Modalità Demo attivata');
    _addLog('ℹ Questo è un dispositivo simulato');
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

  void _exitDemo() {
    _demo.stopDemo();
    Navigator.of(context).pushReplacement(
      MaterialPageRoute(builder: (_) => const DeviceDiscoveryScreen()),
    );
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
                  _buildDemoBanner(),
                  const SizedBox(height: 16),
                  _buildStatusCard(),
                  const SizedBox(height: 16),
                  _buildTimeCard(),
                  const SizedBox(height: 16),
                  _buildEffectsCard(),
                  const SizedBox(height: 16),
                  _buildPlaybackCard(),
                  const SizedBox(height: 16),
                  _buildPongCard(),
                  const SizedBox(height: 16),
                  _buildBrightnessCard(),
                  const SizedBox(height: 16),
                  _buildOTACard(),
                  const SizedBox(height: 16),
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
                  Colors.orange[600]!,
                  Colors.orange[400]!,
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
                  'Modalità Demo',
                  style: TextStyle(color: Colors.orange[400]),
                ),
              ],
            ),
          ),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
            decoration: BoxDecoration(
              color: Colors.orange.withValues(alpha: 0.2),
              borderRadius: BorderRadius.circular(20),
            ),
            child: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                Icon(Icons.play_circle, size: 16, color: Colors.orange[400]),
                const SizedBox(width: 4),
                Text(
                  'DEMO',
                  style: TextStyle(
                    color: Colors.orange[400],
                    fontWeight: FontWeight.bold,
                    fontSize: 12,
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildDemoBanner() {
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        gradient: LinearGradient(
          colors: [
            Colors.orange.withValues(alpha: 0.2),
            Colors.orange.withValues(alpha: 0.1),
          ],
        ),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: Colors.orange.withValues(alpha: 0.3)),
      ),
      child: Row(
        children: [
          Icon(Icons.info_outline, color: Colors.orange[400]),
          const SizedBox(width: 12),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  'Stai usando la modalità demo',
                  style: TextStyle(
                    color: Colors.orange[300],
                    fontWeight: FontWeight.w600,
                  ),
                ),
                const SizedBox(height: 4),
                Text(
                  'I dati mostrati sono simulati. Connetti un dispositivo reale per il controllo effettivo.',
                  style: TextStyle(
                    color: Colors.grey[400],
                    fontSize: 12,
                  ),
                ),
              ],
            ),
          ),
          const SizedBox(width: 12),
          TextButton(
            onPressed: _exitDemo,
            child: Text(
              'Esci',
              style: TextStyle(color: Colors.orange[400]),
            ),
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
              Icon(Icons.info_outline, color: Colors.orange[400]),
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
              Icon(Icons.access_time, color: Colors.orange[400]),
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
                    _addLog('→ Sync ora (demo)');
                    HapticFeedback.lightImpact();
                  },
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: ActionButton(
                  icon: Icons.schedule,
                  label: 'Imposta',
                  onTap: () {
                    _addLog('→ Imposta ora (demo)');
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

  Widget _buildEffectsCard() {
    final effectsList = _effects.isNotEmpty ? _effects : _getDefaultEffects();

    return GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(Icons.auto_awesome, color: Colors.orange[400]),
              const SizedBox(width: 12),
              const Text(
                'Effetti',
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
              ),
              const Spacer(),
              if (_status != null)
                Container(
                  padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                  decoration: BoxDecoration(
                    color: Colors.orange.withValues(alpha: 0.2),
                    borderRadius: BorderRadius.circular(8),
                  ),
                  child: Text(
                    _status!.effect,
                    style: TextStyle(color: Colors.orange[400], fontSize: 12),
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
                  _demo.selectEffect(index);
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
    return DemoService.demoEffects
        .asMap()
        .entries
        .map((e) => EffectInfo(index: e.key, name: e.value, isCurrent: false))
        .toList();
  }

  Widget _buildPlaybackCard() {
    return GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(Icons.play_circle_outline, color: Colors.orange[400]),
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
                onTap: () {
                  _addLog('→ Pause (demo)');
                  HapticFeedback.lightImpact();
                },
              ),
              CircleButton(
                icon: Icons.play_arrow,
                label: 'Play',
                onTap: () {
                  _addLog('→ Resume (demo)');
                  HapticFeedback.lightImpact();
                },
              ),
              CircleButton(
                icon: Icons.skip_next,
                label: 'Next',
                onTap: () {
                  _demo.nextEffect();
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

  Widget _buildPongCard() {
    return GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(Icons.sports_esports, color: Colors.orange[400]),
              const SizedBox(width: 12),
              const Text(
                'Pong Multiplayer',
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
              ),
            ],
          ),
          const SizedBox(height: 12),
          Text(
            'Prova il controller Pong con lo slider verticale in modalità demo',
            style: TextStyle(
              color: Colors.grey[400],
              fontSize: 13,
            ),
          ),
          const SizedBox(height: 16),
          SizedBox(
            width: double.infinity,
            child: ElevatedButton.icon(
              onPressed: () {
                _addLog('→ Apertura Pong Controller (demo)');
                HapticFeedback.mediumImpact();
                Navigator.push(
                  context,
                  MaterialPageRoute(
                    builder: (context) => PongControllerScreen(
                      deviceService: DemoDeviceAdapter(_demo),
                    ),
                  ),
                );
              },
              icon: const Icon(Icons.gamepad),
              label: const Text('Apri Controller'),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.orange[600],
                foregroundColor: Colors.white,
                padding: const EdgeInsets.symmetric(vertical: 16),
              ),
            ),
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
              Icon(Icons.brightness_6, color: Colors.orange[400]),
              const SizedBox(width: 12),
              const Text(
                'Luminosità',
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
              ),
              const Spacer(),
              if (_status != null)
                Container(
                  padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                  decoration: BoxDecoration(
                    color: _status!.isNight
                        ? Colors.indigo.withValues(alpha: 0.2)
                        : Colors.orange.withValues(alpha: 0.2),
                    borderRadius: BorderRadius.circular(8),
                  ),
                  child: Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Icon(
                        _status!.isNight ? Icons.nightlight_round : Icons.wb_sunny,
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
          BrightnessSlider(
            label: 'Giorno',
            icon: Icons.wb_sunny,
            value: _settings?.brightnessDay ?? 200,
            onChanged: (value) {
              _demo.setBrightness(value);
              _addLog('→ Brightness: $value');
            },
          ),
          const SizedBox(height: 12),
          BrightnessSlider(
            label: 'Notte',
            icon: Icons.nightlight_round,
            value: _settings?.brightnessNight ?? 30,
            onChanged: (value) {
              _addLog('→ Night brightness: $value (demo)');
            },
          ),
          const SizedBox(height: 20),
          Row(
            children: [
              Icon(Icons.schedule, size: 20, color: Colors.grey[400]),
              const SizedBox(width: 12),
              Text(
                'Orario notturno: $nightStart:00 - $nightEnd:00',
                style: TextStyle(color: Colors.grey[400]),
              ),
            ],
          ),
        ],
      ),
    );
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
                color: Colors.grey[600],
              ),
              const SizedBox(width: 12),
              const Text(
                'Firmware Update (OTA)',
                style: TextStyle(
                  fontSize: 18,
                  fontWeight: FontWeight.w600,
                  color: Colors.grey,
                ),
              ),
            ],
          ),
          const SizedBox(height: 16),

          // Info - funzione non disponibile in demo
          Container(
            padding: const EdgeInsets.all(12),
            decoration: BoxDecoration(
              color: Colors.orange.withOpacity(0.1),
              borderRadius: BorderRadius.circular(8),
              border: Border.all(color: Colors.orange.withOpacity(0.3)),
            ),
            child: Row(
              children: [
                Icon(Icons.lock_outline, size: 20, color: Colors.orange[300]),
                const SizedBox(width: 12),
                Expanded(
                  child: Text(
                    'Questa funzione è disponibile solo con un dispositivo reale connesso.',
                    style: TextStyle(color: Colors.orange[200], fontSize: 12),
                  ),
                ),
              ],
            ),
          ),

          const SizedBox(height: 16),

          // Pulsante disabilitato
          Opacity(
            opacity: 0.5,
            child: ActionButton(
              icon: Icons.upload_file,
              label: 'Seleziona file .bin',
              color: Colors.grey[700],
              onTap: () {
                // Mostra dialog informativo
                showDialog(
                  context: context,
                  builder: (context) => AlertDialog(
                    backgroundColor: const Color(0xFF1a1a2e),
                    title: Row(
                      children: [
                        Icon(Icons.info_outline, color: Colors.blue[400]),
                        const SizedBox(width: 12),
                        const Text('Modalità Demo'),
                      ],
                    ),
                    content: const Text(
                      'La funzione di aggiornamento firmware (OTA) è disponibile solo quando sei connesso a un dispositivo ESP32 reale.\n\n'
                      'Esci dalla modalità Demo e connettiti a un dispositivo per usare questa funzione.',
                    ),
                    actions: [
                      TextButton(
                        onPressed: () => Navigator.pop(context),
                        child: const Text('OK'),
                      ),
                    ],
                  ),
                );
              },
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildConsoleCard() {
    return GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(Icons.terminal, color: Colors.orange[400]),
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
              color: Colors.black.withValues(alpha: 0.3),
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
                if (log.contains('ℹ')) color = Colors.orange[300]!;

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
