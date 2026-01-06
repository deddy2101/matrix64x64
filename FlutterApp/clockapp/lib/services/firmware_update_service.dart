import 'dart:async';
import 'dart:convert';
import 'package:http/http.dart' as http;
import '../models/firmware_version.dart';

class FirmwareUpdateService {
  static const String defaultManifestUrl =
      'https://binaries.server21.it/api/manifest';

  final String manifestUrl;

  FirmwareUpdateService({this.manifestUrl = defaultManifestUrl});

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
