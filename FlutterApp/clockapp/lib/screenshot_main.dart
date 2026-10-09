// TEMPORANEO: entry point per generare gli screenshot dell'App Store.
import 'dart:async';
import 'package:flutter/material.dart';
import 'main.dart';
import 'screens/demo_home_screen.dart';
import 'screens/device_discovery_screen.dart';
import 'services/demo_service.dart';

const _offsets = [0.0, 700.0, 1400.0, 2100.0, 99999.0];

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
  final _scroll = ScrollController();
  int _scene = 0; // 0 = discovery, poi demo home ai vari offset

  @override
  void initState() {
    super.initState();
    Timer.periodic(const Duration(seconds: 6), (t) {
      if (_scene > _offsets.length) return t.cancel();
      setState(() => _scene++);
      if (_scene == 1) DemoService().startDemo();
      if (_scene >= 1) {
        WidgetsBinding.instance.addPostFrameCallback((_) {
          if (!_scroll.hasClients) return;
          final max = _scroll.position.maxScrollExtent;
          final o = _offsets[(_scene - 1).clamp(0, _offsets.length - 1)];
          _scroll.jumpTo(o > max ? max : o);
        });
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    final app = const LedMatrixApp().build(context) as MaterialApp;
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      theme: app.theme,
      home: _scene == 0
          ? const DeviceDiscoveryScreen()
          : PrimaryScrollController(
              controller: _scroll,
              child: const DemoHomeScreen(),
            ),
    );
  }
}
