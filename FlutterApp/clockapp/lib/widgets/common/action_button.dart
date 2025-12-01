import 'package:flutter/material.dart';

/// Pulsante azione con icona e label
class ActionButton extends StatelessWidget {
  final IconData icon;
  final String label;
  final VoidCallback? onTap;
  final Color? color;
  final bool enabled;

  const ActionButton({
    super.key,
    required this.icon,
    required this.label,
    this.onTap,
    this.color,
    this.enabled = true,
  });

  @override
  Widget build(BuildContext context) {
    return Material(
      color: Colors.transparent,
      child: InkWell(
        onTap: enabled ? onTap : null,
        borderRadius: BorderRadius.circular(12),
        child: Container(
          padding: const EdgeInsets.symmetric(vertical: 12, horizontal: 16),
          decoration: BoxDecoration(
            color: (color ?? Theme.of(context).colorScheme.primary).withOpacity(
              enabled ? 0.15 : 0.05,
            ),
            borderRadius: BorderRadius.circular(12),
            border: Border.all(
              color: (color ?? Theme.of(context).colorScheme.primary)
                  .withOpacity(enabled ? 0.3 : 0.1),
            ),
          ),
          child: Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Icon(
                icon,
                size: 20,
                color: enabled
                    ? (color ?? Theme.of(context).colorScheme.primary)
                    : Colors.grey,
              ),
              const SizedBox(width: 8),
              Text(
                label,
                style: TextStyle(
                  color: enabled ? Colors.white : Colors.grey,
                  fontWeight: FontWeight.w500,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
