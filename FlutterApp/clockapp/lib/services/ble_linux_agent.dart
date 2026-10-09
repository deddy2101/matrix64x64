import 'package:bluez/bluez.dart';
import 'package:dbus/dbus.dart';

/// Agent BlueZ per il pairing su Linux.
///
/// Il display richiede un pairing con passkey (PIN dinamico mostrato sulla
/// matrice). Su Linux è BlueZ a gestirlo, ma solo se c'è un "agent" che chiede
/// il PIN all'utente: universal_ble non ne registra nessuno e GNOME lo fa solo
/// con il pannello Bluetooth aperto. Senza agent BlueZ ripiega su un pairing
/// senza PIN, il display lo rifiuta e chiude la connessione.
///
/// Questo agent viene registrato come predefinito e gira la richiesta di
/// passkey a [onPasskey] (impostato da BleLink durante connect()).
class BleLinuxAgent extends BlueZAgent {
  BleLinuxAgent._();
  static final BleLinuxAgent instance = BleLinuxAgent._();

  /// Chiede all'utente il PIN dinamico; null = annullato
  Future<String?> Function()? onPasskey;

  BlueZClient? _client;
  Future<void>? _registering;

  /// Registra l'agent una volta sola (le chiamate successive non fanno nulla)
  Future<void> ensureRegistered() => _registering ??= _register();

  Future<void> _register() async {
    try {
      final client = BlueZClient();
      await client.connect();
      await client.registerAgent(
        this,
        path: DBusObjectPath('/com/ledmatrix/clockapp/agent'),
        capability: BlueZAgentCapability.keyboardDisplay,
      );
      await client.requestDefaultAgent();
      _client = client;
      print('[BLE] Linux pairing agent registered');
    } catch (e) {
      // Senza agent il primo pairing fallirà, ma i display già associati
      // funzionano lo stesso: riprova al prossimo connect()
      print('[BLE] Linux pairing agent registration failed: $e');
      _registering = null;
    }
  }

  @override
  Future<BlueZAgentPasskeyResponse> requestPasskey(BlueZDevice device) async {
    final ask = onPasskey;
    if (ask == null) return BlueZAgentPasskeyResponse.canceled();

    final text = await ask();
    final passkey = int.tryParse(text ?? '');
    if (passkey == null) return BlueZAgentPasskeyResponse.canceled();
    return BlueZAgentPasskeyResponse.success(passkey);
  }

  @override
  Future<void> release() async {
    _client = null;
    _registering = null;
  }

  Future<void> dispose() async {
    final client = _client;
    _client = null;
    _registering = null;
    if (client == null) return;
    try {
      await client.unregisterAgent();
    } catch (_) {}
    await client.close();
  }
}
