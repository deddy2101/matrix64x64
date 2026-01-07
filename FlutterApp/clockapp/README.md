# ClockApp - LED Matrix Controller

App Flutter per il controllo remoto della matrice LED 64x64 tramite WiFi.

## Caratteristiche

- **Discovery Automatico**: Trova automaticamente la matrice LED sulla rete locale tramite mDNS
- **Controllo Real-time**: WebSocket per controllo immediato degli effetti
- **Gestione Effetti**: Selezione e configurazione di vari effetti visivi
- **Aggiornamenti OTA**: Upload e installazione firmware direttamente dall'app
- **Impostazioni Persistenti**: Luminosità, testo personalizzato, configurazioni salvate
- **Multi-piattaforma**: Supporto Android, iOS e macOS

## Screenshot

L'app fornisce un'interfaccia intuitiva per:
- Selezionare effetti visivi (Pong, Mario Clock, Plasma, etc.)
- Regolare la luminosità in tempo reale
- Inserire testo personalizzato per lo scroll
- Gestire le impostazioni WiFi del dispositivo
- Aggiornare il firmware via OTA

## Requisiti

- Flutter SDK 3.10.0 o superiore
- Dispositivo mobile/desktop sulla stessa rete della matrice LED
- Matrice LED configurata e connessa alla rete

## Installazione

### 1. Clona il repository

```bash
cd FlutterApp/clockapp
```

### 2. Installa le dipendenze

```bash
flutter pub get
```

### 3. Genera le icone (opzionale)

```bash
flutter pub run flutter_launcher_icons
```

## Esecuzione

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

## Dipendenze Principali

### Networking
- `web_socket_channel` (^2.4.0): Comunicazione WebSocket real-time
- `http` (^1.2.0): Chiamate REST API per configurazioni e OTA
- `connectivity_plus` (^5.0.2): Monitoraggio stato connessione

### Funzionalità
- `google_fonts` (^6.1.0): Font personalizzati per UI
- `file_picker` (^8.0.0): Selezione file firmware per OTA
- `crypto` (^3.0.3): Hash MD5 per verifica firmware
- `permission_handler` (^11.0.1): Gestione permessi di sistema
- `device_info_plus` (^9.1.1): Informazioni sul dispositivo
- `package_info_plus` (^8.0.0): Informazioni sulla versione dell'app
- `path_provider` (^2.1.0): Accesso alle directory di sistema

## Struttura del Progetto

```
lib/
├── main.dart              # Entry point dell'app
├── screens/               # Schermate principali
│   ├── home_screen.dart   # Schermata principale controllo
│   └── settings_screen.dart
├── widgets/               # Widget riutilizzabili
│   ├── effect_card.dart   # Card per selezione effetti
│   └── connection_status.dart
├── services/              # Servizi di comunicazione
│   ├── websocket_service.dart
│   ├── api_service.dart
│   └── discovery_service.dart
├── models/                # Modelli dati
│   └── device_info.dart
├── dialogs/               # Dialog personalizzati
│   └── ota_dialog.dart
└── utils/                 # Utility e helpers
```

## Configurazione

### Connessione alla Matrice LED

L'app utilizza mDNS per trovare automaticamente il dispositivo con hostname `led-matrix.local`. Se il discovery automatico non funziona, è possibile inserire manualmente l'indirizzo IP.

### Protocollo Comandi

L'app comunica con l'ESP32 tramite WebSocket usando comandi CSV:

```
# Cambia effetto
EFFECT,pong

# Imposta luminosità (0-255)
BRIGHTNESS,128

# Mostra testo scrolling
TEXT,Hello World

# Ottieni stato
STATUS
```

## Funzionalità Principali

### Discovery e Connessione

```dart
// Discovery automatico tramite mDNS
final service = DiscoveryService();
final device = await service.findDevice();

// Connessione WebSocket
await service.connect(device.address);
```

### Controllo Effetti

```dart
// Cambia effetto
await websocketService.sendCommand('EFFECT,mario_clock');

// Regola luminosità
await websocketService.sendCommand('BRIGHTNESS,200');

// Testo personalizzato
await websocketService.sendCommand('TEXT,Custom Message');
```

### Aggiornamento OTA

```dart
// Upload firmware
final file = await FilePicker.platform.pickFiles();
await apiService.uploadFirmware(file, deviceAddress);

// Verifica aggiornamento
await apiService.checkOTAStatus(deviceAddress);
```

## Gestione Permessi

L'app richiede i seguenti permessi:

**Android:**
- Internet (per comunicazione rete)
- Storage (per selezione file firmware)
- Network State (per controllo connessione)

**iOS:**
- Local Network (per mDNS discovery)
- Storage (per selezione file)

## Icone e Branding

L'app utilizza icone personalizzate configurate in [pubspec.yaml](pubspec.yaml):
- Icona principale: `assets/icon.png`
- Adaptive icon background: `#1a1a2e`

Per rigenerare le icone:
```bash
flutter pub run flutter_launcher_icons
```

## Troubleshooting

### Device non trovato
1. Verifica che il dispositivo mobile e la matrice LED siano sulla stessa rete WiFi
2. Controlla che mDNS sia supportato dal router
3. Prova a connetterti manualmente inserendo l'IP della matrice LED

### WebSocket non si connette
1. Verifica che la matrice LED sia accesa e connessa
2. Controlla i log dell'ESP32 per errori
3. Assicurati che la porta WebSocket (80) non sia bloccata

### OTA fallisce
1. Verifica che il file firmware sia valido (.bin)
2. Assicurati che ci sia spazio sufficiente sull'ESP32
3. Controlla che la connessione sia stabile durante l'upload

### Permessi negati
1. Vai nelle impostazioni del dispositivo
2. Trova l'app ClockApp
3. Abilita i permessi necessari (Storage, Network)

## Build Configurazioni

### Android
Configurato per:
- Min SDK: 21 (Android 5.0)
- Target SDK: 34 (Android 14)
- Adaptive icons per diverse versioni Android

### iOS
Configurato per:
- iOS 12.0+
- Info.plist con permessi Local Network e Bonjour services

### macOS
Configurato per:
- macOS 10.14+
- Network entitlements abilitati

## Sviluppo

### Debug

```bash
# Run con hot reload
flutter run

# Run con logging verboso
flutter run -v

# Profiling performance
flutter run --profile
```

### Testing

```bash
# Run tutti i test
flutter test

# Test coverage
flutter test --coverage
```

## Versioning

Versione corrente: **1.1.0+5**

- Major: 1 (versione principale)
- Minor: 1 (nuove funzionalità)
- Patch: 0 (bug fix)
- Build: 5 (numero build interno)

## Contribuire

Per contribuire al progetto:
1. Fork il repository
2. Crea un branch per la feature
3. Commit le modifiche
4. Push e crea una Pull Request

## Licenza

Questo progetto è parte del sistema Matrix 64x64 LED Display.

## Supporto

Per problemi o domande, controlla:
- [README principale del progetto](../../README.md)
- [README ESP32 Firmware](../../ESPCode/README.md)
- Issue tracker su GitHub
