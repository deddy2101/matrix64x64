import 'dart:io';

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
