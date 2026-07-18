import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'package:http/http.dart' as http;
import 'package:path_provider/path_provider.dart';
import '../models/firmware_version.dart';

class FirmwareUpdateService {
  /// Sorgenti firmware provate in ordine: prima il server pubblico,
  /// poi quello locale in LAN (nginx su :8080, stessi endpoint
  /// /api/manifest e /binaries/...). Così la lista si carica anche
  /// quando il telefono non ha accesso a internet (es. AP del display
  /// o rete solo locale).
  static const List<String> defaultBaseUrls = [
    'https://binaries.server21.it',
    'http://10.0.100.117:8080',
  ];

  final List<String> baseUrls;

  /// Base URL della sorgente che ha servito l'ultimo manifest:
  /// i download devono partire dalla stessa sorgente
  String? _activeBaseUrl;
  String? get activeBaseUrl => _activeBaseUrl;

  // Download progress callback
  void Function(int received, int total)? onDownloadProgress;

  FirmwareUpdateService({List<String>? baseUrls})
      : baseUrls = baseUrls ?? defaultBaseUrls;

  /// Converte URL relativo in assoluto usando la sorgente attiva
  String _resolveUrl(String url) {
    if (url.startsWith('http://') || url.startsWith('https://')) {
      return url;
    }
    final base = _activeBaseUrl ?? baseUrls.first;
    // URL relativo - aggiungi base URL
    if (url.startsWith('/')) {
      return '$base$url';
    }
    return '$base/$url';
  }

  /// Fetch firmware manifest: prova le sorgenti in ordine e usa la
  /// prima che risponde
  Future<FirmwareManifest> fetchManifest() async {
    final errors = <String>[];

    for (final base in baseUrls) {
      try {
        final response = await http
            .get(Uri.parse('$base/api/manifest'))
            .timeout(const Duration(seconds: 5));

        if (response.statusCode == 200) {
          final json = jsonDecode(response.body) as Map<String, dynamic>;
          _activeBaseUrl = base;
          return FirmwareManifest.fromJson(json);
        }
        errors.add('$base: HTTP ${response.statusCode}');
      } catch (e) {
        errors.add('$base: $e');
      }
    }

    throw Exception(
        'Failed to fetch manifest from all sources: ${errors.join('; ')}');
  }

  /// Check if an update is available
  Future<UpdateCheckResult> checkForUpdate(
      FirmwareVersion currentVersion) async {
    try {
      final manifest = await fetchManifest();
      final latestRelease = manifest.latestRelease;

      if (latestRelease == null) {
        return UpdateCheckResult(
          updateAvailable: false,
          currentVersion: currentVersion,
        );
      }

      final isNewer = latestRelease.isNewerThan(currentVersion);

      return UpdateCheckResult(
        updateAvailable: isNewer,
        currentVersion: currentVersion,
        latestRelease: latestRelease,
        manifest: manifest,
      );
    } catch (e) {
      return UpdateCheckResult(
        updateAvailable: false,
        currentVersion: currentVersion,
        error: e.toString(),
      );
    }
  }

  /// Find a release in the manifest by version
  FirmwareRelease? findRelease(
      FirmwareManifest manifest, String version, String buildNumber) {
    try {
      return manifest.releases.firstWhere(
        (r) => r.version == version && r.buildNumber == buildNumber,
      );
    } catch (_) {
      return null;
    }
  }

  /// Download firmware from server and save to temp file
  /// Returns the path to the downloaded file
  Future<String> downloadFirmware(
    FirmwareRelease release, {
    void Function(int received, int total)? onProgress,
  }) async {
    try {
      final tempDir = await getTemporaryDirectory();
      final filePath = '${tempDir.path}/firmware_${release.version}_${release.buildNumber}.bin';
      final file = File(filePath);

      // Check if already downloaded
      if (await file.exists()) {
        final existingSize = await file.length();
        if (existingSize == release.size) {
          return filePath;
        }
        // Delete incomplete file
        await file.delete();
      }

      // Download with progress - resolve relative URLs
      final resolvedUrl = _resolveUrl(release.url);
      final request = http.Request('GET', Uri.parse(resolvedUrl));
      final response = await http.Client().send(request);

      if (response.statusCode != 200) {
        throw Exception('Download failed: ${response.statusCode}');
      }

      final total = response.contentLength ?? release.size;
      int received = 0;

      final sink = file.openWrite();

      await for (final chunk in response.stream) {
        sink.add(chunk);
        received += chunk.length;
        onProgress?.call(received, total);
      }

      await sink.close();

      // Verify size
      final downloadedSize = await file.length();
      if (downloadedSize != release.size) {
        await file.delete();
        throw Exception('Download incomplete: $downloadedSize/${release.size} bytes');
      }

      return filePath;
    } catch (e) {
      throw Exception('Download failed: $e');
    }
  }

  /// Download and return firmware bytes directly (for platforms without file access)
  Future<List<int>> downloadFirmwareBytes(
    FirmwareRelease release, {
    void Function(int received, int total)? onProgress,
  }) async {
    try {
      // Resolve relative URLs
      final resolvedUrl = _resolveUrl(release.url);
      final request = http.Request('GET', Uri.parse(resolvedUrl));
      final response = await http.Client().send(request);

      if (response.statusCode != 200) {
        throw Exception('Download failed: ${response.statusCode}');
      }

      final total = response.contentLength ?? release.size;
      int received = 0;
      final bytes = <int>[];

      await for (final chunk in response.stream) {
        bytes.addAll(chunk);
        received += chunk.length;
        onProgress?.call(received, total);
      }

      if (bytes.length != release.size) {
        throw Exception('Download incomplete: ${bytes.length}/${release.size} bytes');
      }

      return bytes;
    } catch (e) {
      throw Exception('Download failed: $e');
    }
  }
}

class UpdateCheckResult {
  final bool updateAvailable;
  final FirmwareVersion currentVersion;
  final FirmwareRelease? latestRelease;
  final FirmwareManifest? manifest;
  final String? error;

  UpdateCheckResult({
    required this.updateAvailable,
    required this.currentVersion,
    this.latestRelease,
    this.manifest,
    this.error,
  });

  bool get hasError => error != null;
}
