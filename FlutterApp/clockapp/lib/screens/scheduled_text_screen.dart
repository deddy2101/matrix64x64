import 'package:flutter/material.dart';
import '../services/device_service.dart';
import '../services/scheduled_text_service.dart';
import '../models/scheduled_text.dart';
import '../dialogs/add_scheduled_text_dialog.dart';

class ScheduledTextScreen extends StatefulWidget {
  final DeviceService deviceService;

  const ScheduledTextScreen({
    super.key,
    required this.deviceService,
  });

  @override
  State<ScheduledTextScreen> createState() => _ScheduledTextScreenState();
}

class _ScheduledTextScreenState extends State<ScheduledTextScreen> {
  late ScheduledTextService _scheduledTextService;
  List<ScheduledText> _scheduledTexts = [];
  bool _loading = false;
  String _status = '';

  @override
  void initState() {
    super.initState();
    _scheduledTextService = ScheduledTextService(widget.deviceService);
    _scheduledTextService.statusStream.listen((status) {
      if (mounted) {
        setState(() => _status = status);
      }
    });
    _refreshScheduledTexts();
  }

  @override
  void dispose() {
    _scheduledTextService.dispose();
    super.dispose();
  }

  Future<void> _refreshScheduledTexts() async {
    if (!widget.deviceService.isConnected) {
      _showSnackBar('Dispositivo non connesso');
      return;
    }

    setState(() => _loading = true);

    try {
      final texts = await _scheduledTextService.listScheduledTexts();

      if (mounted) {
        setState(() {
          _scheduledTexts = texts;
          _loading = false;
        });
      }
    } catch (e) {
      if (mounted) {
        setState(() => _loading = false);
        _showSnackBar('Errore caricamento: $e');
      }
    }
  }

  Future<void> _addScheduledText() async {
    if (!widget.deviceService.isConnected) {
      _showSnackBar('Dispositivo non connesso');
      return;
    }

    final result = await showDialog<Map<String, dynamic>>(
      context: context,
      builder: (context) => const AddScheduledTextDialog(),
    );

    if (result == null) return;

    final success = await _scheduledTextService.addScheduledText(
      text: result['text'],
      color: result['color'],
      hour: result['hour'],
      minute: result['minute'],
      repeatDays: result['repeatDays'] ?? 0xFF,
      year: result['year'] ?? 0,
      month: result['month'] ?? 0,
      day: result['day'] ?? 0,
      loopCount: result['loopCount'] ?? 1,
    );

    if (success) {
      _showSnackBar('Scritta programmata aggiunta');
      _refreshScheduledTexts();
    } else {
      _showSnackBar('Errore durante l\'aggiunta');
    }
  }

  Future<void> _editScheduledText(ScheduledText scheduledText) async {
    if (!widget.deviceService.isConnected) {
      _showSnackBar('Dispositivo non connesso');
      return;
    }

    final result = await showDialog<Map<String, dynamic>>(
      context: context,
      builder: (context) => AddScheduledTextDialog(scheduledText: scheduledText),
    );

    if (result == null) return;

    final success = await _scheduledTextService.updateScheduledText(
      id: scheduledText.id,
      text: result['text'],
      color: result['color'],
      hour: result['hour'],
      minute: result['minute'],
      repeatDays: result['repeatDays'] ?? 0xFF,
      year: result['year'] ?? 0,
      month: result['month'] ?? 0,
      day: result['day'] ?? 0,
      loopCount: result['loopCount'] ?? 1,
    );

    if (success) {
      _showSnackBar('Scritta programmata aggiornata');
      _refreshScheduledTexts();
    } else {
      _showSnackBar('Errore durante l\'aggiornamento');
    }
  }

  Future<void> _deleteScheduledText(ScheduledText scheduledText) async {
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Conferma eliminazione'),
        content: Text('Eliminare la scritta programmata "${scheduledText.text}"?'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context, false),
            child: const Text('Annulla'),
          ),
          TextButton(
            onPressed: () => Navigator.pop(context, true),
            style: TextButton.styleFrom(foregroundColor: Colors.red),
            child: const Text('Elimina'),
          ),
        ],
      ),
    );

    if (confirmed != true) return;

    final success = await _scheduledTextService.deleteScheduledText(scheduledText.id);

    if (success) {
      _showSnackBar('Scritta programmata eliminata');
      _refreshScheduledTexts();
    } else {
      _showSnackBar('Errore durante l\'eliminazione');
    }
  }

  Future<void> _toggleEnabled(ScheduledText scheduledText) async {
    final success = scheduledText.enabled
        ? await _scheduledTextService.disableScheduledText(scheduledText.id)
        : await _scheduledTextService.enableScheduledText(scheduledText.id);

    if (success) {
      _refreshScheduledTexts();
    } else {
      _showSnackBar('Errore durante il cambio stato');
    }
  }

  void _showSnackBar(String message) {
    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text(message), duration: const Duration(seconds: 2)),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.black,
      appBar: AppBar(
        backgroundColor: Colors.grey[900],
        title: const Text('Scritte Programmate'),
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: _refreshScheduledTexts,
          ),
        ],
      ),
      body: Column(
        children: [
          // Status bar
          if (_status.isNotEmpty)
            Container(
              width: double.infinity,
              padding: const EdgeInsets.all(12),
              color: Colors.grey[850],
              child: Text(
                _status,
                style: const TextStyle(color: Colors.white70, fontSize: 12),
                textAlign: TextAlign.center,
              ),
            ),

          // Content
          Expanded(
            child: _loading
                ? const Center(child: CircularProgressIndicator())
                : _scheduledTexts.isEmpty
                    ? _buildEmptyState()
                    : _buildScheduledTextList(),
          ),
        ],
      ),
      floatingActionButton: FloatingActionButton(
        onPressed: _addScheduledText,
        backgroundColor: const Color(0xFF8B5CF6),
        child: const Icon(Icons.add),
      ),
    );
  }

  Widget _buildEmptyState() {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(Icons.schedule, size: 64, color: Colors.grey[700]),
          const SizedBox(height: 16),
          Text(
            'Nessuna scritta programmata',
            style: TextStyle(color: Colors.grey[600], fontSize: 18),
          ),
          const SizedBox(height: 8),
          Text(
            'Premi + per aggiungerne una',
            style: TextStyle(color: Colors.grey[700], fontSize: 14),
          ),
        ],
      ),
    );
  }

  Widget _buildScheduledTextList() {
    return ListView.builder(
      padding: const EdgeInsets.all(16),
      itemCount: _scheduledTexts.length,
      itemBuilder: (context, index) {
        final scheduledText = _scheduledTexts[index];
        return _buildScheduledTextCard(scheduledText);
      },
    );
  }

  Widget _buildScheduledTextCard(ScheduledText scheduledText) {
    return Card(
      color: Colors.grey[900],
      margin: const EdgeInsets.only(bottom: 12),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // Header with time and actions
            Row(
              children: [
                // Time
                Container(
                  padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
                  decoration: BoxDecoration(
                    color: const Color(0xFF8B5CF6),
                    borderRadius: BorderRadius.circular(8),
                  ),
                  child: Text(
                    scheduledText.timeString,
                    style: const TextStyle(
                      color: Colors.white,
                      fontSize: 18,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                ),
                const SizedBox(width: 12),

                // Color indicator
                Container(
                  width: 24,
                  height: 24,
                  decoration: BoxDecoration(
                    color: scheduledText.color,
                    borderRadius: BorderRadius.circular(4),
                    border: Border.all(color: Colors.white24),
                  ),
                ),
                const Spacer(),

                // Enabled switch
                Switch(
                  value: scheduledText.enabled,
                  onChanged: (_) => _toggleEnabled(scheduledText),
                  activeTrackColor: const Color(0xFF8B5CF6),
                ),

                // Edit button
                IconButton(
                  icon: const Icon(Icons.edit, color: Colors.white70),
                  onPressed: () => _editScheduledText(scheduledText),
                ),

                // Delete button
                IconButton(
                  icon: const Icon(Icons.delete, color: Colors.red),
                  onPressed: () => _deleteScheduledText(scheduledText),
                ),
              ],
            ),
            const SizedBox(height: 12),

            // Text
            Text(
              scheduledText.text,
              style: TextStyle(
                color: scheduledText.enabled ? Colors.white : Colors.grey[600],
                fontSize: 16,
              ),
            ),
            const SizedBox(height: 8),

            // Repeat pattern
            Row(
              children: [
                Icon(Icons.repeat, size: 16, color: Colors.grey[600]),
                const SizedBox(width: 4),
                Text(
                  scheduledText.repeatPattern,
                  style: TextStyle(
                    color: Colors.grey[500],
                    fontSize: 12,
                  ),
                ),
                const SizedBox(width: 16),
                Icon(Icons.replay, size: 16, color: Colors.grey[600]),
                const SizedBox(width: 4),
                Text(
                  scheduledText.loopCount == 0
                      ? 'Loop infinito'
                      : '${scheduledText.loopCount} loop',
                  style: TextStyle(
                    color: Colors.grey[500],
                    fontSize: 12,
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}
