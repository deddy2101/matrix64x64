// TEMPORANEO: entry point per generare gli screenshot dell'App Store.
import 'dart:async';
import 'package:flutter/material.dart';
import 'main.dart';
import 'screens/game_controller_screen.dart';
import 'screens/home_screen.dart';
import 'services/demo_service.dart';
import 'services/device_service.dart';

const _offsets = [0.0, 650.0, 1300.0];

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(const _ShotsApp());
}

class _ShotsApp extends StatefulWidget {
  const _ShotsApp();
  @override
  State<_ShotsApp> createState() => _ShotsAppState();
}

class _ShotsAppState extends State<_ShotsApp> {
  int _scene = 0;

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) {
      DeviceService().debugSimulateConnection('LED Matrix', [
        'WELCOME,LED Matrix Controller',
        'STATUS,21:37:12,2026/10/09,RTC,1,23.5,Mario Clock,8,60.0,1,14,200,0,'
            'disconnected,,,,86400,182000,1',
        'EFFECTS,${DemoService.demoEffects.join(',')}',
        'SETTINGS,,0,200,30,22,7,10000,1,8,LED Matrix,Ciao!,1,'
            'CET-1CEST,M3.5.0,M10.5.0/3',
      ]);
    });
    Timer.periodic(const Duration(seconds: 8), (t) {
      if (_scene >= _offsets.length) return t.cancel();
      setState(() => _scene++);
      if (_scene == _offsets.length) {
        Future.delayed(const Duration(milliseconds: 800), () {
          DeviceService().debugSimulateConnection('LED Matrix', [
            'PONG_STATE,playing,3,2,human,ai,40,22',
          ]);
        });
      }
      if (_scene >= _offsets.length) return;
      final pos = _findScroll();
      if (pos == null) return;
      final o = _offsets[_scene];
      pos.jumpTo(o > pos.maxScrollExtent ? pos.maxScrollExtent : o);
    });
  }

  /// Primo scrollable verticale della schermata (la ListView della home)
  ScrollPosition? _findScroll() {
    ScrollPosition? found;
    void visit(Element e) {
      if (found != null) return;
      if (e is StatefulElement && e.state is ScrollableState) {
        final st = e.state as ScrollableState;
        if (st.axisDirection == AxisDirection.down) {
          found = st.position;
          return;
        }
      }
      e.visitChildren(visit);
    }
    (context as Element).visitChildren(visit);
    return found;
  }

  @override
  Widget build(BuildContext context) {
    final app = const LedMatrixApp().build(context) as MaterialApp;
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      theme: app.theme,
      home: _scene < _offsets.length
          ? const HomeScreen()
          : GameControllerScreen(deviceService: DeviceService()),
    );
  }
}
