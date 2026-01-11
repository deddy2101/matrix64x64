import 'dart:typed_data';
import 'package:flutter/material.dart';
import 'package:file_picker/file_picker.dart';
import '../../services/ota_service.dart';
import '../../services/firmware_update_service.dart';
import '../../models/firmware_version.dart';
import '../common/common_widgets.dart';

class OTACard extends StatefulWidget {
  final OtaService otaService;
  final FirmwareUpdateService firmwareUpdateService;
  final FirmwareVersion? currentVersion;
  final FirmwareManifest? manifest;
  final Function(String) onLog;

  const OTACard({
    super.key,
    required this.otaService,
    required this.firmwareUpdateService,
    required this.currentVersion,
    required this.manifest,
    required this.onLog,
  });

  @override
  State<OTACard> createState() => _OTACardState();
}

class _OTACardState extends State<OTACard> {
  @override
  void initState() {
    super.initState();
    // Listen to OtaService changes for automatic UI updates
    widget.otaService.addListener(_onOtaServiceChanged);
  }

  @override
  void dispose() {
    widget.otaService.removeListener(_onOtaServiceChanged);
    super.dispose();
  }

  void _onOtaServiceChanged() {
    if (mounted) {
      setState(() {});
    }
  }

  bool _hasNewerVersion() {
    if (widget.currentVersion == null || widget.manifest?.latestRelease == null) return false;
    return widget.manifest!.latestRelease!.isNewerThan(widget.currentVersion!);
  }

  @override
  Widget build(BuildContext context) {
    return GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(Icons.system_update, color: Theme.of(context).colorScheme.primary),
              const SizedBox(width: 12),
              const Text('Aggiornamenti OTA', style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600)),
            ],
          ),
          const SizedBox(height: 16),

          // Versioni
          if (widget.currentVersion != null || widget.manifest?.latestRelease != null) ...[
            if (widget.currentVersion != null)
              _buildVersionInfo('Versione Corrente', widget.currentVersion, null, Colors.blue[400]!),
            if (widget.currentVersion != null && widget.manifest?.latestRelease != null)
              const SizedBox(height: 12),
            if (widget.manifest?.latestRelease != null)
              _buildVersionInfo('Ultima Disponibile', null, widget.manifest!.latestRelease, Colors.green[400]!),
            const SizedBox(height: 12),
          ],

          const SizedBox(height: 4),

          // Progress o pulsanti
          if (widget.otaService.isUpdating) ...[
            Column(
              children: [
                LinearProgressIndicator(
                  value: widget.otaService.progress / 100,
                  backgroundColor: Colors.grey[800],
                  color: Colors.green[400],
                  minHeight: 8,
                ),
                const SizedBox(height: 12),
                Text(
                  '${widget.otaService.status} (${widget.otaService.progress}%)',
                  style: TextStyle(color: Colors.grey[400], fontSize: 14),
                ),
                const SizedBox(height: 16),
                ActionButton(
                  icon: Icons.cancel,
                  label: 'Annulla',
                  color: Colors.red[400],
                  onTap: () {
                    widget.otaService.abortUpdate();
                    widget.onLog('⚠ OTA annullato');
                  },
                ),
              ],
            ),
          ] else ...[
            // Installa da server
            if (widget.manifest?.latestRelease != null && _hasNewerVersion()) ...[
              ActionButton(
                icon: Icons.cloud_download,
                label: 'Installa ${widget.manifest!.latestRelease!.fullVersion}',
                color: Colors.green[400],
                onTap: () => _downloadAndInstall(context),
              ),
              const SizedBox(height: 12),
            ],
            // Upload manuale
            ActionButton(
              icon: Icons.upload_file,
              label: 'Seleziona file .bin',
              color: Colors.purple[400],
              onTap: () => _selectAndUpload(context),
            ),
          ],
        ],
      ),
    );
  }

  Widget _buildVersionInfo(String label, FirmwareVersion? version, FirmwareRelease? release, Color color) {
    final displayVersion = release?.fullVersion ?? version?.fullVersion ?? 'N/A';
    final buildDate = release?.releaseDate ?? (version != null ? '${version.buildDate} ${version.buildTime}' : null);

    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: color.withOpacity(0.1),
        borderRadius: BorderRadius.circular(8),
        border: Border.all(color: color.withOpacity(0.3)),
      ),
      child: Row(
        children: [
          Icon(Icons.circle, size: 12, color: color),
          const SizedBox(width: 12),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(label, style: TextStyle(color: Colors.grey[400], fontSize: 11, fontWeight: FontWeight.w500)),
                const SizedBox(height: 4),
                Text(displayVersion, style: TextStyle(color: color, fontSize: 14, fontWeight: FontWeight.bold)),
                if (buildDate != null) ...[
                  const SizedBox(height: 2),
                  Text(buildDate, style: TextStyle(color: Colors.grey[600], fontSize: 10)),
                ],
              ],
            ),
          ),
        ],
      ),
    );
  }

  Future<void> _downloadAndInstall(BuildContext context) async {
    final release = widget.manifest!.latestRelease!;
    try {
      widget.onLog('→ Download firmware ${release.fullVersion}...');

      final bytes = await widget.firmwareUpdateService.downloadFirmwareBytes(
        release,
        onProgress: (received, total) {
          final percent = (received * 100 / total).round();
          if (percent % 10 == 0) widget.onLog('↓ Download: $percent%');
        },
      );

      widget.onLog('✓ Download completato (${bytes.length} bytes)');
      widget.onLog('→ Inizio OTA update...');

      final success = await widget.otaService.updateFirmwareFromBytes(Uint8List.fromList(bytes), expectedMd5: release.md5);

      if (success) {
        widget.onLog('✓ ${widget.otaService.status}');
        if (context.mounted) {
          final statusMsg = widget.otaService.status;
          final isVersionChanged = !statusMsg.contains('non cambiata');
          _showResultDialog(context, isVersionChanged, statusMsg);
        }
      } else {
        widget.onLog('✗ Errore: ${widget.otaService.status}');
        if (context.mounted) {
          _showErrorDialog(context, widget.otaService.status);
        }
      }
    } catch (e) {
      widget.onLog('✗ Errore: $e');
      if (context.mounted) {
        _showErrorDialog(context, '$e');
      }
    }
  }

  Future<void> _selectAndUpload(BuildContext context) async {
    try {
      final result = await FilePicker.platform.pickFiles(type: FileType.custom, allowedExtensions: ['bin']);
      if (result == null || result.files.isEmpty) return;

      final filePath = result.files.first.path;
      if (filePath == null) {
        widget.onLog('⚠ Impossibile leggere il file');
        return;
      }

      widget.onLog('→ Inizio OTA update...');
      widget.onLog('→ File: ${result.files.first.name}');

      final success = await widget.otaService.updateFirmware(filePath);

      if (success) {
        widget.onLog('✓ ${widget.otaService.status}');
        if (context.mounted) {
          final statusMsg = widget.otaService.status;
          final isVersionChanged = !statusMsg.contains('non cambiata');
          _showResultDialog(context, isVersionChanged, statusMsg);
        }
      } else {
        widget.onLog('✗ Errore: ${widget.otaService.status}');
        if (context.mounted) {
          _showErrorDialog(context, widget.otaService.status);
        }
      }
    } catch (e) {
      widget.onLog('✗ Errore: $e');
      if (context.mounted) {
        _showErrorDialog(context, '$e');
      }
    }
  }

  void _showResultDialog(BuildContext context, bool isVersionChanged, String message) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        backgroundColor: const Color(0xFF1a1a2e),
        title: Row(
          children: [
            Icon(isVersionChanged ? Icons.check_circle : Icons.info,
                color: isVersionChanged ? Colors.green[400] : Colors.orange[400]),
            const SizedBox(width: 12),
            Text(isVersionChanged ? 'Update Completato' : 'Update Eseguito'),
          ],
        ),
        content: Text(message),
        actions: [TextButton(onPressed: () => Navigator.pop(context), child: const Text('OK'))],
      ),
    );
  }

  void _showErrorDialog(BuildContext context, String error) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        backgroundColor: const Color(0xFF1a1a2e),
        title: Row(
          children: [
            Icon(Icons.error, color: Colors.red[400]),
            const SizedBox(width: 12),
            const Text('Errore Update'),
          ],
        ),
        content: Text(error),
        actions: [TextButton(onPressed: () => Navigator.pop(context), child: const Text('OK'))],
      ),
    );
  }
}
