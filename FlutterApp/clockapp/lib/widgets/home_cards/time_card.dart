import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../../services/device_service.dart';
import '../common/common_widgets.dart';

class TimeCard extends StatelessWidget {
  final DeviceService device;
  final DeviceStatus? status;
  final VoidCallback onSync;
  final VoidCallback onShowPicker;

  const TimeCard({
    super.key,
    required this.device,
    required this.status,
    required this.onSync,
    required this.onShowPicker,
  });

  @override
  Widget build(BuildContext context) {
    return GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(Icons.access_time, color: Theme.of(context).colorScheme.primary),
              const SizedBox(width: 12),
              const Text('Ora', style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600)),
            ],
          ),
          const SizedBox(height: 16),
          Row(
            children: [
              Expanded(
                child: ActionButton(
                  icon: Icons.sync,
                  label: 'Sincronizza',
                  onTap: () {
                    onSync();
                    HapticFeedback.lightImpact();
                  },
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: ActionButton(
                  icon: Icons.schedule,
                  label: 'Imposta',
                  onTap: onShowPicker,
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }
}
