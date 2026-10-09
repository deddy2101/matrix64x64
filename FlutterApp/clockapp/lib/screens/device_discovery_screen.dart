import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'dart:async';
import 'dart:io';
import '../services/ble_link.dart';
import '../services/discovery_service.dart';
import '../services/device_service.dart';
import '../services/permission_service.dart';
import '../services/demo_service.dart';
import '../widgets/discovery/discovery_widgets.dart';
import 'home_screen.dart';
import 'demo_home_screen.dart';
import 'faq_screen.dart';

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

  // Bluetooth LE
  final Map<String, BleScanResult> _bleDevices = {};
  StreamSubscription<BleScanResult>? _bleSub;
  Timer? _bleStopTimer;
  bool _bleScanning = false;
  String? _bleError;
  static const Duration _bleScanDuration = Duration(seconds: 20);

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

    // Dispositivi Bluetooth trovati
    _bleSub = _deviceService.ble.scanResults.listen((device) {
      if (mounted) setState(() => _bleDevices[device.id] = device);
    });

    // Richiedi permessi e avvia scansione con protezione da errori
    WidgetsBinding.instance.addPostFrameCallback((_) {
      _startBleScan();
      _requestPermissionsAndScan();
    });
  }

  /// Scansione Bluetooth (principale). Si ferma da sola dopo qualche secondo
  /// per non consumare batteria.
  Future<void> _startBleScan() async {
    _bleStopTimer?.cancel();
    setState(() {
      _bleDevices.clear();
      _bleError = null;
      _bleScanning = true;
    });

    final ok = await _deviceService.ble.startScan();
    if (!mounted) return;

    if (!ok) {
      setState(() {
        _bleScanning = false;
        _bleError = _deviceService.ble.lastError;
      });
      return;
    }

    _bleStopTimer = Timer(_bleScanDuration, _stopBleScan);
  }

  Future<void> _stopBleScan() async {
    _bleStopTimer?.cancel();
    await _deviceService.ble.stopScan();
    if (mounted) setState(() => _bleScanning = false);
  }

  /// Richiede permessi Android (se necessario) e avvia scansione
  Future<void> _requestPermissionsAndScan() async {
    // Su Android, richiedi permessi prima di scansionare
    if (Platform.isAndroid) {
      final hasPermission =
          await PermissionService.requestDiscoveryPermissions();

      if (!hasPermission) {
        if (mounted) {
          setState(() {
            _error = 'Permessi necessari per trovare dispositivi';
          });

          // Mostra dialog per aprire impostazioni
          _showPermissionDialog();
        }
        return;
      }
    }

    // Permessi OK (o non necessari), avvia scansione
    _startScan();
  }

  /// Dialog per richiedere permessi
  Future<void> _showPermissionDialog() async {
    final result = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        backgroundColor: const Color(0xFF1a1a2e),
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
        title: const Row(
          children: [
            Icon(Icons.warning, color: Colors.orange),
            SizedBox(width: 12),
            Text('Permessi Necessari'),
          ],
        ),
        content: Text(PermissionService.getPermissionMessage()),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context, false),
            child: const Text('Annulla'),
          ),
          ElevatedButton(
            onPressed: () => Navigator.pop(context, true),
            style: ElevatedButton.styleFrom(
              backgroundColor: const Color(0xFF8B5CF6),
            ),
            child: const Text('Apri Impostazioni'),
          ),
        ],
      ),
    );

    if (result == true) {
      await PermissionService.openSettings();
      // Dopo che l'utente torna dalle impostazioni, riprova
      if (mounted && await PermissionService.hasDiscoveryPermissions()) {
        _startScan();
      }
    }
  }

  @override
  void dispose() {
    _bleStopTimer?.cancel();
    _bleSub?.cancel();
    _deviceService.ble.stopScan();
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

  /// Connetti direttamente all'IP di default AP (192.168.4.1)
  Future<void> _connectToDefaultAP() async {
    final device = DiscoveredDevice(
      name: 'ESP32 (AP Mode)',
      ip: '192.168.4.1',
      port: 80,
    );
    await _connectToDevice(device);
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
        // Vai alla home con push (non pushReplacement) così può tornare indietro con pop
        Navigator.of(context).push(
          MaterialPageRoute(builder: (_) => const HomeScreen()),
        );
      } else {
        setState(() {
          _error = 'Impossibile connettersi a ${device.name} (${device.ip})';
        });
      }
    }
  }

  Future<void> _connectToBle(BleScanResult device) async {
    setState(() {
      _error = null;
    });

    // Il primo collegamento richiede il PIN mostrato sul display
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
                SizedBox(height: 8),
                Text(
                  'Al primo collegamento serviranno due PIN:\nquello di accesso e poi quello che compare\nin alto a destra sul display',
                  textAlign: TextAlign.center,
                  style: TextStyle(fontSize: 12, color: Colors.grey),
                ),
              ],
            ),
          ),
        ),
      ),
    );

    await _stopBleScan();
    final success = await _deviceService.connectBle(
      device.id,
      device.name,
      askPin: _askStaticPin,
      askPasskey: _askDynamicPin,
    );

    if (mounted) {
      Navigator.of(context).pop(); // Chiudi loading dialog

      if (success) {
        Navigator.of(context).push(
          MaterialPageRoute(builder: (_) => const HomeScreen()),
        );
      } else {
        setState(() {
          _error = _deviceService.bleError ??
              'Impossibile connettersi a ${device.name}';
        });
      }
    }
  }

  /// Chiede all'utente il PIN statico di accesso del display
  Future<String?> _askStaticPin(bool retry) async {
    if (!mounted) return null;
    return showDialog<String>(
      context: context,
      barrierDismissible: false,
      builder: (context) => _PinDialog(
        icon: Icons.lock_outline,
        title: 'PIN di accesso',
        message: retry
            ? 'PIN errato, riprova.'
            : 'Inserisci il PIN di accesso del display (quello di '
                'fabbrica o l\'ultimo che hai impostato). Solo dopo '
                'comparirà il PIN da digitare sul telefono.',
        messageColor: retry ? Colors.red[300] : null,
        obscureText: true,
        confirmLabel: 'Continua',
      ),
    );
  }

  /// Chiede il PIN dinamico mostrato dal display (solo Linux: sugli altri
  /// sistemi lo chiede la finestra di pairing del sistema operativo)
  Future<String?> _askDynamicPin() async {
    if (!mounted) return null;
    return showDialog<String>(
      context: context,
      barrierDismissible: false,
      builder: (context) => const _PinDialog(
        icon: Icons.bluetooth_searching,
        title: 'PIN del display',
        message: 'Digita il PIN che compare in alto a destra sul display. '
            'Hai 30 secondi.',
        confirmLabel: 'Associa',
      ),
    );
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
        // Vai alla home con push (non pushReplacement) così può tornare indietro con pop
        Navigator.of(context).push(
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
          // Bottone connessione diretta AP
          IconButton(
            icon: const Icon(Icons.router),
            onPressed: _connectToDefaultAP,
            tooltip: 'Connetti a 192.168.4.1 (AP Mode)',
          ),
          IconButton(
            icon: const Icon(Icons.help_outline),
            onPressed: () => Navigator.push(
              context,
              MaterialPageRoute(builder: (_) => const FaqScreen()),
            ),
            tooltip: 'FAQ',
          ),
          IconButton(
            icon: (_isScanning || _bleScanning)
                ? const SizedBox(
                    width: 20,
                    height: 20,
                    child: CircularProgressIndicator(strokeWidth: 2),
                  )
                : const Icon(Icons.refresh),
            onPressed: (_isScanning || _bleScanning) ? null : _refresh,
            tooltip: 'Aggiorna',
          ),
        ],
      ),
      body: Column(
        children: [
          _buildNoticeBanner(),
          Expanded(child: _buildBody()),
        ],
      ),
    );
  }

  Future<void> _refresh() async {
    _startBleScan();
    _startScan();
  }

  Widget _buildBody() {
    final hasSerial = _showSerialFallback && _serialDevices.isNotEmpty;
    final hasAny = _bleDevices.isNotEmpty ||
        _discoveredDevices.isNotEmpty ||
        hasSerial;

    if (hasAny) {
      return _buildResults(hasSerial);
    }

    // Mostra stato scanning o empty
    if (_isScanning || _bleScanning) {
      return _buildScanningState();
    }

    return _buildEmptyState();
  }

  Widget _sectionHeader(IconData icon, String title, {String? subtitle}) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 12, top: 4),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(icon, color: const Color(0xFF8B5CF6), size: 20),
              const SizedBox(width: 12),
              Text(
                title,
                style: const TextStyle(
                  fontSize: 16,
                  fontWeight: FontWeight.w600,
                ),
              ),
            ],
          ),
          if (subtitle != null) ...[
            const SizedBox(height: 4),
            Text(
              subtitle,
              style: TextStyle(fontSize: 12, color: Colors.grey[400]),
            ),
          ],
        ],
      ),
    );
  }

  Widget _buildResults(bool hasSerial) {
    final bleList = _bleDevices.values.toList()
      ..sort((a, b) => (b.rssi ?? -127).compareTo(a.rssi ?? -127));

    return Column(
      children: [
        Expanded(
          child: ListView(
            padding: const EdgeInsets.all(16),
            children: [
              if (bleList.isNotEmpty) ...[
                _sectionHeader(
                  Icons.bluetooth,
                  'Dispositivi Bluetooth (${bleList.length})',
                ),
                for (final device in bleList)
                  BleDeviceCard(
                    device: device,
                    onTap: () => _connectToBle(device),
                  ),
              ],
              if (_discoveredDevices.isNotEmpty) ...[
                _sectionHeader(
                  Icons.router,
                  'Dispositivi WiFi (${_discoveredDevices.length})',
                  subtitle: 'Firmware precedente',
                ),
                for (final device in _discoveredDevices)
                  DeviceCard(
                    device: device,
                    onTap: () => _connectToDevice(device),
                  ),
              ],
              if (hasSerial) ...[
                _sectionHeader(Icons.usb, 'Dispositivi Seriali'),
                for (final device in _serialDevices)
                  SerialCard(
                    device: device,
                    onTap: () => _connectToSerial(device),
                  ),
              ],
              if (_bleError != null) _buildBleNotice(),
            ],
          ),
        ),

        // Error message
        if (_error != null) _buildErrorBanner(),
      ],
    );
  }

  /// Messaggio persistente (es. dopo la migrazione al firmware Bluetooth)
  Widget _buildNoticeBanner() {
    return ValueListenableBuilder<String?>(
      valueListenable: _deviceService.notice,
      builder: (context, notice, _) {
        if (notice == null) return const SizedBox.shrink();
        return Container(
          width: double.infinity,
          padding: const EdgeInsets.all(16),
          decoration: BoxDecoration(
            color: const Color(0xFF8B5CF6).withOpacity(0.12),
            border: Border(
              bottom: BorderSide(
                color: const Color(0xFF8B5CF6).withOpacity(0.4),
              ),
            ),
          ),
          child: Row(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const Icon(Icons.bluetooth, color: Color(0xFF8B5CF6), size: 20),
              const SizedBox(width: 12),
              Expanded(child: Text(notice, style: const TextStyle(fontSize: 13))),
              IconButton(
                icon: const Icon(Icons.close, size: 20),
                padding: EdgeInsets.zero,
                constraints: const BoxConstraints(),
                onPressed: () => _deviceService.notice.value = null,
              ),
            ],
          ),
        );
      },
    );
  }

  Widget _buildBleNotice() {
    return Padding(
      padding: const EdgeInsets.only(top: 8),
      child: Row(
        children: [
          const Icon(Icons.bluetooth_disabled, color: Colors.orange, size: 18),
          const SizedBox(width: 8),
          Expanded(
            child: Text(
              _bleError!,
              style: const TextStyle(color: Colors.orange, fontSize: 13),
            ),
          ),
        ],
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
            'Scansione Bluetooth e reti WiFi',
            style: TextStyle(fontSize: 14, color: Colors.grey[400]),
          ),
        ],
      ),
    );
  }

  Widget _buildEmptyState() {
    return SingleChildScrollView(
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
            _buildCheckItem('• Il display sia acceso'),
            _buildCheckItem('• Il Bluetooth del telefono sia attivo'),
            _buildCheckItem('• Se ha il firmware precedente, sia sulla stessa rete WiFi'),
            if (_isDesktop)
              _buildCheckItem('• Se via USB, controlla la porta seriale'),
            if (_bleError != null) ...[
              const SizedBox(height: 12),
              _buildBleNotice(),
            ],
            const SizedBox(height: 32),
            ElevatedButton.icon(
              onPressed: _refresh,
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
            const SizedBox(height: 24),
            // Bottone connessione diretta AP
            _buildDirectAPSection(),
            const SizedBox(height: 24),
            // Sezione Demo
            _buildDemoSection(),
          ],
        ),
      ),
    );
  }

  Widget _buildDirectAPSection() {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: const Color(0xFF1a1a2e),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(
          color: Colors.blue.withOpacity(0.3),
        ),
      ),
      child: Column(
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Icon(
                Icons.router,
                color: Colors.blue[400],
                size: 24,
              ),
              const SizedBox(width: 8),
              Text(
                'Connessione Diretta (AP Mode)',
                style: TextStyle(
                  fontSize: 16,
                  fontWeight: FontWeight.w600,
                  color: Colors.blue[400],
                ),
              ),
            ],
          ),
          const SizedBox(height: 12),
          Text(
            'Se sei connesso alla rete WiFi dell\'ESP32,\npremi qui per connetterti direttamente.',
            textAlign: TextAlign.center,
            style: TextStyle(
              fontSize: 13,
              color: Colors.grey[400],
            ),
          ),
          const SizedBox(height: 16),
          ElevatedButton.icon(
            onPressed: _connectToDefaultAP,
            icon: const Icon(Icons.link),
            label: const Text('192.168.4.1'),
            style: ElevatedButton.styleFrom(
              backgroundColor: Colors.blue[700],
              foregroundColor: Colors.white,
              padding: const EdgeInsets.symmetric(
                horizontal: 32,
                vertical: 14,
              ),
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(12),
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildDemoSection() {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: const Color(0xFF1a1a2e),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(
          color: const Color(0xFF8B5CF6).withOpacity(0.3),
        ),
      ),
      child: Column(
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Icon(
                Icons.play_circle_outline,
                color: Colors.orange[400],
                size: 24,
              ),
              const SizedBox(width: 8),
              Text(
                'Modalità Demo',
                style: TextStyle(
                  fontSize: 18,
                  fontWeight: FontWeight.w600,
                  color: Colors.orange[400],
                ),
              ),
            ],
          ),
          const SizedBox(height: 12),
          Text(
            'Non hai un dispositivo LED Matrix?\nProva l\'app con un dispositivo simulato!',
            textAlign: TextAlign.center,
            style: TextStyle(
              fontSize: 14,
              color: Colors.grey[400],
            ),
          ),
          const SizedBox(height: 16),
          ElevatedButton.icon(
            onPressed: _startDemoMode,
            icon: const Icon(Icons.visibility),
            label: const Text('Avvia Demo'),
            style: ElevatedButton.styleFrom(
              backgroundColor: Colors.orange[700],
              foregroundColor: Colors.white,
              padding: const EdgeInsets.symmetric(
                horizontal: 32,
                vertical: 14,
              ),
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(12),
              ),
            ),
          ),
        ],
      ),
    );
  }

  void _startDemoMode() {
    DemoService().startDemo();
    Navigator.of(context).pushReplacement(
      MaterialPageRoute(builder: (_) => const DemoHomeScreen()),
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
}

/// Dialog per l'inserimento di un PIN a 6 cifre. Possiede il proprio
/// controller, così viene liberato solo dopo l'animazione di chiusura.
class _PinDialog extends StatefulWidget {
  final IconData icon;
  final String title;
  final String message;
  final Color? messageColor;
  final bool obscureText;
  final String confirmLabel;

  const _PinDialog({
    required this.icon,
    required this.title,
    required this.message,
    this.messageColor,
    this.obscureText = false,
    required this.confirmLabel,
  });

  @override
  State<_PinDialog> createState() => _PinDialogState();
}

class _PinDialogState extends State<_PinDialog> {
  final _controller = TextEditingController();

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  void _submit() {
    if (_controller.text.length == 6) {
      Navigator.pop(context, _controller.text);
    }
  }

  @override
  Widget build(BuildContext context) {
    return AlertDialog(
      backgroundColor: const Color(0xFF1a1a2e),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
      title: Row(
        children: [
          Icon(widget.icon, color: const Color(0xFF8B5CF6)),
          const SizedBox(width: 12),
          Text(widget.title),
        ],
      ),
      content: Column(
        mainAxisSize: MainAxisSize.min,
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(widget.message, style: TextStyle(color: widget.messageColor)),
          const SizedBox(height: 16),
          TextField(
            controller: _controller,
            autofocus: true,
            keyboardType: TextInputType.number,
            maxLength: 6,
            obscureText: widget.obscureText,
            decoration: const InputDecoration(
              labelText: 'PIN (6 cifre)',
              counterText: '',
            ),
            inputFormatters: [FilteringTextInputFormatter.digitsOnly],
            onSubmitted: (_) => _submit(),
          ),
        ],
      ),
      actions: [
        TextButton(
          onPressed: () => Navigator.pop(context),
          child: const Text('Annulla'),
        ),
        ElevatedButton(
          onPressed: _submit,
          style: ElevatedButton.styleFrom(
            backgroundColor: const Color(0xFF8B5CF6),
          ),
          child: Text(widget.confirmLabel),
        ),
      ],
    );
  }
}
