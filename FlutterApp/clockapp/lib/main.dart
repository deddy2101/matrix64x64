import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';
import 'screens/device_discovery_screen.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(const LedMatrixApp());
}

class LedMatrixApp extends StatelessWidget {
  const LedMatrixApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'LED Matrix Controller',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(
          seedColor: const Color(0xFF8B5CF6),
          brightness: Brightness.dark,
          surface: const Color(0xFF121218),
        ),
        scaffoldBackgroundColor: const Color(0xFF0A0A0F),
        useMaterial3: true,
        textTheme: _buildTextTheme(),
      ),
      home: const DeviceDiscoveryScreen(),
    );
  }

  TextTheme _buildTextTheme() {
    try {
      return GoogleFonts.spaceGroteskTextTheme(
        ThemeData.dark().textTheme,
      );
    } catch (e) {
      // Fallback se Google Fonts fallisce
      return ThemeData.dark().textTheme;
    }
  }
}
