import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../services/serial_service.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> with TickerProviderStateMixin {
  final SerialService _serial = SerialService();
  bool _isConnected = false;
  bool _isPaused = false;
  bool _isRtcMode = true;
  int _selectedEffect = 0;
  
  final List<String> _consoleLog = [];
  final ScrollController _consoleScroll = ScrollController();
  
  TimeOfDay _selectedTime = TimeOfDay.now();

  final List<Map<String, dynamic>> _effects = [
    {'name': 'Scroll Text', 'icon': Icons.text_fields},
    {'name': 'Plasma', 'icon': Icons.blur_on},
    {'name': 'Pong', 'icon': Icons.sports_tennis},
    {'name': 'Matrix Rain', 'icon': Icons.grid_on},
    {'name': 'Fire', 'icon': Icons.local_fire_department},
    {'name': 'Starfield', 'icon': Icons.star},
    {'name': 'Paese', 'icon': Icons.landscape},
    {'name': 'Pokemon', 'icon': Icons.catching_pokemon},
    {'name': 'Mario Clock', 'icon': Icons.access_time},
    {'name': 'Andre', 'icon': Icons.person},
  ];

  @override
  void initState() {
    super.initState();
    
    _serial.connectionState.listen((connected) {
      setState(() => _isConnected = connected);
      _addLog(connected ? '✓ Connesso a ${_serial.connectedDeviceName}' : '✗ Disconnesso');
    });

    _serial.dataStream.listen((data) {
      _addLog('← $data');
    });
  }

  void _addLog(String message) {
    final now = DateTime.now();
    final time = '${now.hour.toString().padLeft(2, '0')}:${now.minute.toString().padLeft(2, '0')}:${now.second.toString().padLeft(2, '0')}';
    setState(() {
      _consoleLog.insert(0, '[$time] $message');
      if (_consoleLog.length > 50) _consoleLog.removeLast();
    });
  }

  void _sendCommand(String cmd, String label) {
    _serial.send(cmd);
    _addLog('→ $label');
    HapticFeedback.lightImpact();
  }

  Future<void> _showDeviceSelector() async {
    final devices = _serial.getDevices();
    
    if (devices.isEmpty) {
      _addLog('⚠ Nessuna porta seriale trovata');
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('Nessuna porta seriale trovata')),
        );
      }
      return;
    }

    if (!mounted) return;
    
    final device = await showModalBottomSheet<SerialDevice>(
      context: context,
      backgroundColor: Colors.transparent,
      builder: (context) => _DeviceSelector(devices: devices),
    );

    if (device != null) {
      _addLog('Connessione a ${device.port}...');
      final success = _serial.connect(device);
      if (!success) {
        _addLog('✗ Connessione fallita');
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: SafeArea(
        child: Column(
          children: [
            // Header
            _buildHeader(),
            
            // Content
            Expanded(
              child: ListView(
                padding: const EdgeInsets.all(16),
                children: [
                  // Connection Card
                  _buildConnectionCard(),
                  const SizedBox(height: 16),
                  
                  // Time Control
                  _buildTimeCard(),
                  const SizedBox(height: 16),
                  
                  // Effects Grid
                  _buildEffectsCard(),
                  const SizedBox(height: 16),
                  
                  // Playback Controls
                  _buildPlaybackCard(),
                  const SizedBox(height: 16),
                  
                  // Console
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
                  style: TextStyle(
                    fontSize: 24,
                    fontWeight: FontWeight.bold,
                  ),
                ),
                Text(
                  _isConnected ? 'Connesso' : 'Non connesso',
                  style: TextStyle(
                    color: _isConnected ? Colors.green[400] : Colors.grey,
                  ),
                ),
              ],
            ),
          ),
          // Status indicator
          Container(
            width: 12,
            height: 12,
            decoration: BoxDecoration(
              shape: BoxShape.circle,
              color: _isConnected ? Colors.green[400] : Colors.grey,
              boxShadow: _isConnected ? [
                BoxShadow(
                  color: Colors.green.withValues(alpha: 100),
                  blurRadius: 8,
                  spreadRadius: 2,
                ),
              ] : null,
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
              Icon(Icons.usb_rounded, color: Theme.of(context).colorScheme.primary),
              const SizedBox(width: 12),
              const Text(
                'Connessione',
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
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
                      _serial.disconnect();
                    } else {
                      _showDeviceSelector();
                    }
                  },
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: _ActionButton(
                  icon: Icons.refresh,
                  label: 'Status',
                  enabled: _isConnected,
                  onTap: () => _sendCommand('S', 'Status'),
                ),
              ),
            ],
          ),
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
              Icon(Icons.schedule, color: Theme.of(context).colorScheme.primary),
              const SizedBox(width: 12),
              const Text(
                'Orologio',
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
              ),
              const Spacer(),
              // Mode Toggle
              Container(
                padding: const EdgeInsets.all(4),
                decoration: BoxDecoration(
                  color: Colors.white.withOpacity(0.05),
                  borderRadius: BorderRadius.circular(20),
                ),
                child: Row(
                  children: [
                    _ModeChip(
                      label: 'RTC',
                      selected: _isRtcMode,
                      onTap: () {
                        setState(() => _isRtcMode = true);
                        _sendCommand('Mrtc', 'Mode RTC');
                      },
                    ),
                    _ModeChip(
                      label: 'FAKE',
                      selected: !_isRtcMode,
                      onTap: () {
                        setState(() => _isRtcMode = false);
                        _sendCommand('Mfake', 'Mode Fake');
                      },
                    ),
                  ],
                ),
              ),
            ],
          ),
          const SizedBox(height: 20),
          
          // Time Display
          GestureDetector(
            onTap: () async {
              final time = await showTimePicker(
                context: context,
                initialTime: _selectedTime,
              );
              if (time != null) {
                setState(() => _selectedTime = time);
              }
            },
            child: Container(
              width: double.infinity,
              padding: const EdgeInsets.symmetric(vertical: 24),
              decoration: BoxDecoration(
                gradient: LinearGradient(
                  colors: [
                    Theme.of(context).colorScheme.primary.withOpacity(0.2),
                    Theme.of(context).colorScheme.secondary.withOpacity(0.1),
                  ],
                ),
                borderRadius: BorderRadius.circular(16),
                border: Border.all(
                  color: Theme.of(context).colorScheme.primary.withOpacity(0.3),
                ),
              ),
              child: Column(
                children: [
                  Text(
                    '${_selectedTime.hour.toString().padLeft(2, '0')}:${_selectedTime.minute.toString().padLeft(2, '0')}',
                    style: const TextStyle(
                      fontSize: 48,
                      fontWeight: FontWeight.w300,
                      letterSpacing: 4,
                    ),
                  ),
                  const SizedBox(height: 4),
                  Text(
                    'Tocca per modificare',
                    style: TextStyle(
                      fontSize: 12,
                      color: Colors.grey[500],
                    ),
                  ),
                ],
              ),
            ),
          ),
          
          const SizedBox(height: 16),
          Row(
            children: [
              Expanded(
                child: _ActionButton(
                  icon: Icons.sync,
                  label: 'Sync Ora Telefono',
                  enabled: _isConnected,
                  color: Theme.of(context).colorScheme.primary,
                  onTap: () {
                    _serial.syncNow();
                    _addLog('→ Sync ${TimeOfDay.now().format(context)}');
                    HapticFeedback.mediumImpact();
                  },
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: _ActionButton(
                  icon: Icons.send,
                  label: 'Invia Selezionato',
                  enabled: _isConnected,
                  onTap: () {
                    _serial.setTime(_selectedTime.hour, _selectedTime.minute, 0);
                    _addLog('→ Set ${_selectedTime.format(context)}');
                    HapticFeedback.mediumImpact();
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
    return _GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(Icons.auto_awesome, color: Theme.of(context).colorScheme.primary),
              const SizedBox(width: 12),
              const Text(
                'Effetti',
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
              ),
            ],
          ),
          const SizedBox(height: 16),
          GridView.builder(
            shrinkWrap: true,
            physics: const NeverScrollableScrollPhysics(),
            gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
              crossAxisCount: 5,
              mainAxisSpacing: 8,
              crossAxisSpacing: 8,
              childAspectRatio: 1,
            ),
            itemCount: _effects.length,
            itemBuilder: (context, index) {
              final effect = _effects[index];
              final isSelected = _selectedEffect == index;
              
              return GestureDetector(
                onTap: _isConnected ? () {
                  setState(() => _selectedEffect = index);
                  _serial.selectEffect(index);
                  _addLog('→ Effect: ${effect['name']}');
                  HapticFeedback.selectionClick();
                } : null,
                child: AnimatedContainer(
                  duration: const Duration(milliseconds: 200),
                  decoration: BoxDecoration(
                    color: isSelected 
                        ? Theme.of(context).colorScheme.primary.withOpacity(0.3)
                        : Colors.white.withOpacity(0.05),
                    borderRadius: BorderRadius.circular(12),
                    border: Border.all(
                      color: isSelected 
                          ? Theme.of(context).colorScheme.primary
                          : Colors.transparent,
                      width: 2,
                    ),
                  ),
                  child: Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Icon(
                        effect['icon'],
                        size: 24,
                        color: isSelected 
                            ? Theme.of(context).colorScheme.primary
                            : Colors.grey,
                      ),
                      const SizedBox(height: 4),
                      Text(
                        '$index',
                        style: TextStyle(
                          fontSize: 10,
                          color: isSelected ? Colors.white : Colors.grey,
                        ),
                      ),
                    ],
                  ),
                ),
              );
            },
          ),
        ],
      ),
    );
  }

  Widget _buildPlaybackCard() {
    return _GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(Icons.play_circle_outline, color: Theme.of(context).colorScheme.primary),
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
                icon: _isPaused ? Icons.play_arrow : Icons.pause,
                size: 64,
                enabled: _isConnected,
                primary: true,
                onTap: () {
                  setState(() => _isPaused = !_isPaused);
                  if (_isPaused) {
                    _serial.pause();
                    _addLog('→ Pausa');
                  } else {
                    _serial.resume();
                    _addLog('→ Riprendi');
                  }
                  HapticFeedback.mediumImpact();
                },
              ),
              _CircleButton(
                icon: Icons.skip_next,
                size: 56,
                enabled: _isConnected,
                onTap: () {
                  _serial.nextEffect();
                  _addLog('→ Prossimo effetto');
                  HapticFeedback.lightImpact();
                },
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildConsoleCard() {
    return _GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(Icons.terminal, color: Theme.of(context).colorScheme.primary),
              const SizedBox(width: 12),
              const Text(
                'Console',
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
              ),
              const Spacer(),
              IconButton(
                icon: const Icon(Icons.delete_outline, size: 20),
                onPressed: () => setState(() => _consoleLog.clear()),
              ),
            ],
          ),
          const SizedBox(height: 12),
          Container(
            height: 150,
            width: double.infinity,
            padding: const EdgeInsets.all(12),
            decoration: BoxDecoration(
              color: Colors.black.withOpacity(0.3),
              borderRadius: BorderRadius.circular(12),
            ),
            child: _consoleLog.isEmpty
                ? Center(
                    child: Text(
                      'Log vuoto',
                      style: TextStyle(color: Colors.grey[600]),
                    ),
                  )
                : ListView.builder(
                    controller: _consoleScroll,
                    itemCount: _consoleLog.length,
                    itemBuilder: (context, index) {
                      final log = _consoleLog[index];
                      Color color = Colors.grey[400]!;
                      if (log.contains('→')) color = Colors.blue[300]!;
                      if (log.contains('←')) color = Colors.green[300]!;
                      if (log.contains('✗')) color = Colors.red[300]!;
                      if (log.contains('✓')) color = Colors.green[400]!;
                      
                      return Padding(
                        padding: const EdgeInsets.symmetric(vertical: 2),
                        child: Text(
                          log,
                          style: TextStyle(
                            fontFamily: 'monospace',
                            fontSize: 11,
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

  @override
  void dispose() {
    _serial.dispose();
    _consoleScroll.dispose();
    super.dispose();
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Widgets
// ═══════════════════════════════════════════════════════════════════════════

class _GlassCard extends StatelessWidget {
  final Widget child;

  const _GlassCard({required this.child});

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.05),
        borderRadius: BorderRadius.circular(20),
        border: Border.all(
          color: Colors.white.withOpacity(0.1),
        ),
      ),
      child: child,
    );
  }
}

class _ActionButton extends StatelessWidget {
  final IconData icon;
  final String label;
  final VoidCallback onTap;
  final Color? color;
  final bool enabled;

  const _ActionButton({
    required this.icon,
    required this.label,
    required this.onTap,
    this.color,
    this.enabled = true,
  });

  @override
  Widget build(BuildContext context) {
    final buttonColor = color ?? Theme.of(context).colorScheme.primary;
    
    return GestureDetector(
      onTap: enabled ? onTap : null,
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 200),
        padding: const EdgeInsets.symmetric(vertical: 14, horizontal: 16),
        decoration: BoxDecoration(
          color: enabled 
              ? buttonColor.withOpacity(0.15)
              : Colors.grey.withOpacity(0.1),
          borderRadius: BorderRadius.circular(12),
          border: Border.all(
            color: enabled 
                ? buttonColor.withOpacity(0.3)
                : Colors.grey.withOpacity(0.2),
          ),
        ),
        child: Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(
              icon,
              size: 18,
              color: enabled ? buttonColor : Colors.grey,
            ),
            const SizedBox(width: 8),
            Text(
              label,
              style: TextStyle(
                color: enabled ? buttonColor : Colors.grey,
                fontWeight: FontWeight.w500,
                fontSize: 13,
              ),
            ),
          ],
        ),
      ),
    );
  }
}

class _CircleButton extends StatelessWidget {
  final IconData icon;
  final double size;
  final VoidCallback onTap;
  final bool enabled;
  final bool primary;

  const _CircleButton({
    required this.icon,
    required this.size,
    required this.onTap,
    this.enabled = true,
    this.primary = false,
  });

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: enabled ? onTap : null,
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 200),
        width: size,
        height: size,
        decoration: BoxDecoration(
          shape: BoxShape.circle,
          gradient: enabled && primary
              ? LinearGradient(
                  colors: [
                    Theme.of(context).colorScheme.primary,
                    Theme.of(context).colorScheme.secondary,
                  ],
                  begin: Alignment.topLeft,
                  end: Alignment.bottomRight,
                )
              : null,
          color: enabled && !primary
              ? Theme.of(context).colorScheme.primary.withOpacity(0.2)
              : !enabled
                  ? Colors.grey.withOpacity(0.2)
                  : null,
        ),
        child: Icon(
          icon,
          size: size * 0.5,
          color: enabled ? Colors.white : Colors.grey,
        ),
      ),
    );
  }
}

class _ModeChip extends StatelessWidget {
  final String label;
  final bool selected;
  final VoidCallback onTap;

  const _ModeChip({
    required this.label,
    required this.selected,
    required this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: onTap,
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 200),
        padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
        decoration: BoxDecoration(
          color: selected 
              ? Theme.of(context).colorScheme.primary
              : Colors.transparent,
          borderRadius: BorderRadius.circular(16),
        ),
        child: Text(
          label,
          style: TextStyle(
            fontSize: 12,
            fontWeight: selected ? FontWeight.w600 : FontWeight.normal,
            color: selected ? Colors.white : Colors.grey,
          ),
        ),
      ),
    );
  }
}

class _DeviceSelector extends StatelessWidget {
  final List<SerialDevice> devices;

  const _DeviceSelector({required this.devices});

  @override
  Widget build(BuildContext context) {
    return Container(
      margin: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Theme.of(context).scaffoldBackgroundColor,
        borderRadius: BorderRadius.circular(24),
      ),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          const SizedBox(height: 12),
          Container(
            width: 40,
            height: 4,
            decoration: BoxDecoration(
              color: Colors.grey[600],
              borderRadius: BorderRadius.circular(2),
            ),
          ),
          const SizedBox(height: 20),
          const Text(
            'Seleziona Porta Seriale',
            style: TextStyle(
              fontSize: 20,
              fontWeight: FontWeight.bold,
            ),
          ),
          const SizedBox(height: 16),
          ...devices.map((device) => ListTile(
            leading: Container(
              padding: const EdgeInsets.all(10),
              decoration: BoxDecoration(
                color: Theme.of(context).colorScheme.primary.withOpacity(0.2),
                borderRadius: BorderRadius.circular(12),
              ),
              child: Icon(
                Icons.usb,
                color: Theme.of(context).colorScheme.primary,
              ),
            ),
            title: Text(device.port),
            subtitle: Text(device.description ?? device.manufacturer ?? 'Serial Port'),
            trailing: const Icon(Icons.chevron_right),
            onTap: () => Navigator.pop(context, device),
          )),
          const SizedBox(height: 16),
        ],
      ),
    );
  }
}
