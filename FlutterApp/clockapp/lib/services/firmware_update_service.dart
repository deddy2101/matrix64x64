import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'package:http/http.dart' as http;
import 'package:path_provider/path_provider.dart';
import '../models/firmware_version.dart';

class FirmwareUpdateService {
  static const String defaultManifestUrl =
      'https://binaries.server21.it/api/manifest';
  static const String defaultBaseUrl =
      'https://binaries.server21.it';

  final String manifestUrl;
  final String baseUrl;

  // Download progress callback
  void Function(int received, int total)? onDownloadProgress;

  FirmwareUpdateService({
    this.manifestUrl = defaultManifestUrl,
    this.baseUrl = defaultBaseUrl,
  });

  /// Converte URL relativo in assoluto
  String _resolveUrl(String url) {
    if (url.startsWith('http://') || url.startsWith('https://')) {
      return url;
    }
    // URL relativo - aggiungi base URL
    if (url.startsWith('/')) {
      return '$baseUrl$url';
    }
    return '$baseUrl/$url';
  }

  /// Fetch firmware manifest from server
  Future<FirmwareManifest> fetchManifest() async {
    try {
      final response = await http
          .get(Uri.parse(manifestUrl))
          .timeout(const Duration(seconds: 10));

      if (response.statusCode == 200) {
        final json = jsonDecode(response.body) as Map<String, dynamic>;
        return FirmwareManifest.fromJson(json);
      } else {
        throw Exception('Failed to load manifest: ${response.statusCode}');
      }
    } catch (e) {
      throw Exception('Failed to fetch manifest: $e');
    }
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
