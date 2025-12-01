import 'package:flutter/material.dart';

/// Pulsante per selezionare orari notte/giorno
class TimePickerButton extends StatelessWidget {
  final String label;
  final int hour;
  final IconData icon;
  final VoidCallback? onTap;

  const TimePickerButton({
    super.key,
    required this.label,
    required this.hour,
    required this.icon,
    this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    return Material(
      color: Colors.transparent,
      child: InkWell(
        onTap: onTap,
        borderRadius: BorderRadius.circular(12),
        child: Container(
          padding: const EdgeInsets.symmetric(vertical: 12, horizontal: 16),
          decoration: BoxDecoration(
            color: Colors.grey.withOpacity(0.1),
            borderRadius: BorderRadius.circular(12),
            border: Border.all(color: Colors.grey.withOpacity(0.2)),
          ),
          child: Row(
            children: [
              Icon(icon, size: 18, color: Colors.grey[400]),
              const SizedBox(width: 8),
              Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    label,
                    style: TextStyle(fontSize: 11, color: Colors.grey[500]),
                  ),
                  Text(
                    '${hour.toString().padLeft(2, '0')}:00',
                    style: const TextStyle(
                      fontSize: 16,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                ],
              ),
              const Spacer(),
              Icon(Icons.edit, size: 16, color: Colors.grey[500]),
            ],
          ),
        ),
      ),
    );
  }
}
