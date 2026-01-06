import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';
import 'package:crypto/crypto.dart';
import 'device_service.dart';

/// Servizio per gestire OTA (Over-The-Air) firmware update dell'ESP32
class OtaService {
  final DeviceService _device;

  bool _isUpdating = false;
  int _progress = 0;
  String _status = '';

  OtaService(this._device);

  bool get isUpdating => _isUpdating;
  int get progress => _progress;
  String get status => _status;

  /// Invia firmware update via WebSocket
  ///
  /// [firmwarePath] - Percorso del file .bin
  /// Returns true se l'update è completato con successo
  Future<bool> updateFirmware(String firmwarePath) async {
    if (!_device.isConnected) {
      _status = 'Dispositivo non connesso';
      return false;
    }

    if (_isUpdating) {
      _status = 'Update già in corso';
      return false;
    }

    try {
      _isUpdating = true;
      _progress = 0;
      _status = 'Lettura firmware...';

      // Leggi file
      final file = File(firmwarePath);
      if (!await file.exists()) {
        _status = 'File non trovato';
        return false;
      }

      final bytes = await file.readAsBytes();
      final size = bytes.length;

      _status = 'Calcolo MD5...';
      final md5Hash = md5.convert(bytes).toString();

      print('[OTA] File size: $size bytes, MD5: $md5Hash');

      // 1. Invia comando start
      _status = 'Inizializzazione OTA...';
      _device.send('ota,start,$size');

      // Aspetta conferma OTA_READY
      await Future.delayed(Duration(milliseconds: 500));

      // 2. Invia dati in chunk di 512 bytes
      _status = 'Invio firmware...';
      const chunkSize = 512;
      int offset = 0;

      while (offset < size) {
        final end = (offset + chunkSize < size) ? offset + chunkSize : size;
        final chunk = bytes.sublist(offset, end);

        // Converti chunk in base64
        final base64Chunk = base64.encode(chunk);

        // Invia chunk
        _device.send('ota,data,$base64Chunk');

        offset = end;
        _progress = ((offset * 100) / size).round();

        print('[OTA] Progress: $_progress% ($offset/$size bytes)');

        // Piccolo delay per non saturare il WebSocket
        await Future.delayed(Duration(milliseconds: 50));
      }

      // 3. Finalizza con MD5
      _status = 'Verifica integrità...';
      _device.send('ota,end,$md5Hash');

      // Aspetta conferma
      await Future.delayed(Duration(seconds: 2));

      _status = 'Update completato!';
      _progress = 100;

      return true;

    } catch (e) {
      _status = 'Errore: $e';
      print('[OTA] Error: $e');
      return false;
    } finally {
      _isUpdating = false;
    }
  }

  /// Annulla update in corso
  void abortUpdate() {
    if (_isUpdating) {
      _device.send('ota,abort');
      _isUpdating = false;
      _status = 'Update annullato';
    }
  }
}
