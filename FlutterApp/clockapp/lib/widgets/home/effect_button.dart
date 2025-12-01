import 'package:flutter/material.dart';

/// Pulsante effetto per la griglia
class EffectButton extends StatelessWidget {
  final String name;
  final int index;
  final bool isSelected;
  final VoidCallback? onTap;

  const EffectButton({
    super.key,
    required this.name,
    required this.index,
    required this.isSelected,
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
          decoration: BoxDecoration(
            color: isSelected
                ? Theme.of(context).colorScheme.primary.withOpacity(0.3)
                : Colors.white.withOpacity(0.05),
            borderRadius: BorderRadius.circular(12),
            border: Border.all(
              color: isSelected
                  ? Theme.of(context).colorScheme.primary
                  : Colors.white.withOpacity(0.1),
              width: isSelected ? 2 : 1,
            ),
          ),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Text(
                '$index',
                style: TextStyle(
                  fontWeight: FontWeight.bold,
                  color: isSelected
                      ? Theme.of(context).colorScheme.primary
                      : Colors.white,
                ),
              ),
              const SizedBox(height: 4),
              Text(
                name.length > 8 ? '${name.substring(0, 8)}...' : name,
                style: TextStyle(fontSize: 9, color: Colors.grey[400]),
                textAlign: TextAlign.center,
                maxLines: 1,
              ),
            ],
          ),
        ),
      ),
    );
  }
}
