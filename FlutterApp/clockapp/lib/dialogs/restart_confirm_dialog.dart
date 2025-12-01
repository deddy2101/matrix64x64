import 'package:flutter/material.dart';

/// Dialog conferma riavvio ESP32
class RestartConfirmDialog extends StatelessWidget {
  const RestartConfirmDialog({super.key});

  @override
  Widget build(BuildContext context) {
    return AlertDialog(
      backgroundColor: const Color(0xFF1a1a2e),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
      title: const Row(
        children: [
          Icon(Icons.restart_alt, color: Colors.orange),
          SizedBox(width: 12),
          Text('Riavviare?'),
        ],
      ),
      content: const Text(
        'L\'ESP32 si riavvierà per applicare le nuove impostazioni WiFi.\n\n'
        'Dovrai riconnetterti dopo il riavvio.',
      ),
      actions: [
        TextButton(
          onPressed: () => Navigator.of(context).pop(false),
          child: const Text('Annulla'),
        ),
        ElevatedButton(
          onPressed: () => Navigator.of(context).pop(true),
          style: ElevatedButton.styleFrom(backgroundColor: Colors.orange),
          child: const Text('Riavvia'),
        ),
      ],
    );
  }
}
