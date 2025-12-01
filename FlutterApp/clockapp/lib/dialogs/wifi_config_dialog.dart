import 'package:flutter/material.dart';
import '../utils/wifi_scanner.dart';
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

  List<WiFiNetwork> _networks = [];
  bool _scanning = false;

  @override
  void initState() {
    super.initState();
    ssidController = TextEditingController(text: widget.currentSsid ?? '');
    passwordController = TextEditingController();
    apMode = widget.currentApMode;

    _scanWiFi();
  }

  @override
  void dispose() {
    ssidController.dispose();
    passwordController.dispose();
    super.dispose();
  }

  Future<void> _scanWiFi() async {
    setState(() => _scanning = true);
    _networks = await WiFiScanner.scan();
    if (mounted) {
      setState(() => _scanning = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Dialog(
      backgroundColor: const Color(0xFF1a1a2e),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
      child: ConstrainedBox(
        constraints: const BoxConstraints(maxWidth: 500, maxHeight: 600),
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
                    onSelectionChanged: (set) => setState(() => apMode = set.first),
                  ),
                ],
              ),
              const SizedBox(height: 24),

              if (!apMode) ...[
                // Scansione reti
                Row(
                  children: [
                    const Text(
                      'Reti disponibili:',
                      style: TextStyle(fontWeight: FontWeight.w500),
                    ),
                    const Spacer(),
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
                  child: _networks.isEmpty && !_scanning
                      ? Center(
                          child: Padding(
                            padding: const EdgeInsets.all(16),
                            child: Text(
                              'Nessuna rete trovata',
                              style: TextStyle(color: Colors.grey[400]),
                            ),
                          ),
                        )
                      : ListView.builder(
                          shrinkWrap: true,
                          itemCount: _networks.length,
                          itemBuilder: (context, index) {
                            final network = _networks[index];
                            return ListTile(
                              dense: true,
                              leading: Icon(
                                network.isSecured
                                    ? Icons.lock
                                    : Icons.lock_open,
                                size: 20,
                              ),
                              title: Text(network.ssid),
                              trailing: SignalIndicator(
                                strength: network.signalStrength,
                              ),
                              onTap: () {
                                ssidController.text = network.ssid;
                              },
                            );
                          },
                        ),
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
                    hintText: 'Nome rete WiFi',
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
}
