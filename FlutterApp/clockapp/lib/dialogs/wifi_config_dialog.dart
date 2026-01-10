import 'dart:async';
import 'package:flutter/material.dart';
import '../services/device_service.dart';
import '../widgets/common/signal_indicator.dart';

/// Dialog per configurare WiFi ESP32
class WiFiConfigDialog extends StatefulWidget {
  final String? currentSsid;
  final bool currentApMode;
  final String? deviceName;

  const WiFiConfigDialog({
    super.key,
    this.currentSsid,
    this.currentApMode = false,
    this.deviceName,
  });

  @override
  State<WiFiConfigDialog> createState() => _WiFiConfigDialogState();
}

class _WiFiConfigDialogState extends State<WiFiConfigDialog> {
  late TextEditingController ssidController;
  late TextEditingController passwordController;
  late bool apMode;

  List<ScannedWiFiNetwork> _networks = [];
  bool _scanning = false;
  String _scanStatus = '';
  int _retryCount = 0;
  static const int _maxRetries = 8;  // Aumentato per dare più tempo durante riconnessione

  StreamSubscription? _wifiScanSubscription;
  StreamSubscription? _rawDataSubscription;
  StreamSubscription? _connectionSubscription;
  Timer? _retryTimer;

  final _deviceService = DeviceService();

  @override
  void initState() {
    super.initState();
    ssidController = TextEditingController(text: widget.currentSsid ?? '');
    passwordController = TextEditingController();
    apMode = widget.currentApMode;

    // Ascolta i risultati dello scan WiFi
    _wifiScanSubscription = _deviceService.wifiScanStream.listen((networks) {
      if (mounted) {
        _retryTimer?.cancel();
        setState(() {
          _networks = networks;
          _scanning = false;
          _scanStatus = 'Trovate ${networks.length} reti';
          _retryCount = 0;
        });
      }
    });

    // Ascolta anche i messaggi raw per gestire WIFI_SCAN_STARTED/RUNNING
    _rawDataSubscription = _deviceService.rawDataStream.listen((data) {
      if (!mounted) return;

      if (data == 'WIFI_SCAN_STARTED') {
        setState(() {
          _scanStatus = 'Scan avviato...';
        });
        _scheduleRetry();
      } else if (data == 'WIFI_SCAN_RUNNING') {
        setState(() {
          _scanStatus = 'Scan in corso...';
        });
        _scheduleRetry();
      }
    });

    // Ascolta lo stato della connessione per mostrare riconnessione
    _connectionSubscription = _deviceService.connectionState.listen((state) {
      if (!mounted) return;

      if (_scanning && _deviceService.isWifiScanning) {
        if (state == DeviceConnectionState.connecting) {
          setState(() {
            _scanStatus = 'Riconnessione in corso...';
          });
        } else if (state == DeviceConnectionState.connected) {
          setState(() {
            _scanStatus = 'Riconnesso, attendo risultati...';
          });
          // Richiedi di nuovo lo stato dello scan
          _scheduleRetry();
        }
      }
    });

    // Avvia scan automatico dopo un piccolo delay
    Future.delayed(const Duration(milliseconds: 500), _scanWiFi);
  }

  @override
  void dispose() {
    _retryTimer?.cancel();
    _wifiScanSubscription?.cancel();
    _rawDataSubscription?.cancel();
    _connectionSubscription?.cancel();
    ssidController.dispose();
    passwordController.dispose();
    super.dispose();
  }

  void _scheduleRetry() {
    _retryTimer?.cancel();

    if (_retryCount >= _maxRetries) {
      if (mounted) {
        setState(() {
          _scanning = false;
          _scanStatus = 'Timeout - riprova';
        });
      }
      return;
    }

    // Prima attesa più lunga (3s) per dare tempo allo scan di completarsi
    // Attese successive più brevi (2s) - aumentato per dare tempo alla riconnessione
    final delay = _retryCount == 0
        ? const Duration(seconds: 3)
        : const Duration(seconds: 2);

    _retryTimer = Timer(delay, () {
      if (mounted && _scanning) {
        _retryCount++;

        // Se siamo in modalità WiFi scan ma non connessi, aspetta la riconnessione
        if (_deviceService.isWifiScanning && !_deviceService.isConnected) {
          setState(() {
            _scanStatus = 'Attendo riconnessione... (${_retryCount}/$_maxRetries)';
          });
          _scheduleRetry();  // Riprova
          return;
        }

        if (_deviceService.isConnected) {
          setState(() {
            _scanStatus = 'Attendo risultati... (${_retryCount}/$_maxRetries)';
          });
          _deviceService.requestWiFiScan();
        } else {
          setState(() {
            _scanStatus = 'Connessione persa - riprova';
            _scanning = false;
          });
        }
      }
    });
  }

  void _scanWiFi() {
    if (!_deviceService.isConnected) {
      setState(() {
        _networks = [];
        _scanning = false;
        _scanStatus = 'Non connesso al dispositivo';
      });
      return;
    }

    setState(() {
      _scanning = true;
      _scanStatus = 'Avvio scansione...';
      _retryCount = 0;
    });

    // Richiedi scan all'ESP32
    _deviceService.requestWiFiScan();
  }

  @override
  Widget build(BuildContext context) {
    return Dialog(
      backgroundColor: const Color(0xFF1a1a2e),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
      child: ConstrainedBox(
        constraints: const BoxConstraints(maxWidth: 500, maxHeight: 650),
        child: SingleChildScrollView(
          padding: const EdgeInsets.all(24),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              // Header
              Row(
                children: [
                  const Icon(Icons.wifi, color: Color(0xFF8B5CF6)),
                  const SizedBox(width: 12),
                  const Text(
                    'Configura WiFi ESP32',
                    style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
                  ),
                  const Spacer(),
                  IconButton(
                    icon: const Icon(Icons.close),
                    onPressed: () => Navigator.of(context).pop(),
                  ),
                ],
              ),
              const SizedBox(height: 24),

              // Modalità
              Row(
                children: [
                  const Text(
                    'Modalità:',
                    style: TextStyle(fontWeight: FontWeight.w500),
                  ),
                  const Spacer(),
                  SegmentedButton<bool>(
                    segments: const [
                      ButtonSegment(
                        value: false,
                        label: Text('Station'),
                        icon: Icon(Icons.wifi, size: 16),
                      ),
                      ButtonSegment(
                        value: true,
                        label: Text('AP'),
                        icon: Icon(Icons.router, size: 16),
                      ),
                    ],
                    selected: {apMode},
                    onSelectionChanged: (set) =>
                        setState(() => apMode = set.first),
                  ),
                ],
              ),
              const SizedBox(height: 24),

              if (!apMode) ...[
                // Scansione reti
                Row(
                  children: [
                    Expanded(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          const Text(
                            'Reti rilevate da ESP32:',
                            style: TextStyle(fontWeight: FontWeight.w500),
                          ),
                          if (_scanStatus.isNotEmpty)
                            Padding(
                              padding: const EdgeInsets.only(top: 4),
                              child: Text(
                                _scanStatus,
                                style: TextStyle(
                                  fontSize: 11,
                                  color: Colors.grey[500],
                                ),
                              ),
                            ),
                        ],
                      ),
                    ),
                    IconButton(
                      icon: _scanning
                          ? const SizedBox(
                              width: 20,
                              height: 20,
                              child: CircularProgressIndicator(strokeWidth: 2),
                            )
                          : const Icon(Icons.refresh, size: 20),
                      onPressed: _scanning ? null : _scanWiFi,
                      tooltip: 'Aggiorna lista',
                    ),
                  ],
                ),
                const SizedBox(height: 12),

                // Lista reti
                Container(
                  constraints: const BoxConstraints(maxHeight: 200),
                  decoration: BoxDecoration(
                    border: Border.all(color: Colors.grey.withOpacity(0.3)),
                    borderRadius: BorderRadius.circular(8),
                  ),
                  child: _buildNetworksList(),
                ),
                const SizedBox(height: 16),

                // Input SSID manuale
                const Text(
                  'SSID:',
                  style: TextStyle(fontWeight: FontWeight.w500),
                ),
                const SizedBox(height: 8),
                TextField(
                  controller: ssidController,
                  decoration: InputDecoration(
                    hintText: 'Nome rete WiFi (o seleziona sopra)',
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
                        'SSID: ${widget.deviceName ?? "ledmatrix"}\n'
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
                  onPressed: () => Navigator.of(context).pop({
                    'ssid': ssidController.text,
                    'password': passwordController.text,
                    'apMode': apMode,
                  }),
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
      ),
    );
  }

  Widget _buildNetworksList() {
    if (_scanning && _networks.isEmpty) {
      return const Center(
        child: Padding(
          padding: EdgeInsets.all(24),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              CircularProgressIndicator(strokeWidth: 2),
              SizedBox(height: 12),
              Text('Scansione reti WiFi...'),
            ],
          ),
        ),
      );
    }

    if (_networks.isEmpty) {
      return Center(
        child: Padding(
          padding: const EdgeInsets.all(16),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Icon(Icons.wifi_off, color: Colors.grey[400], size: 32),
              const SizedBox(height: 8),
              Text(
                _deviceService.isConnected
                    ? 'Nessuna rete trovata'
                    : 'Non connesso',
                style: TextStyle(color: Colors.grey[400]),
              ),
              const SizedBox(height: 4),
              Text(
                'Premi refresh per riprovare',
                style: TextStyle(color: Colors.grey[600], fontSize: 12),
              ),
            ],
          ),
        ),
      );
    }

    return ListView.builder(
      shrinkWrap: true,
      itemCount: _networks.length,
      itemBuilder: (context, index) {
        final network = _networks[index];
        final isSelected = ssidController.text == network.ssid;

        return ListTile(
          dense: true,
          selected: isSelected,
          selectedTileColor: const Color(0xFF8B5CF6).withOpacity(0.2),
          leading: Icon(
            network.isSecured ? Icons.lock : Icons.lock_open,
            size: 20,
            color: isSelected ? const Color(0xFF8B5CF6) : null,
          ),
          title: Text(
            network.ssid,
            style: TextStyle(
              fontWeight: isSelected ? FontWeight.bold : FontWeight.normal,
            ),
          ),
          subtitle: Text(
            '${network.rssi} dBm',
            style: TextStyle(fontSize: 11, color: Colors.grey[500]),
          ),
          trailing: SignalIndicator(
            strength: network.signalStrength,
          ),
          onTap: () {
            setState(() {
              ssidController.text = network.ssid;
            });
          },
        );
      },
    );
  }
}
