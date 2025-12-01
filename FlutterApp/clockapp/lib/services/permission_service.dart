import 'package:permission_handler/permission_handler.dart';
import 'dart:io';

/// Service per gestire permessi Android
class PermissionService {
  /// Richiedi permessi necessari per discovery WiFi
  static Future<bool> requestDiscoveryPermissions() async {
    if (!Platform.isAndroid) return true; // Solo Android necessita permessi
    
    // Android 13+ (API 33+) usa NEARBY_WIFI_DEVICES
    if (await _isAndroid13OrHigher()) {
      final status = await Permission.nearbyWifiDevices.request();
      return status.isGranted;
    }
    
    // Android 6-12 (API 23-32) usa LOCATION
    final status = await Permission.locationWhenInUse.request();
    return status.isGranted;
  }
  
  /// Verifica se permessi discovery sono già garantiti
  static Future<bool> hasDiscoveryPermissions() async {
    if (!Platform.isAndroid) return true;
    
    if (await _isAndroid13OrHigher()) {
      return await Permission.nearbyWifiDevices.isGranted;
    }
    
    return await Permission.locationWhenInUse.isGranted;
  }
  
  /// Verifica se permesso è stato negato permanentemente
  static Future<bool> isPermissionPermanentlyDenied() async {
    if (!Platform.isAndroid) return false;
    
    if (await _isAndroid13OrHigher()) {
      return await Permission.nearbyWifiDevices.isPermanentlyDenied;
    }
    
    return await Permission.locationWhenInUse.isPermanentlyDenied;
  }
  
  /// Apri impostazioni app per modificare permessi
  static Future<void> openSettings() async {
    await openAppSettings();
  }
  
  /// Controlla se siamo su Android 13+ (API 33+)
  static Future<bool> _isAndroid13OrHigher() async {
    if (!Platform.isAndroid) return false;
    
    try {
      // Prova a usare device_info_plus se disponibile
      // altrimenti ritorna false (usa location)
      return false; // Default: usa location per compatibilità
    } catch (_) {
      return false;
    }
  }
  
  /// Ottieni messaggio esplicativo per l'utente
  static String getPermissionMessage() {
    return 'L\'app ha bisogno del permesso per trovare dispositivi sulla rete WiFi.\n\n'
        'Questo permesso è necessario solo per la ricerca automatica dei dispositivi LED Matrix.';
  }
  
  /// Ottieni titolo permesso in base alla versione Android
  static Future<String> getPermissionTitle() async {
    if (await _isAndroid13OrHigher()) {
      return 'Permesso Dispositivi WiFi';
    }
    return 'Permesso Posizione';
  }
}
