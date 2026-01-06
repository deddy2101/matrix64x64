import 'dart:convert';
import 'dart:io';
import 'package:http/http.dart' as http;
import 'package:path_provider/path_provider.dart';

/// Rappresenta una versione firmware disponibile
class FirmwareVersion {
  final String version;
  final int buildNumber;
  final String fullVersion;
  final String url;
  final int size;
  final String md5;
  final DateTime releaseDate;

  FirmwareVersion({
    required this.version,
    required this.buildNumber,
    required this.fullVersion,
    required this.url,
    required this.size,
    required this.md5,
    required this.releaseDate,
  });

  factory FirmwareVersion.fromJson(Map<String, dynamic> json) {
    return FirmwareVersion(
      version: json['version'],
      buildNumber: json['buildNumber'],
      fullVersion: json['fullVersion'],
      url: json['url'],
      size: json['size'],
      md5: json['md5'],
      releaseDate: DateTime.parse(json['releaseDate']),
    );
  }

  /// Ritorna true se questa versione è più recente di [other]
  bool isNewerThan(FirmwareVersion other) {
    return buildNumber > other.buildNumber;
  }

  String get sizeFormatted {
    if (size < 1024) return '$size B';
    if (size < 1024 * 1024) return '${(size / 1024).toStringAsFixed(2)} KB';
    return '${(size / (1024 * 1024)).toStringAsFixed(2)} MB';
  }
}

/// Repository per scaricare e gestire firmware da server remoto
class FirmwareRepository {
  final String baseUrl;

  FirmwareRepository({required this.baseUrl});

  /// Scarica la lista delle versioni disponibili dal server
  Future<List<FirmwareVersion>> fetchAvailableVersions() async {
    try {
      final response = await http.get(
        Uri.parse('$baseUrl/api/manifest'),
        headers: {'Accept': 'application/json'},
      );

      if (response.statusCode != 200) {
        throw Exception('Failed to load manifest: ${response.statusCode}');
      }

      final data = json.decode(response.body);
      final versions = (data['versions'] as List)
          .map((v) => FirmwareVersion.fromJson(v))
          .toList();

      // Ordina per build number (più recente prima)
      versions.sort((a, b) => b.buildNumber.compareTo(a.buildNumber));

      return versions;
    } catch (e) {
      print('[FirmwareRepo] Error fetching versions: $e');
      rethrow;
    }
  }

  /// Scarica un firmware specifico e salva in temp directory
  /// Ritorna il path del file scaricato
  Future<String> downloadFirmware(
    FirmwareVersion version, {
    Function(int received, int total)? onProgress,
  }) async {
    try {
      final url = baseUrl + version.url;
      print('[FirmwareRepo] Downloading from: $url');

      final request = http.Request('GET', Uri.parse(url));
      final response = await request.send();

      if (response.statusCode != 200) {
        throw Exception('Failed to download firmware: ${response.statusCode}');
      }

      // Crea file temporaneo
      final tempDir = await getTemporaryDirectory();
      final filePath = '${tempDir.path}/firmware_${version.fullVersion}.bin';
      final file = File(filePath);

      // Scarica con progress tracking
      final totalBytes = response.contentLength ?? 0;
      int receivedBytes = 0;

      final sink = file.openWrite();

      await for (var chunk in response.stream) {
        sink.add(chunk);
        receivedBytes += chunk.length;

        if (onProgress != null && totalBytes > 0) {
          onProgress(receivedBytes, totalBytes);
        }
      }

      await sink.close();

      print('[FirmwareRepo] Downloaded to: $filePath');
      return filePath;
    } catch (e) {
      print('[FirmwareRepo] Error downloading firmware: $e');
      rethrow;
    }
  }

  /// Trova la versione più recente disponibile
  Future<FirmwareVersion?> getLatestVersion() async {
    final versions = await fetchAvailableVersions();
    return versions.isNotEmpty ? versions.first : null;
  }

  /// Verifica se ci sono aggiornamenti disponibili rispetto alla versione corrente
  Future<FirmwareVersion?> checkForUpdate(String currentVersion) async {
    try {
      final versions = await fetchAvailableVersions();

      if (versions.isEmpty) return null;

      // Estrai build number dalla versione corrente (es: "1.0.0+1" → 1)
      final currentBuildMatch = RegExp(r'\+(\d+)').firstMatch(currentVersion);
      if (currentBuildMatch == null) return null;

      final currentBuild = int.parse(currentBuildMatch.group(1)!);

      // Trova la prima versione con build number maggiore
      for (var version in versions) {
        if (version.buildNumber > currentBuild) {
          return version;
        }
      }

      return null; // Nessun aggiornamento disponibile
    } catch (e) {
      print('[FirmwareRepo] Error checking for update: $e');
      return null;
    }
  }
}
