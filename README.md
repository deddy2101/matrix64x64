# Matrix 64x64 LED Display System

ESP32-based LED matrix controller with Flutter mobile app and OTA firmware server.

## Overview

WiFi-controllable 64x64 LED matrix display system with visual effects, real-time clock support, and remote management via mobile application.

### Project Structure

```
matrix64x64/
├── ESPCode/          # ESP32 firmware
├── FlutterApp/       # Flutter mobile app
└── firmware-server/  # OTA firmware server
```

## Features

- **64x64 LED Matrix**: HUB75 matrix control via ESP32 DMA
- **Visual Effects**: Pong, Mario Clock, Plasma, Scroll Text, Image Display
- **WiFi Control**: WebSocket and REST API
- **Mobile App**: Flutter app for Android/iOS
- **OTA Updates**: Over-the-air firmware updates
- **UDP Discovery**: Automatic device discovery via UDP broadcast
- **AP Fallback**: Automatic Access Point mode when WiFi unavailable
- **Persistent Settings**: Configuration stored in flash memory

## Hardware Requirements

- **ESP32 DevKit v1** or compatible
- **HUB75 64x64 LED Matrix**
- **5V Power Supply** (adequate for LED matrix current draw)
- **DS3231 RTC Module** (optional)

## Quick Start

### ESP32 Firmware

```bash
cd ESPCode
# Configure WiFi credentials in Settings.cpp
pio run -t upload
```

### Flutter App

```bash
cd FlutterApp/clockapp
flutter pub get
flutter run
```

### OTA Firmware Server (Optional)

```bash
cd firmware-server
docker-compose up -d
```

## Configuration

### Device Discovery

UDP broadcast on port 7890. Device responds with:
- Device name
- IP address
- WebSocket service port

### WiFi Modes

- **Station Mode**: Connects to configured WiFi network
- **Access Point Mode**: Automatic fallback when WiFi unavailable
  - SSID: Configured device name
  - Password: `ledmatrix123`
  - IP: `192.168.4.1`

## Architecture

### Communication

- **WebSocket**: Real-time effect control
- **REST API**: Configuration and settings
- **UDP Discovery**: Automatic device discovery (port 7890)
- **OTA**: HTTP-based firmware updates

### Command Protocol

CSV-based WebSocket commands:
```
EFFECT,effect_name,parameters...
BRIGHTNESS,value
TEXT,message
```

## Documentation

- [ESPCode README](ESPCode/README.md) - ESP32 firmware
- [FlutterApp README](FlutterApp/clockapp/README.md) - Mobile app
- [Firmware Server README](firmware-server/README.md) - OTA server

## Development

### Requirements

- **PlatformIO**: ESP32 firmware compilation
- **Flutter SDK**: Mobile app (3.10.0+)
- **Docker**: OTA server (optional)

### Build

```bash
# ESP32 Firmware
cd ESPCode && pio run

# Flutter App
cd FlutterApp/clockapp && flutter build apk

# Firmware Server
cd firmware-server && docker-compose build
```

## License

Open source software.

## Version

- **ESP32 Firmware**: v1.0
- **Flutter App**: v1.1.0+5
- **Last Update**: January 2026
