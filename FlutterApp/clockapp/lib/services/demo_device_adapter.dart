import 'dart:async';
import 'device_service.dart';
import 'demo_service.dart';
import 'pong_device_interface.dart';

/// Adapter che permette al DemoService di essere usato come DeviceService
/// per il PongControllerScreen
class DemoDeviceAdapter implements IPongDevice {
  final DemoService _demoService;

  DemoDeviceAdapter(this._demoService);

  @override
  Stream<PongState> get pongStream => _demoService.pongStream;

  @override
  void pongJoin(int player) => _demoService.pongJoin(player);

  @override
  void pongLeave(int player) => _demoService.pongLeave(player);

  @override
  void pongMove(int player, String direction) => _demoService.pongMove(player, direction);

  @override
  void pongSetPosition(int player, int percentage) => _demoService.pongSetPosition(player, percentage);

  @override
  void pongStart() => _demoService.pongStart();

  @override
  void pongPause() => _demoService.pongPause();

  @override
  void pongResume() => _demoService.pongResume();

  @override
  void pongReset() => _demoService.pongReset();

  @override
  void pongGetState() => _demoService.pongGetState();
}
