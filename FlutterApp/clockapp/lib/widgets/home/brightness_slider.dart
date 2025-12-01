import 'dart:async';
import 'package:flutter/material.dart';

/// Slider luminosità con debounce
class BrightnessSlider extends StatefulWidget {
  final String label;
  final IconData icon;
  final int value;
  final Function(int) onChanged;

  const BrightnessSlider({
    super.key,
    required this.label,
    required this.icon,
    required this.value,
    required this.onChanged,
  });

  @override
  State<BrightnessSlider> createState() => _BrightnessSliderState();
}

class _BrightnessSliderState extends State<BrightnessSlider> {
  late double _value;
  Timer? _debounce;

  @override
  void initState() {
    super.initState();
    _value = widget.value.toDouble();
  }

  @override
  void didUpdateWidget(BrightnessSlider oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.value != widget.value) {
      _value = widget.value.toDouble();
    }
  }

  @override
  void dispose() {
    _debounce?.cancel();
    super.dispose();
  }

  void _onChanged(double value) {
    setState(() => _value = value);

    _debounce?.cancel();
    _debounce = Timer(const Duration(milliseconds: 300), () {
      widget.onChanged(value.round());
    });
  }

  @override
  Widget build(BuildContext context) {
    return Row(
      children: [
        Icon(widget.icon, size: 20, color: Colors.grey[400]),
        const SizedBox(width: 12),
        Text(widget.label, style: TextStyle(color: Colors.grey[400])),
        Expanded(
          child: Slider(value: _value, min: 0, max: 255, onChanged: _onChanged),
        ),
        SizedBox(
          width: 40,
          child: Text(
            _value.round().toString(),
            textAlign: TextAlign.right,
            style: const TextStyle(fontWeight: FontWeight.w500),
          ),
        ),
      ],
    );
  }
}
