# ESP32 LED Matrix Firmware

Firmware per ESP32 che gestisce una matrice LED HUB75 64x64 con effetti visivi, controllo WiFi e aggiornamenti OTA.

## Caratteristiche

- **Display HUB75**: Gestione matrice LED 64x64 tramite libreria DMA
- **Effetti Visivi Multipli**:
  - Pong Game
  - Mario Clock (orologio animato)
  - Plasma Effect
  - Scroll Text
  - Image Display (Andre, Mario, Pokemon, Luigi, Fox)
- **Connettività WiFi**: Access Point e Station mode
- **WebSocket Server**: Controllo real-time tramite comandi CSV
- **REST API**: Endpoint per configurazione e OTA
- **mDNS**: Discovery automatico come `led-matrix.local`
- **RTC Support**: Orologio in tempo reale con modulo DS3231
- **OTA Updates**: Aggiornamenti firmware over-the-air
- **Persistent Settings**: Configurazioni salvate in memoria flash

## Hardware Requirements

### Componenti Principali

- **ESP32 DOIT DevKit v1** (o compatibile con ESP32)
- **Matrice LED HUB75 64x64** (P3 o P4)
- **Alimentatore 5V** (capacità dipendente dalla matrice, tipicamente 10-20A)
- **Modulo RTC DS3231** (opzionale, per orologio accurato)

### Pinout HUB75

Configurazione standard per ESP32:

```cpp
R1:  25    G1:  26    B1:  27
R2:  14    G2:  12    B2:  13
A:   23    B:   19    C:   5
D:   17    E:   32    (pin E per matrice 64 righe)
LAT: 4     OE:  15    CLK: 16
```

## Setup

### 1. Installazione PlatformIO

```bash
# Installa PlatformIO Core
pip install platformio

# Oppure usa l'estensione VSCode
```

### 2. Clone e Build

```bash
cd ESPCode

# Installa dipendenze
pio lib install

# Compila
pio run

# Upload
pio run -t upload

# Monitor seriale
pio device monitor
```

### 3. Configurazione WiFi

Modifica [src/Settings.cpp](src/Settings.cpp):

```cpp
void Settings::loadDefaults() {
    ssid = "TUO_SSID";
    password = "TUA_PASSWORD";
    hostname = "led-matrix";
    // ...
}
```

## Librerie Utilizzate

Definite in [platformio.ini](platformio.ini):

- `FastLED`: Gestione colori e effetti LED
- `ESP32 HUB75 LED MATRIX PANEL DMA Display` (^3.0.14): Driver matrice HUB75 con DMA
- `Adafruit GFX Library` (^1.12.4): Libreria grafica per testo e forme
- `RTClib` (^2.1.4): Gestione modulo RTC DS3231
- `ESPAsyncWebServer` (^1.2.4): Web server asincrono
- `AsyncTCP` (^1.1.1): Stack TCP asincrono

## Struttura del Progetto

```
ESPCode/
├── src/
│   ├── main.cpp                    # Entry point principale
│   ├── DisplayManager.cpp/h        # Gestione hardware matrice LED
│   ├── EffectManager.cpp/h         # Sistema gestione effetti
│   ├── TimeManager.cpp/h           # Gestione RTC e orologio
│   ├── Settings.cpp/h              # Configurazioni persistenti
│   ├── WiFiManager.cpp/h           # Gestione connessione WiFi
│   ├── WebServerManager.cpp/h      # Server HTTP/REST API
│   ├── WebSocketManager.cpp/h      # Server WebSocket
│   ├── CommandHandler.cpp/h        # Parser comandi CSV
│   ├── Discovery.cpp/h             # mDNS service
│   ├── Debug.h                     # Macro di debug
│   ├── effects/                    # Effetti visivi
│   │   ├── Effect.h                # Classe base effetti
│   │   ├── PongEffect.cpp/h
│   │   ├── MarioClockEffect.cpp/h
│   │   ├── PlasmaEffect.cpp/h
│   │   ├── ScrollTextEffect.cpp/h
│   │   └── ImageEffect.cpp/h
│   └── images/                     # Header immagini
│       ├── andre.h
│       ├── mario.h
│       ├── pokemon.h
│       ├── luigi.h
│       └── fox.h
├── platformio.ini                  # Configurazione PlatformIO
└── README.md
```

## Protocollo Comandi

Il firmware accetta comandi CSV via WebSocket sulla porta 80.

### Formato Generale

```
COMANDO[,parametro1,parametro2,...]
```

### Comandi Disponibili

#### Controllo Effetti

```
EFFECT,nome_effetto
```

Effetti disponibili:
- `pong` - Gioco Pong animato
- `mario_clock` - Orologio con animazione Mario
- `plasma` - Effetto plasma colorato
- `text` - Testo scrolling
- `image` - Visualizzazione immagine statica

#### Controllo Display

```
BRIGHTNESS,valore        # Luminosità 0-255
TEXT,messaggio          # Imposta testo per scroll effect
COLOR,r,g,b            # Colore RGB 0-255
CLEAR                  # Cancella display
```

#### Gestione Sistema

```
STATUS                 # Ottieni stato sistema (JSON)
RESTART                # Riavvia ESP32
TIME,HH,MM,SS         # Imposta orario RTC
DATE,YYYY,MM,DD       # Imposta data RTC
```

#### Configurazione WiFi

```
WIFI_SCAN              # Scansiona reti WiFi
WIFI_CONNECT,ssid,pass # Connetti a rete
WIFI_AP                # Modalità Access Point
WIFI_STATUS            # Stato connessione
```

### Esempio Sessione

```bash
# Connetti via WebSocket a ws://led-matrix.local

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

## REST API Endpoints

Server HTTP su porta 80 (`http://led-matrix.local`).

### GET /

Pagina web di controllo (HTML)

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

Upload file firmware `.bin` come multipart/form-data.

```bash
curl -X POST -F "firmware=@firmware.bin" http://led-matrix.local/update
```

## Configurazioni Avanzate

### Modifica Dimensioni Matrice

In [src/main.cpp](src/main.cpp):

```cpp
#define PANEL_WIDTH 64      // Larghezza pannello
#define PANEL_HEIGHT 64     // Altezza pannello
#define PANELS_NUMBER 1     # Numero pannelli concatenati
#define PIN_E 32            // Pin E per matrici >32 righe
```

### Ottimizzazione Performance

In [platformio.ini](platformio.ini):

```ini
build_flags =
    -DCORE_DEBUG_LEVEL=0    # Disabilita debug log
    -Os                     # Ottimizza per dimensione
```

Per più debug cambiare `CORE_DEBUG_LEVEL` a 3-5.

### Persistent Settings

Le impostazioni sono salvate in flash EEPROM:
- WiFi credentials
- Hostname
- Brightness predefinita
- Effetto all'avvio
- Testo personalizzato

## Aggiungere Nuovi Effetti

### 1. Crea classe effetto

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

### 2. Implementa update

```cpp
// src/effects/MyEffect.cpp
#include "MyEffect.h"

MyEffect::MyEffect(DisplayManager* dm) : Effect(dm) {}

void MyEffect::init() {
    // Inizializzazione
}

void MyEffect::update() {
    // Logica rendering frame
    display->clearScreen();
    // ... disegna su display
}
```

### 3. Registra in EffectManager

In [src/main.cpp](src/main.cpp):

```cpp
#include "effects/MyEffect.h"

// Nel setup()
effectManager->registerEffect(new MyEffect(displayManager));
```

## mDNS Configuration

Il dispositivo è raggiungibile come `led-matrix.local`:

```cpp
// src/Discovery.cpp
Discovery::begin("led-matrix");
```

Servizi pubblicati:
- `_http._tcp`: Web server (porta 80)
- `_ws._tcp`: WebSocket server (porta 80)
- `_arduino._tcp`: Identificazione Arduino/ESP32

## Debug e Monitoring

### Serial Monitor

```bash
pio device monitor -b 115200
```

Output include:
- Connessione WiFi
- Comandi ricevuti
- Errori/warning
- Statistiche performance

### Debug Macro

Usa le macro in [src/Debug.h](src/Debug.h):

```cpp
DEBUG_PRINT("Messaggio");
DEBUG_PRINTLN(variabile);
DEBUG_PRINTF("Valore: %d\n", val);
```

## Troubleshooting

### Display non si accende

1. Verifica alimentazione 5V adeguata
2. Controlla collegamenti HUB75
3. Verifica pin E se matrice è 64 righe
4. Testa con esempio base della libreria

### WiFi non si connette

1. Controlla SSID e password in Settings.cpp
2. Verifica che il router supporti 2.4GHz (ESP32 non supporta 5GHz)
3. Controlla serial monitor per errori
4. Prova modalità AP: connetti a `LED-Matrix-AP`

### Crash/Riavvii random

1. Verifica alimentazione stabile (condensatori consigliati)
2. Riduci luminosità se alimentatore insufficiente
3. Aumenta `CORE_DEBUG_LEVEL` per stack trace
4. Controlla free heap: `ESP.getFreeHeap()`

### OTA non funziona

1. Verifica che ci sia spazio sufficiente (partition scheme)
2. Assicurati che il firmware .bin sia valido
3. Controlla connessione stabile durante upload
4. Verifica dimensione firmware < spazio disponibile

### Effetti lenti/jerky

1. Riduci complessità effetto
2. Aumenta frequenza CPU (160MHz -> 240MHz)
3. Ottimizza codice in update()
4. Usa `-O2` o `-O3` invece di `-Os`

## Performance Tips

- Usa DMA per I/O matrice (già configurato)
- Evita `delay()`, usa timer o task scheduler
- Minimizza allocazioni dinamiche in update()
- Pre-calcola valori costanti
- Usa `IRAM_ATTR` per funzioni critiche

## OTA Updates

### Da Browser

1. Vai a `http://led-matrix.local`
2. Click su sezione OTA/Update
3. Seleziona file `.bin`
4. Attendi completamento

### Da App Flutter

1. Apri ClockApp
2. Vai in Settings
3. Seleziona "Update Firmware"
4. Scegli file firmware
5. Conferma upload

### Da Command Line

```bash
# Build firmware
pio run

# Upload via OTA
curl -F "firmware=@.pio/build/esp32doit-devkit-v1/firmware.bin" \
     http://led-matrix.local/update
```

## Sicurezza

### Raccomandazioni

- Cambia password WiFi di default
- Non esporre il dispositivo su internet pubblico
- Usa rete WiFi protetta (WPA2/WPA3)
- Considera autenticazione per endpoint OTA
- Aggiorna regolarmente le librerie

## Versioning

Per gestire versioni firmware:

```cpp
// In main.cpp
const char* FIRMWARE_VERSION = "1.0.0";
```

Includi versione nella risposta `/status`.

## Contribuire

Per aggiungere funzionalità:
1. Crea branch da main
2. Implementa e testa
3. Documenta codice
4. Submit pull request

## Licenza

Parte del progetto Matrix 64x64 LED Display System.

## Riferimenti

- [ESP32 HUB75 DMA Library](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA)
- [FastLED Documentation](http://fastled.io/)
- [PlatformIO Docs](https://docs.platformio.org/)
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)
