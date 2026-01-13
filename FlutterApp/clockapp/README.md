# ClockApp - LED Matrix Controller

Flutter mobile app for controlling 64x64 LED matrix via WiFi.

## Features

- **Automatic Discovery**: UDP broadcast device discovery
- **Real-time Control**: WebSocket-based effect control
- **Effect Management**: Visual effect selection and configuration
- **OTA Updates**: Direct firmware upload from app
- **Manual Connection**: IP-based connection for AP mode
- **Persistent Settings**: Brightness, custom text, configurations
- **Multi-platform**: Android, iOS, and macOS support

## Requirements

- Flutter SDK 3.10.0+
- Mobile device on same network as LED matrix
- LED matrix configured and connected to network

## Installation

### Clone Repository

```bash
cd FlutterApp/clockapp
```

### Install Dependencies

```bash
flutter pub get
```

### Generate Icons (Optional)

```bash
flutter pub run flutter_launcher_icons
```

## Running

### Development

```bash
flutter run
```

### Build Release

**Android APK:**
```bash
flutter build apk --release
```

**iOS:**
```bash
flutter build ios --release
```

**macOS:**
```bash
flutter build macos --release
```

## Dependencies

### Networking
- `web_socket_channel` (^2.4.0): WebSocket real-time communication
- `http` (^1.2.0): REST API calls for configuration and OTA
- `connectivity_plus` (^5.0.2): Connection state monitoring

### Functionality
- `google_fonts` (^6.1.0): Custom UI fonts
- `file_picker` (^8.0.0): Firmware file selection for OTA
- `crypto` (^3.0.3): MD5 hash for firmware verification
- `permission_handler` (^11.0.1): System permission management
- `device_info_plus` (^9.1.1): Device information
- `package_info_plus` (^8.0.0): App version info
- `path_provider` (^2.1.0): System directory access

## Project Structure

```
lib/
├── main.dart              # App entry point
├── screens/               # Main screens
│   ├── home_screen.dart   # Main control screen
│   └── settings_screen.dart
├── widgets/               # Reusable widgets
│   ├── effect_card.dart   # Effect selection cards
│   └── connection_status.dart
├── services/              # Communication services
│   ├── websocket_service.dart
│   ├── api_service.dart
│   └── discovery_service.dart
├── models/                # Data models
│   └── device_info.dart
├── dialogs/               # Custom dialogs
│   └── ota_dialog.dart
└── utils/                 # Utilities and helpers
```

## Configuration

### LED Matrix Connection

UDP broadcast on port 7890 for automatic device discovery.

**Automatic Discovery:**
1. App sends UDP broadcast `LEDMATRIX_DISCOVER`
2. Device responds with `LEDMATRIX_HERE,name,ip,port`
3. App connects automatically to WebSocket

**Manual Connection:**
For AP mode or discovery failure:
1. Connect mobile device to ESP32 AP
2. Manually enter IP `192.168.4.1` in app
3. AP password: `ledmatrix123`

### Command Protocol

WebSocket communication using CSV commands:

```
# Change effect
EFFECT,pong

# Set brightness (0-255)
BRIGHTNESS,128

# Display scrolling text
TEXT,Hello World

# Get status
STATUS
```

## Main Features

### Discovery and Connection

```dart
// Automatic discovery via UDP broadcast
final service = DiscoveryService();
await service.startDiscovery();

// Response: LEDMATRIX_HERE,name,192.168.1.100,80
final device = service.discoveredDevice;

// WebSocket connection
await websocketService.connect(device.ip, device.port);
```

### Effect Control

```dart
// Change effect
await websocketService.sendCommand('EFFECT,mario_clock');

// Adjust brightness
await websocketService.sendCommand('BRIGHTNESS,200');

// Custom text
await websocketService.sendCommand('TEXT,Custom Message');
```

### OTA Update

```dart
// Upload firmware
final file = await FilePicker.platform.pickFiles();
await apiService.uploadFirmware(file, deviceAddress);

// Check update status
await apiService.checkOTAStatus(deviceAddress);
```

## Permissions

Required permissions:

**Android:**
- Internet (network communication)
- Storage (firmware file selection)
- Network State (connection monitoring)

**iOS:**
- Local Network (UDP discovery)
- Storage (file selection)

## Icons and Branding

Custom icons configured in [pubspec.yaml](pubspec.yaml):
- Main icon: `assets/icon.png`
- Adaptive icon background: `#1a1a2e`

Regenerate icons:
```bash
flutter pub run flutter_launcher_icons
```

## Build Configurations

### Android
- Min SDK: 21 (Android 5.0)
- Target SDK: 34 (Android 14)
- Adaptive icons for various Android versions

### iOS
- iOS 12.0+
- Info.plist with Local Network permissions for UDP broadcast

### macOS
- macOS 10.14+
- Network entitlements enabled

## Development

### Debug

```bash
# Run with hot reload
flutter run

# Run with verbose logging
flutter run -v

# Performance profiling
flutter run --profile
```

### Testing

```bash
# Run all tests
flutter test

# Test coverage
flutter test --coverage
```

## Version

Current version: **1.1.0+5**

- Major: 1 (main version)
- Minor: 1 (new features)
- Patch: 0 (bug fixes)
- Build: 5 (internal build number)

## License

Part of Matrix 64x64 LED Display System.

## Support

For issues or questions, check:
- [Main project README](../../README.md)
- [ESP32 Firmware README](../../ESPCode/README.md)
- GitHub issue tracker
