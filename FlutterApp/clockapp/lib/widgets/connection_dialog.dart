import 'package:flutter/material.dart';
import '../services/device_service.dart';

class ConnectionDialog extends StatefulWidget {
  const ConnectionDialog({super.key});

  @override
  State<ConnectionDialog> createState() => _ConnectionDialogState();
}

class _ConnectionDialogState extends State<ConnectionDialog> with SingleTickerProviderStateMixin {
  late TabController _tabController;
  final _deviceService = DeviceService();
  
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
      _serialDevices = _deviceService.getSerialDevices();
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

    final success = await _deviceService.connectSerial(_selectedDevice!);
    
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

    final success = await _deviceService.connectWebSocket(host, port: port);
    
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
                children: [
                  _buildSerialTab(),
                  _buildWebSocketTab(),
                ],
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
                    const Icon(Icons.error_outline, color: Colors.red, size: 20),
                    const SizedBox(width: 8),
                    Expanded(
                      child: Text(_error!, style: const TextStyle(color: Colors.red)),
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
                onPressed: _connecting ? null : () {
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
                        style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
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
            const Text('Porta seriale:', style: TextStyle(fontWeight: FontWeight.w500)),
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
                          style: TextStyle(fontSize: 12, color: Colors.grey[400]),
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
