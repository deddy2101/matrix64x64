# Matrix 64x64 LED Display System

Sistema completo per la gestione di una matrice LED 64x64 con ESP32, interfaccia Flutter e server OTA per aggiornamenti firmware.

## Panoramica del Progetto

Questo progetto implementa un sistema di display LED controllabile tramite WiFi, con supporto per vari effetti visivi, orologio in tempo reale e gestione remota tramite app mobile.

### Componenti Principali

```
matrix64x64/
├── ESPCode/          # Firmware ESP32 per la matrice LED
├── FlutterApp/       # App mobile per il controllo
└── firmware-server/  # Server OTA per aggiornamenti firmware
```

## Caratteristiche

- **Display LED 64x64**: Gestione matrice HUB75 tramite ESP32
- **Effetti Visivi**: Pong, Mario Clock, Plasma, Scroll Text, Image Display
- **Controllo WiFi**: WebSocket e API REST per controllo remoto
- **App Mobile**: Interfaccia Flutter per Android/iOS
- **OTA Updates**: Aggiornamenti firmware over-the-air
- **mDNS Discovery**: Rilevamento automatico del dispositivo in rete
- **Persistent Settings**: Salvataggio permanente delle configurazioni

## Hardware Richiesto

- **ESP32 DevKit v1** (o compatibile)
- **Matrice LED HUB75 64x64**
- **Alimentatore 5V** (adeguato alla matrice LED)
- **RTC DS3231** (opzionale, per orologio in tempo reale)

## Quick Start

### 1. Setup ESP32

```bash
cd ESPCode
# Configura le credenziali WiFi in Settings.cpp
pio run -t upload
```

### 2. Setup App Flutter

```bash
cd FlutterApp/clockapp
flutter pub get
flutter run
```

### 3. Setup Server OTA (opzionale)

```bash
cd firmware-server
docker-compose up -d
```

## Configurazione

### Credenziali WiFi

Modifica il file [ESPCode/src/Settings.cpp](ESPCode/src/Settings.cpp):
```cpp
ssid = "TUO_SSID";
password = "TUA_PASSWORD";
```

### Hostname mDNS

Default: `led-matrix.local`

## Architettura

### Comunicazione

- **WebSocket**: Controllo in tempo reale degli effetti
- **REST API**: Gestione configurazioni e impostazioni
- **mDNS**: Discovery automatico del dispositivo
- **OTA**: Aggiornamenti firmware HTTP

### Protocollo Comandi

Il sistema utilizza un protocollo CSV per i comandi WebSocket:
```
EFFECT,nome_effetto,parametri...
BRIGHTNESS,valore
TEXT,messaggio
```

## Documentazione

Per informazioni dettagliate su ogni componente:

- [ESPCode README](ESPCode/README.md) - Firmware ESP32
- [FlutterApp README](FlutterApp/clockapp/README.md) - App mobile
- [Firmware Server README](firmware-server/README.md) - Server OTA

## Sviluppo

### Requisiti

- **PlatformIO**: Per compilare il firmware ESP32
- **Flutter SDK**: Per l'app mobile (3.10.0+)
- **Docker**: Per il server OTA (opzionale)

### Build

```bash
# ESP32 Firmware
cd ESPCode && pio run

# Flutter App
cd FlutterApp/clockapp && flutter build apk

# Firmware Server
cd firmware-server && docker-compose build
```

## Troubleshooting

### ESP32 non si connette al WiFi
- Verifica le credenziali in `Settings.cpp`
- Controlla che il router supporti 2.4GHz
- Verifica il serial monitor per errori: `pio device monitor`

### App non trova il dispositivo
- Assicurati che il dispositivo mobile sia sulla stessa rete
- Verifica che mDNS sia supportato dalla rete
- Prova a connetterti manualmente tramite IP

### Display LED non funziona
- Verifica i collegamenti HUB75
- Controlla l'alimentazione (5V adeguata)
- Verifica la configurazione pin in `main.cpp`

## Licenza

Questo progetto è distribuito come software open source.

## Autore

Progetto sviluppato per la gestione di display LED Matrix con ESP32.

## Versione

- **ESP32 Firmware**: v1.0
- **Flutter App**: v1.1.0+5
- **Ultimo aggiornamento**: Gennaio 2026
