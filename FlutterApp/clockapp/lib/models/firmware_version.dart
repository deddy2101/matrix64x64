class FirmwareVersion {
  final String version;
  final String buildNumber;
  final String buildDate;
  final String buildTime;

  FirmwareVersion({
    required this.version,
    required this.buildNumber,
    required this.buildDate,
    required this.buildTime,
  });

  /// Parse from ESP32 response: VERSION,1.0.0,1,Jan 6 2026,17:45:00
  factory FirmwareVersion.fromCsvResponse(String response) {
    final parts = response.split(',');
    if (parts.length < 5 || parts[0] != 'VERSION') {
      throw FormatException('Invalid VERSION response: $response');
    }

    return FirmwareVersion(
      version: parts[1],
      buildNumber: parts[2],
      buildDate: parts[3],
      buildTime: parts[4],
    );
  }

  String get fullVersion => '$version (build $buildNumber)';

  @override
  String toString() => fullVersion;

  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      other is FirmwareVersion &&
          runtimeType == other.runtimeType &&
          version == other.version &&
          buildNumber == other.buildNumber;

  @override
  int get hashCode => version.hashCode ^ buildNumber.hashCode;
}

class FirmwareManifest {
  final String latestVersion;
  final List<FirmwareRelease> releases;

  FirmwareManifest({
    required this.latestVersion,
    required this.releases,
  });

  factory FirmwareManifest.fromJson(Map<String, dynamic> json) {
    return FirmwareManifest(
      latestVersion: json['latest_version'] as String,
      releases: (json['releases'] as List)
          .map((r) => FirmwareRelease.fromJson(r as Map<String, dynamic>))
          .toList(),
    );
  }

  FirmwareRelease? get latestRelease =>
      releases.firstWhere((r) => r.version == latestVersion);
}

class FirmwareRelease {
  final String version;
  final String buildNumber;
  final String url;
  final int size;
  final String md5;
  final String releaseDate;
  final String? description;

  FirmwareRelease({
    required this.version,
    required this.buildNumber,
    required this.url,
    required this.size,
    required this.md5,
    required this.releaseDate,
    this.description,
  });

  factory FirmwareRelease.fromJson(Map<String, dynamic> json) {
    return FirmwareRelease(
      version: json['version'] as String,
      buildNumber: json['build_number'] as String,
      url: json['url'] as String,
      size: json['size'] as int,
      md5: json['md5'] as String,
      releaseDate: json['release_date'] as String,
      description: json['description'] as String?,
    );
  }

  String get fullVersion => '$version (build $buildNumber)';

  bool isNewerThan(FirmwareVersion current) {
    // Simple comparison: compare build numbers if versions are the same
    if (version == current.version) {
      return int.parse(buildNumber) > int.parse(current.buildNumber);
    }
    // Compare version strings (assuming semantic versioning)
    return _compareVersions(version, current.version) > 0;
  }

  static int _compareVersions(String v1, String v2) {
    final parts1 = v1.split('.').map(int.parse).toList();
    final parts2 = v2.split('.').map(int.parse).toList();

    for (int i = 0; i < 3; i++) {
      final p1 = i < parts1.length ? parts1[i] : 0;
      final p2 = i < parts2.length ? parts2[i] : 0;
      if (p1 != p2) return p1.compareTo(p2);
    }
    return 0;
  }
}
