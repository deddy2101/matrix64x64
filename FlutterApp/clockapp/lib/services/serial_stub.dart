// Stub per piattaforme non-desktop (Android, iOS, Web)
// La seriale non è disponibile su queste piattaforme

import 'dart:async';
import 'dart:typed_data';
import 'device_service.dart';

List<SerialDevice> getAvailableDevices() {
  return [];
}

Future<Map<String, dynamic>?> connect(
  String port, {
  int baudRate = 115200,
  void Function(Uint8List)? onData,
  void Function(dynamic)? onError,
  void Function()? onDone,
}) async {
  return null;
}

void closePort(dynamic port, dynamic reader) {
  // No-op
}

void write(dynamic port, String data) {
  // No-op
}
