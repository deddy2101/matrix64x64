import 'package:flutter/material.dart';
import 'dart:io';
import '../services/discovery_service.dart';
import '../services/device_service.dart';
import 'home_screen.dart';

class DeviceDiscoveryScreen extends StatefulWidget {
  const DeviceDiscoveryScreen({super.key});

  @override
  State<DeviceDiscoveryScreen> createState() => _DeviceDiscoveryScreenState();
}

class _DeviceDiscoveryScreenState extends State<DeviceDiscoveryScreen> {
  final _discoveryService = DiscoveryService();
  final _deviceService = DeviceService();

  List<DiscoveredDevice> _discoveredDevices = [];
  List<SerialDevice> _serialDevices = [];
  bool _isScanning = false;
  bool _showSerialFallback = false;
  String? _error;

  @override
  void initState() {
    super.initState();

    // Ascolta dispositivi WiFi trovati
    _discoveryService.devicesStream.listen((devices) {
      if (mounted) {
        setState(() {
          _discoveredDevices = devices;

          // Su desktop, se dopo 3 secondi non trova nulla via WiFi, mostra seriale
          if (_isDesktop && devices.isEmpty && _isScanning) {
            Future.delayed(const Duration(seconds: 3), () {
              if (mounted && _discoveredDevices.isEmpty && _isScanning) {
                _scanSerialDevices();
              }
            });
          }
        });
      }
    });

    // Avvia scansione automatica
    _startScan();
  }

  @override
  void dispose() {
    _discoveryService.dispose();
    super.dispose();
  }

  bool get _isDesktop {
    try {
      return Platform.isLinux || Platform.isMacOS || Platform.isWindows;
    } catch (_) {
      return false;
    }
  }

  Future<void> _startScan() async {
    setState(() {
      _isScanning = true;
      _error = null;
      _showSerialFallback = false;
    });

    try {
      // Scansione WiFi/Network
      await _discoveryService.scan();

      // Su desktop, se non trova nulla, cerca dispositivi seriali
      if (_isDesktop && _discoveredDevices.isEmpty) {
        await Future.delayed(const Duration(milliseconds: 500));
        _scanSerialDevices();
      }
    } catch (e) {
      setState(() => _error = 'Errore durante la scansione: $e');
    } finally {
      if (mounted) {
        setState(() => _isScanning = false);
      }
    }
  }

  void _scanSerialDevices() {
    if (!_isDesktop) return;

    try {
      _serialDevices = _deviceService.getSerialDevices();
      if (_serialDevices.isNotEmpty) {
        setState(() => _showSerialFallback = true);
      }
    } catch (e) {
      print('Error scanning serial: $e');
    }
  }

  Future<void> _connectToDevice(DiscoveredDevice device) async {
    setState(() {
      _error = null;
    });

    // Mostra loading
    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (context) => const Center(
        child: Card(
          child: Padding(
            padding: EdgeInsets.all(24),
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                CircularProgressIndicator(),
                SizedBox(height: 16),
                Text('Connessione in corso...'),
              ],
            ),
          ),
        ),
      ),
    );

    final success = await _deviceService.connectWebSocket(
      device.ip,
      port: device.port,
    );

    if (mounted) {
      Navigator.of(context).pop(); // Chiudi loading dialog

      if (success) {
        // Torna alla home con successo
        Navigator.of(context).pushReplacement(
          MaterialPageRoute(builder: (_) => const HomeScreen()),
        );
      } else {
        setState(() {
          _error = 'Impossibile connettersi a ${device.name} (${device.ip})';
        });
      }
    }
  }

  Future<void> _connectToSerial(SerialDevice device) async {
    setState(() {
      _error = null;
    });

    // Mostra loading
    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (context) => const Center(
        child: Card(
          child: Padding(
            padding: EdgeInsets.all(24),
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                CircularProgressIndicator(),
                SizedBox(height: 16),
                Text('Connessione in corso...'),
              ],
            ),
          ),
        ),
      ),
    );

    final success = await _deviceService.connectSerial(device);

    if (mounted) {
      Navigator.of(context).pop(); // Chiudi loading dialog

      if (success) {
        // Torna alla home con successo
        Navigator.of(context).pushReplacement(
          MaterialPageRoute(builder: (_) => const HomeScreen()),
        );
      } else {
        setState(() {
          _error = 'Impossibile connettersi a ${device.name}';
        });
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFF0A0A0F),
      appBar: AppBar(
        backgroundColor: const Color(0xFF121218),
        title: const Text('Trova Dispositivi'),
        actions: [
          IconButton(
            icon: _isScanning
                ? const SizedBox(
                    width: 20,
                    height: 20,
                    child: CircularProgressIndicator(strokeWidth: 2),
                  )
                : const Icon(Icons.refresh),
            onPressed: _isScanning ? null : _startScan,
            tooltip: 'Aggiorna',
          ),
        ],
      ),
      body: _buildBody(),
    );
  }

  Widget _buildBody() {
    // Mostra dispositivi trovati via WiFi
    if (_discoveredDevices.isNotEmpty) {
      return _buildDeviceList();
    }

    // Mostra seriale su desktop se disponibile
    if (_showSerialFallback && _serialDevices.isNotEmpty) {
      return _buildSerialList();
    }

    // Mostra stato scanning o empty
    if (_isScanning) {
      return _buildScanningState();
    }

    return _buildEmptyState();
  }

  Widget _buildDeviceList() {
    return Column(
      children: [
        // Header
        Container(
          width: double.infinity,
          padding: const EdgeInsets.all(16),
          decoration: BoxDecoration(
            color: const Color(0xFF121218),
            border: Border(
              bottom: BorderSide(color: Colors.grey.withOpacity(0.2)),
            ),
          ),
          child: Row(
            children: [
              const Icon(Icons.router, color: Color(0xFF8B5CF6), size: 20),
              const SizedBox(width: 12),
              Text(
                'Dispositivi trovati (${_discoveredDevices.length})',
                style: const TextStyle(
                  fontSize: 16,
                  fontWeight: FontWeight.w600,
                ),
              ),
            ],
          ),
        ),

        // Lista dispositivi
        Expanded(
          child: ListView.builder(
            padding: const EdgeInsets.all(16),
            itemCount: _discoveredDevices.length,
            itemBuilder: (context, index) {
              final device = _discoveredDevices[index];
              return _buildDeviceCard(device);
            },
          ),
        ),

        // Error message
        if (_error != null) _buildErrorBanner(),
      ],
    );
  }

  Widget _buildDeviceCard(DiscoveredDevice device) {
    return Card(
      margin: const EdgeInsets.only(bottom: 12),
      color: const Color(0xFF121218),
      elevation: 2,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(12),
        side: BorderSide(color: Colors.grey.withOpacity(0.2)),
      ),
      child: InkWell(
        onTap: () => _connectToDevice(device),
        borderRadius: BorderRadius.circular(12),
        child: Padding(
          padding: const EdgeInsets.all(16),
          child: Row(
            children: [
              // Icona
              Container(
                width: 56,
                height: 56,
                decoration: BoxDecoration(
                  color: const Color(0xFF8B5CF6).withOpacity(0.2),
                  borderRadius: BorderRadius.circular(12),
                ),
                child: const Icon(
                  Icons.grid_on,
                  color: Color(0xFF8B5CF6),
                  size: 28,
                ),
              ),
              const SizedBox(width: 16),

              // Info
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      device.name,
                      style: const TextStyle(
                        fontSize: 18,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    const SizedBox(height: 4),
                    Text(
                      '${device.ip}:${device.port}',
                      style: TextStyle(color: Colors.grey[400], fontSize: 14),
                    ),
                    const SizedBox(height: 4),
                    Row(
                      children: [
                        Icon(Icons.wifi, size: 14, color: Colors.grey[500]),
                        const SizedBox(width: 4),
                        Text(
                          'Trovato ${_formatTime(device.discoveredAt)}',
                          style: TextStyle(
                            color: Colors.grey[500],
                            fontSize: 12,
                          ),
                        ),
                      ],
                    ),
                  ],
                ),
              ),

              // Freccia
              const Icon(
                Icons.arrow_forward_ios,
                size: 20,
                color: Color(0xFF8B5CF6),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildSerialList() {
    return Column(
      children: [
        // Header
        Container(
          width: double.infinity,
          padding: const EdgeInsets.all(16),
          decoration: BoxDecoration(
            color: const Color(0xFF121218),
            border: Border(
              bottom: BorderSide(color: Colors.grey.withOpacity(0.2)),
            ),
          ),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const Row(
                children: [
                  Icon(Icons.usb, color: Color(0xFF8B5CF6), size: 20),
                  SizedBox(width: 12),
                  Text(
                    'Dispositivi Seriali',
                    style: TextStyle(fontSize: 16, fontWeight: FontWeight.w600),
                  ),
                ],
              ),
              const SizedBox(height: 8),
              Text(
                'Nessun dispositivo trovato via WiFi',
                style: TextStyle(fontSize: 12, color: Colors.grey[400]),
              ),
            ],
          ),
        ),

        // Lista seriali
        Expanded(
          child: ListView.builder(
            padding: const EdgeInsets.all(16),
            itemCount: _serialDevices.length,
            itemBuilder: (context, index) {
              final device = _serialDevices[index];
              return _buildSerialCard(device);
            },
          ),
        ),

        // Error message
        if (_error != null) _buildErrorBanner(),
      ],
    );
  }

  Widget _buildSerialCard(SerialDevice device) {
    return Card(
      margin: const EdgeInsets.only(bottom: 12),
      color: const Color(0xFF121218),
      elevation: 2,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(12),
        side: BorderSide(color: Colors.grey.withOpacity(0.2)),
      ),
      child: InkWell(
        onTap: () => _connectToSerial(device),
        borderRadius: BorderRadius.circular(12),
        child: Padding(
          padding: const EdgeInsets.all(16),
          child: Row(
            children: [
              // Icona
              Container(
                width: 56,
                height: 56,
                decoration: BoxDecoration(
                  color: Colors.orange.withOpacity(0.2),
                  borderRadius: BorderRadius.circular(12),
                ),
                child: const Icon(Icons.usb, color: Colors.orange, size: 28),
              ),
              const SizedBox(width: 16),

              // Info
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      device.displayName,
                      style: const TextStyle(
                        fontSize: 18,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    const SizedBox(height: 4),
                    Text(
                      device.port,
                      style: TextStyle(color: Colors.grey[400], fontSize: 14),
                    ),
                  ],
                ),
              ),

              // Freccia
              const Icon(
                Icons.arrow_forward_ios,
                size: 20,
                color: Colors.orange,
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildScanningState() {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          // Animazione
          TweenAnimationBuilder(
            tween: Tween<double>(begin: 0, end: 1),
            duration: const Duration(seconds: 2),
            builder: (context, double value, child) {
              return Transform.scale(
                scale: 0.8 + (value * 0.2),
                child: Container(
                  width: 120,
                  height: 120,
                  decoration: BoxDecoration(
                    shape: BoxShape.circle,
                    color: const Color(0xFF8B5CF6).withOpacity(0.1),
                  ),
                  child: const Icon(
                    Icons.search,
                    size: 60,
                    color: Color(0xFF8B5CF6),
                  ),
                ),
              );
            },
          ),
          const SizedBox(height: 32),
          const CircularProgressIndicator(color: Color(0xFF8B5CF6)),
          const SizedBox(height: 24),
          const Text(
            'Ricerca dispositivi...',
            style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
          ),
          const SizedBox(height: 8),
          Text(
            'Scansione delle reti WiFi ed Ethernet',
            style: TextStyle(fontSize: 14, color: Colors.grey[400]),
          ),
        ],
      ),
    );
  }

  Widget _buildEmptyState() {
    return Center(
      child: Padding(
        padding: const EdgeInsets.all(32),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(Icons.devices_other, size: 80, color: Colors.grey[700]),
            const SizedBox(height: 24),
            const Text(
              'Nessun dispositivo trovato',
              style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 12),
            Text(
              'Assicurati che:',
              style: TextStyle(fontSize: 14, color: Colors.grey[400]),
            ),
            const SizedBox(height: 12),
            _buildCheckItem('• L\'ESP32 sia acceso e connesso al WiFi'),
            _buildCheckItem('• Il dispositivo sia sulla stessa rete'),
            if (_isDesktop)
              _buildCheckItem('• Se via USB, controlla la porta seriale'),
            const SizedBox(height: 32),
            ElevatedButton.icon(
              onPressed: _startScan,
              icon: const Icon(Icons.refresh),
              label: const Text('Riprova'),
              style: ElevatedButton.styleFrom(
                backgroundColor: const Color(0xFF8B5CF6),
                padding: const EdgeInsets.symmetric(
                  horizontal: 32,
                  vertical: 16,
                ),
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(12),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildCheckItem(String text) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4),
      child: Text(
        text,
        style: TextStyle(fontSize: 13, color: Colors.grey[500]),
      ),
    );
  }

  Widget _buildErrorBanner() {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Colors.red.withOpacity(0.1),
        border: Border(top: BorderSide(color: Colors.red.withOpacity(0.3))),
      ),
      child: Row(
        children: [
          const Icon(Icons.error_outline, color: Colors.red, size: 20),
          const SizedBox(width: 12),
          Expanded(
            child: Text(_error!, style: const TextStyle(color: Colors.red)),
          ),
          IconButton(
            icon: const Icon(Icons.close, size: 20, color: Colors.red),
            onPressed: () => setState(() => _error = null),
          ),
        ],
      ),
    );
  }

  String _formatTime(DateTime time) {
    final now = DateTime.now();
    final diff = now.difference(time);

    if (diff.inSeconds < 10) return 'ora';
    if (diff.inSeconds < 60) return '${diff.inSeconds}s fa';
    if (diff.inMinutes < 60) return '${diff.inMinutes}m fa';
    return '${time.hour}:${time.minute.toString().padLeft(2, '0')}';
  }
}
