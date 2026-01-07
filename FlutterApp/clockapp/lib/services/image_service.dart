import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';
import 'package:image/image.dart' as img;
import 'device_service.dart';

/// Image information stored on ESP32
class StoredImage {
  final String name;
  final int size;

  StoredImage({required this.name, required this.size});

  factory StoredImage.fromParts(List<String> parts, int startIndex) {
    return StoredImage(
      name: parts[startIndex],
      size: int.parse(parts[startIndex + 1]),
    );
  }
}

/// Storage information from ESP32
class ImageStorageInfo {
  final int totalBytes;
  final int usedBytes;
  final int freeBytes;

  ImageStorageInfo({
    required this.totalBytes,
    required this.usedBytes,
    required this.freeBytes,
  });

  double get usedPercent => totalBytes > 0 ? (usedBytes / totalBytes) * 100 : 0;
  double get freePercent => totalBytes > 0 ? (freeBytes / totalBytes) * 100 : 0;
}

/// Service for managing images on ESP32 LED Matrix
/// Handles conversion, upload, and storage management
class ImageService {
  final DeviceService _device;
  final StreamController<String> _statusController = StreamController.broadcast();

  String _status = '';
  String get status => _status;
  Stream<String> get statusStream => _statusController.stream;

  ImageService(this._device);

  void dispose() {
    _statusController.close();
  }

  void _setStatus(String status) {
    _status = status;
    _statusController.add(status);
    print('[ImageService] $_status');
  }

  /// Convert image to 64x64 RGB565 format and encode as base64
  ///
  /// Takes any image, resizes to 64x64, converts to RGB565 format,
  /// and returns base64 encoded data ready for ESP32
  Future<String?> convertImageToRGB565Base64(Uint8List imageBytes) async {
    try {
      _setStatus('Decodifica immagine...');

      // Decode image using image package
      img.Image? image = img.decodeImage(imageBytes);
      if (image == null) {
        _setStatus('Errore: impossibile decodificare immagine');
        return null;
      }

      _setStatus('Resize a 64x64...');

      // Resize to 64x64 using Lanczos (high quality)
      img.Image resized = img.copyResize(
        image,
        width: 64,
        height: 64,
        interpolation: img.Interpolation.cubic,
      );

      _setStatus('Conversione RGB565...');

      // Convert to RGB565 format (2 bytes per pixel)
      final rgb565Bytes = Uint8List(64 * 64 * 2);
      int byteIndex = 0;

      for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
          final pixel = resized.getPixel(x, y);

          // Extract RGB components (0-255)
          final r = pixel.r.toInt();
          final g = pixel.g.toInt();
          final b = pixel.b.toInt();

          // Convert to RGB565:
          // - Red: 5 bits (r >> 3)
          // - Green: 6 bits (g >> 2)
          // - Blue: 5 bits (b >> 3)
          final rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);

          // Store as little-endian 16-bit value
          rgb565Bytes[byteIndex++] = rgb565 & 0xFF;        // Low byte
          rgb565Bytes[byteIndex++] = (rgb565 >> 8) & 0xFF; // High byte
        }
      }

      _setStatus('Codifica base64...');

      // Encode to base64
      final base64Data = base64.encode(rgb565Bytes);

      _setStatus('Conversione completata');
      return base64Data;

    } catch (e) {
      _setStatus('Errore conversione: $e');
      print('[ImageService] Conversion error: $e');
      return null;
    }
  }

  /// Upload image to ESP32 storage
  ///
  /// Args:
  ///   name: Image name (without extension)
  ///   imageBytes: Original image data (PNG, JPG, etc.)
  ///
  /// Returns true if upload successful
  Future<bool> uploadImage(String name, Uint8List imageBytes) async {
    try {
      _setStatus('Conversione immagine...');

      // Convert to RGB565 base64
      final base64Data = await convertImageToRGB565Base64(imageBytes);
      if (base64Data == null) {
        return false;
      }

      _setStatus('Upload in corso...');
      print('[ImageService] Uploading image "$name" (${base64Data.length} bytes base64)');

      // Send to ESP32: image,upload,NAME,BASE64
      _device.send('image,upload,$name,$base64Data');

      // Wait for response
      final response = await _waitForResponse('OK,Image uploaded:', timeout: const Duration(seconds: 10));

      if (response != null && response.startsWith('OK,Image uploaded:')) {
        _setStatus('Upload completato!');
        print('[ImageService] Upload successful: $name');
        return true;
      } else {
        _setStatus('Errore upload: ${response ?? "timeout"}');
        print('[ImageService] Upload failed: $response');
        return false;
      }

    } catch (e) {
      _setStatus('Errore: $e');
      print('[ImageService] Upload error: $e');
      return false;
    }
  }

  /// Get list of images stored on ESP32
  Future<List<StoredImage>> listImages() async {
    try {
      _setStatus('Caricamento lista...');

      _device.send('image,list');
      final response = await _waitForResponse('IMAGES,', timeout: const Duration(seconds: 5));

      if (response == null || !response.startsWith('IMAGES,')) {
        _setStatus('Errore: nessuna risposta');
        return [];
      }

      // Parse response: IMAGES,COUNT,name1,size1,name2,size2,...
      final parts = response.split(',');
      if (parts.length < 2) {
        _setStatus('Nessuna immagine');
        return [];
      }

      final count = int.tryParse(parts[1]) ?? 0;
      if (count == 0) {
        _setStatus('Nessuna immagine');
        return [];
      }

      final images = <StoredImage>[];
      for (int i = 0; i < count; i++) {
        final baseIndex = 2 + (i * 2);
        if (baseIndex + 1 < parts.length) {
          images.add(StoredImage.fromParts(parts, baseIndex));
        }
      }

      _setStatus('${images.length} immagini trovate');
      return images;

    } catch (e) {
      _setStatus('Errore: $e');
      print('[ImageService] List error: $e');
      return [];
    }
  }

  /// Delete image from ESP32 storage
  Future<bool> deleteImage(String name) async {
    try {
      _setStatus('Eliminazione...');

      _device.send('image,delete,$name');
      final response = await _waitForResponse('OK,Image deleted:', timeout: const Duration(seconds: 5));

      if (response != null && response.startsWith('OK,Image deleted:')) {
        _setStatus('Immagine eliminata');
        print('[ImageService] Deleted: $name');
        return true;
      } else {
        _setStatus('Errore eliminazione');
        print('[ImageService] Delete failed: $response');
        return false;
      }

    } catch (e) {
      _setStatus('Errore: $e');
      print('[ImageService] Delete error: $e');
      return false;
    }
  }

  /// Get storage information
  Future<ImageStorageInfo?> getStorageInfo() async {
    try {
      _device.send('image,info');
      final response = await _waitForResponse('IMAGE_INFO,', timeout: const Duration(seconds: 5));

      if (response == null || !response.startsWith('IMAGE_INFO,')) {
        print('[ImageService] Invalid storage info response: $response');
        return null;
      }

      // Parse: IMAGE_INFO,total,used,free
      final parts = response.split(',');
      if (parts.length < 4) {
        print('[ImageService] Invalid storage info format');
        return null;
      }

      return ImageStorageInfo(
        totalBytes: int.parse(parts[1]),
        usedBytes: int.parse(parts[2]),
        freeBytes: int.parse(parts[3]),
      );

    } catch (e) {
      print('[ImageService] Storage info error: $e');
      return null;
    }
  }

  /// Wait for specific response from device
  Future<String?> _waitForResponse(String prefix, {Duration timeout = const Duration(seconds: 5)}) async {
    final completer = Completer<String?>();
    late StreamSubscription sub;

    sub = _device.rawDataStream.listen((message) {
      if (message.startsWith(prefix) || message.startsWith('ERR,')) {
        if (!completer.isCompleted) {
          completer.complete(message);
          sub.cancel();
        }
      }
    });

    // Timeout
    Future.delayed(timeout, () {
      if (!completer.isCompleted) {
        completer.complete(null);
        sub.cancel();
      }
    });

    return completer.future;
  }

  /// Format bytes to human readable string
  static String formatBytes(int bytes) {
    if (bytes < 1024) return '$bytes B';
    if (bytes < 1024 * 1024) return '${(bytes / 1024).toStringAsFixed(1)} KB';
    return '${(bytes / (1024 * 1024)).toStringAsFixed(1)} MB';
  }
}
