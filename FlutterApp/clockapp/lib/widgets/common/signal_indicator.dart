import 'package:flutter/material.dart';

/// Widget per mostrare la potenza del segnale WiFi
class SignalIndicator extends StatelessWidget {
  final int strength;

  const SignalIndicator({super.key, required this.strength});

  @override
  Widget build(BuildContext context) {
    final bars = (strength / 25).ceil().clamp(1, 4);

    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        for (int i = 1; i <= 4; i++)
          Container(
            width: 3,
            height: 4.0 + (i * 3),
            margin: const EdgeInsets.only(left: 1),
            decoration: BoxDecoration(
              color: i <= bars
                  ? (bars >= 3
                      ? Colors.green
                      : (bars >= 2 ? Colors.orange : Colors.red))
                  : Colors.grey.withOpacity(0.3),
              borderRadius: BorderRadius.circular(1),
            ),
          ),
        const SizedBox(width: 4),
        Text(
          '$strength%',
          style: TextStyle(fontSize: 11, color: Colors.grey[400]),
        ),
      ],
    );
  }
}
