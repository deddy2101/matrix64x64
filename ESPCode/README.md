# ESP32 LED Matrix Firmware

ESP32 firmware for HUB75 64x64 LED matrix with visual effects, WiFi control, and OTA updates.

## Features

- **HUB75 Display**: 64x64 LED matrix control via DMA library
- **Visual Effects**:
  - Pong Game
  - Mario Clock
  - Plasma Effect
  - Scroll Text
  - Image Display (Andre, Mario, Pokemon, Luigi, Fox)
- **WiFi**: Station mode with automatic AP fallback
- **WebSocket Server**: Real-time control via CSV commands
- **REST API**: Configuration and OTA endpoints
- **UDP Discovery**: Automatic device discovery (port 7890)
- **RTC Support**: DS3231 real-time clock module
- **OTA Updates**: Over-the-air firmware updates
- **Persistent Settings**: Flash storage for configurations

## Hardware

### Components

- **ESP32 DOIT DevKit v1** or compatible
- **HUB75 64x64 LED Matrix** (P3 or P4)
- **5V Power Supply** (10-20A depending on matrix)
- **DS3231 RTC Module** (optional)

### HUB75 Pinout

```cpp
R1:  25    G1:  26    B1:  27
R2:  14    G2:  12    B2:  13
A:   23    B:   19    C:   5
D:   17    E:   32    (E pin for 64-row matrix)
LAT: 4     OE:  15    CLK: 16
```

## Setup

### Installation

```bash
cd ESPCode
pio lib install
pio run
pio run -t upload
pio device monitor
```

### WiFi Configuration

Edit [src/Settings.cpp](src/Settings.cpp):

```cpp
void Settings::loadDefaults() {
    ssid = "YOUR_SSID";
    password = "YOUR_PASSWORD";
    hostname = "led-matrix";
    // ...
}
```

## Libraries

Defined in [platformio.ini](platformio.ini):

- `FastLED`: LED color and effects
- `ESP32 HUB75 LED MATRIX PANEL DMA Display` (^3.0.14): HUB75 driver with DMA
- `Adafruit GFX Library` (^1.12.4): Graphics library
- `RTClib` (^2.1.4): DS3231 RTC management
- `ESPAsyncWebServer` (^1.2.4): Async web server
- `AsyncTCP` (^1.1.1): Async TCP stack

## Project Structure

```
ESPCode/
├── src/
│   ├── main.cpp                    # Main entry point
│   ├── DisplayManager.cpp/h        # LED matrix hardware management
│   ├── EffectManager.cpp/h         # Effect system
│   ├── TimeManager.cpp/h           # RTC and clock management
│   ├── Settings.cpp/h              # Persistent configurations
│   ├── WiFiManager.cpp/h           # WiFi connection management
│   ├── WebServerManager.cpp/h      # HTTP/REST API server
│   ├── WebSocketManager.cpp/h      # WebSocket server
│   ├── CommandHandler.cpp/h        # CSV command parser
│   ├── Discovery.cpp/h             # UDP Discovery service
│   ├── Debug.h                     # Debug macros
│   ├── effects/                    # Visual effects
│   │   ├── Effect.h                # Base effect class
│   │   ├── PongEffect.cpp/h
│   │   ├── MarioClockEffect.cpp/h
│   │   ├── PlasmaEffect.cpp/h
│   │   ├── ScrollTextEffect.cpp/h
│   │   └── ImageEffect.cpp/h
│   └── images/                     # Image headers
│       ├── andre.h
│       ├── mario.h
│       ├── pokemon.h
│       ├── luigi.h
│       └── fox.h
├── platformio.ini                  # PlatformIO configuration
└── README.md
```

## Command Protocol

WebSocket CSV commands on port 80.

### Format

```
COMMAND[,param1,param2,...]
```

### Available Commands

#### Effect Control

```
EFFECT,effect_name
```

Available effects:
- `pong` - Animated Pong game
- `mario_clock` - Mario animated clock
- `plasma` - Colorful plasma effect
- `text` - Scrolling text
- `image` - Static image display

#### Display Control

```
BRIGHTNESS,value        # 0-255
TEXT,message           # Set text for scroll effect
COLOR,r,g,b           # RGB color 0-255
CLEAR                 # Clear display
```

#### System Management

```
STATUS                 # Get system status (JSON)
RESTART               # Restart ESP32
TIME,HH,MM,SS         # Set RTC time
DATE,YYYY,MM,DD       # Set RTC date
```

#### WiFi Configuration

```
WIFI_SCAN              # Scan WiFi networks
WIFI_CONNECT,ssid,pass # Connect to network
WIFI_AP                # Access Point mode
WIFI_STATUS            # Connection status
```

### Example Session

```bash
# Connect via WebSocket to ws://device-ip

> EFFECT,pong
< OK

> BRIGHTNESS,150
< OK

> TEXT,Hello World!
< OK

> EFFECT,text
< OK

> STATUS
< {"effect":"text","brightness":150,"wifi":"connected","ip":"192.168.1.100"}
```

## Device Discovery

UDP broadcast for automatic device discovery by Flutter app.

### Discovery Protocol

**UDP Port**: 7890

**Request** (from app):
```
LEDMATRIX_DISCOVER
```

**Response** (from device):
```
LEDMATRIX_HERE,device_name,192.168.1.100,80
```

Format: `LEDMATRIX_HERE,<name>,<ip>,<service_port>`

## WiFi Modes

### Station Mode (Primary)
Connects to configured WiFi network. Automatic fallback to AP mode on failure.

### Access Point Mode (Fallback)
- **SSID**: Configured device name (default: unique name)
- **Password**: `ledmatrix123`
- **IP**: `192.168.4.1`
- **Gateway/Subnet**: 192.168.4.1 / 255.255.255.0

Manual connection:
1. Connect mobile device to ESP32 AP
2. Use IP `192.168.4.1` in app
3. Connect to WebSocket on port 80

## REST API Endpoints

HTTP server on port 80.

### GET /

Web control page (HTML)

### GET /status

```json
{
  "effect": "pong",
  "brightness": 128,
  "wifi_ssid": "MyNetwork",
  "wifi_ip": "192.168.1.100",
  "uptime": 3600,
  "free_heap": 150000
}
```

### POST /brightness

```json
{
  "value": 200
}
```

### POST /effect

```json
{
  "name": "mario_clock"
}
```

### POST /text

```json
{
  "message": "Custom Text"
}
```

### POST /update (OTA)

Upload firmware `.bin` file as multipart/form-data.

```bash
curl -X POST -F "firmware=@firmware.bin" http://device-ip/update
```

## Advanced Configuration

### Matrix Dimensions

In [src/main.cpp](src/main.cpp):

```cpp
#define PANEL_WIDTH 64      // Panel width
#define PANEL_HEIGHT 64     // Panel height
#define PANELS_NUMBER 1     # Number of chained panels
#define PIN_E 32            // E pin for >32 row matrices
```

### Performance Optimization

In [platformio.ini](platformio.ini):

```ini
build_flags =
    -DCORE_DEBUG_LEVEL=0    # Disable debug logs
    -Os                     # Optimize for size
```

Change `CORE_DEBUG_LEVEL` to 3-5 for more debug output.

### Persistent Settings

Settings saved in flash EEPROM:
- WiFi credentials
- Hostname
- Default brightness
- Startup effect
- Custom text

## Adding New Effects

### 1. Create Effect Class

```cpp
// src/effects/MyEffect.h
#ifndef MY_EFFECT_H
#define MY_EFFECT_H

#include "Effect.h"

class MyEffect : public Effect {
public:
    MyEffect(DisplayManager* dm);
    void init() override;
    void update() override;
    const char* getName() const override { return "my_effect"; }
};

#endif
```

### 2. Implement Update

```cpp
// src/effects/MyEffect.cpp
#include "MyEffect.h"

MyEffect::MyEffect(DisplayManager* dm) : Effect(dm) {}

void MyEffect::init() {
    // Initialization
}

void MyEffect::update() {
    // Frame rendering logic
    display->clearScreen();
    // ... draw on display
}
```

### 3. Register in EffectManager

In [src/main.cpp](src/main.cpp):

```cpp
#include "effects/MyEffect.h"

// In setup()
effectManager->registerEffect(new MyEffect(displayManager));
```

## UDP Discovery Configuration

Discovery service on port 7890:

```cpp
// src/Discovery.cpp
#define DISCOVERY_PORT 7890
#define DISCOVERY_MAGIC "LEDMATRIX_DISCOVER"
#define DISCOVERY_RESPONSE "LEDMATRIX_HERE"
```

Device responds to UDP broadcast with:
- Configured device name
- Current IP address (STA or AP)
- WebSocket service port (default 80)

## Debug and Monitoring

### Serial Monitor

```bash
pio device monitor -b 115200
```

Output includes:
- WiFi connection
- Received commands
- Errors/warnings
- Performance statistics

### Debug Macros

Use macros in [src/Debug.h](src/Debug.h):

```cpp
DEBUG_PRINT("Message");
DEBUG_PRINTLN(variable);
DEBUG_PRINTF("Value: %d\n", val);
```

## OTA Updates

### From Browser

1. Go to `http://device-ip`
2. Click OTA/Update section
3. Select `.bin` file
4. Wait for completion

### From Flutter App

1. Open ClockApp
2. Go to Settings
3. Select "Update Firmware"
4. Choose firmware file
5. Confirm upload

### From Command Line

```bash
# Build firmware
pio run

# Upload via OTA
curl -F "firmware=@.pio/build/esp32doit-devkit-v1/firmware.bin" \
     http://device-ip/update
```

## Versioning

Version management:

```cpp
// In main.cpp
const char* FIRMWARE_VERSION = "1.0.0";
```

Include version in `/status` response.

## License

Part of Matrix 64x64 LED Display System project.

## References

- [ESP32 HUB75 DMA Library](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA)
- [FastLED Documentation](http://fastled.io/)
- [PlatformIO Docs](https://docs.platformio.org/)
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)
