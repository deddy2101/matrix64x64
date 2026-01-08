import 'dart:async';
import 'device_service.dart';

/// Interface per device che supportano Pong
/// Implementata sia da DeviceService che da DemoDeviceAdapter
abstract class IPongDevice {
  Stream<PongState> get pongStream;
  void pongJoin(int player);
  void pongLeave(int player);
  void pongMove(int player, String direction);
  void pongSetPosition(int player, int percentage);
  void pongStart();
  void pongPause();
  void pongResume();
  void pongReset();
  void pongGetState();
}
