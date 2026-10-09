import 'package:flutter/material.dart';
import '../../services/ble_link.dart';
import '../common/signal_indicator.dart';

/// Card per dispositivo Bluetooth trovato
class BleDeviceCard extends StatelessWidget {
  final BleScanResult device;
  final VoidCallback onTap;

  const BleDeviceCard({
    super.key,
    required this.device,
    required this.onTap,
  });

  /// RSSI (dBm) → 0..100: -100 o meno = 0, -40 o più = 100
  int get _strength {
    final rssi = device.rssi;
    if (rssi == null) return 0;
    return (((rssi + 100) * 100) / 60).round().clamp(0, 100);
  }

  @override
  Widget build(BuildContext context) {
    return Card(
      margin: const EdgeInsets.only(bottom: 12),
      color: const Color(0xFF121218),
      elevation: 2,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(12),
        side: BorderSide(color: Colors.grey.withOpacity(0.2)),
      ),
      child: InkWell(
        onTap: onTap,
        borderRadius: BorderRadius.circular(12),
        child: Padding(
          padding: const EdgeInsets.all(16),
          child: Row(
            children: [
              Container(
                width: 56,
                height: 56,
                decoration: BoxDecoration(
                  color: const Color(0xFF8B5CF6).withOpacity(0.2),
                  borderRadius: BorderRadius.circular(12),
                ),
                child: const Icon(
                  Icons.bluetooth,
                  color: Color(0xFF8B5CF6),
                  size: 28,
                ),
              ),
              const SizedBox(width: 16),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      device.name,
                      style: const TextStyle(
                        fontSize: 18,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    const SizedBox(height: 6),
                    if (device.rssi != null)
                      SignalIndicator(strength: _strength)
                    else
                      Text(
                        'Bluetooth',
                        style: TextStyle(color: Colors.grey[500], fontSize: 12),
                      ),
                  ],
                ),
              ),
              const Icon(
                Icons.arrow_forward_ios,
                size: 20,
                color: Color(0xFF8B5CF6),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
