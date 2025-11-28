import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';
import 'screens/device_discovery_screen.dart'; // Cambia import
// Rimuovi: import 'screens/home_screen.dart';

void main() {
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
        textTheme: GoogleFonts.spaceGroteskTextTheme(
          ThemeData.dark().textTheme,
        ),
      ),
      home: const DeviceDiscoveryScreen(), // Cambia qui
    );
  }
}
