import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../../services/device_service.dart';
import '../common/common_widgets.dart';

class ConnectionCard extends StatelessWidget {
  final DeviceService device;
  final bool isConnected;
  final DeviceStatus? status;
  final VoidCallback onDisconnect;
  final VoidCallback onRefresh;

  const ConnectionCard({
    super.key,
    required this.device,
    required this.isConnected,
    required this.status,
    required this.onDisconnect,
    required this.onRefresh,
  });

  @override
  Widget build(BuildContext context) {
    return GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(
                device.connectionType == ConnectionType.websocket
                    ? Icons.wifi
                    : Icons.usb_rounded,
                color: Theme.of(context).colorScheme.primary,
              ),
              const SizedBox(width: 12),
              const Text(
                'Connessione',
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
              ),
              const Spacer(),
              if (isConnected && device.connectionType == ConnectionType.websocket)
                Text(
                  status?.ip ?? '',
                  style: TextStyle(color: Colors.grey[400], fontSize: 12),
                ),
            ],
          ),
          const SizedBox(height: 16),
          Row(
            children: [
              Expanded(
                child: ActionButton(
                  icon: isConnected ? Icons.link_off : Icons.link,
                  label: isConnected ? 'Disconnetti' : 'Connetti',
                  color: isConnected ? Colors.red[400]! : Colors.green[400]!,
                  onTap: onDisconnect,
                ),
              ),
              if (isConnected) ...[
                const SizedBox(width: 12),
                Expanded(
                  child: ActionButton(
                    icon: Icons.refresh,
                    label: 'Refresh',
                    onTap: () {
                      onRefresh();
                      HapticFeedback.lightImpact();
                    },
                  ),
                ),
              ],
            ],
          ),
        ],
      ),
    );
  }
}
