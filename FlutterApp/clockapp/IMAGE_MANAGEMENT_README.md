# Gestione Immagini LED Matrix

Sistema completo per caricare, gestire ed eliminare immagini dinamicamente sulla matrice LED 64x64 via app Flutter.

## 🎯 Caratteristiche

- **Upload automatico**: Carica qualsiasi immagine (PNG, JPG, WebP, ecc.)
- **Conversione automatica**: Resize a 64x64 e conversione RGB565
- **Gestione storage**: Visualizza spazio disponibile su ESP32
- **Lista immagini**: Vedi tutte le immagini salvate
- **Eliminazione**: Rimuovi immagini non più necessarie

## 📱 Come Usare (App Flutter)

### 1. Connettiti al Dispositivo
- Apri l'app e connettiti al tuo ESP32 via WiFi o USB

### 2. Accedi alla Gestione Immagini
- Nella home screen, premi l'icona **immagine** (📷) nell'header
- Si aprirà la schermata "Gestione Immagini"

### 3. Carica un'Immagine
1. Premi il pulsante **"Carica Immagine"** (floating action button)
2. Seleziona un'immagine dal tuo dispositivo
3. Inserisci un nome per l'immagine (es: "mario", "pikachu")
4. Attendi la conversione e l'upload

**Cosa succede dietro le quinte:**
- L'immagine viene ridimensionata a 64x64 pixel (qualità Lanczos)
- Convertita in formato RGB565 (2 bytes per pixel)
- Codificata in base64
- Inviata all'ESP32 in chunk da 3KB
- Salvata su LittleFS come `/images/NAME.img`

### 4. Visualizza Storage
- Nella parte superiore della schermata vedi:
  - Spazio totale (es: 1.4 MB)
  - Spazio usato
  - Spazio libero
  - Progress bar visuale

### 5. Elimina Immagini
- Nella lista, premi l'icona **cestino** (🗑️) accanto all'immagine
- Conferma l'eliminazione

## 🔧 Specifiche Tecniche

### Formato Immagini
- **Dimensione**: 64x64 pixels (fisso)
- **Formato colore**: RGB565 (16-bit)
  - Rosso: 5 bit (32 livelli)
  - Verde: 6 bit (64 livelli)
  - Blu: 5 bit (32 livelli)
- **Dimensione file**: 8192 bytes (64 × 64 × 2)
- **Codifica trasferimento**: Base64 (~11 KB per immagine)

### Limiti Storage ESP32
- **Storage totale**: ~1.4 MB (LittleFS)
- **Capacità**: ~175 immagini (8KB ciascuna)
- **Partizione**: Separata da App e OTA

### Protocollo Comandi

#### Upload Immagine
```
Comando:  image,upload,NAME,BASE64_DATA
Risposta: OK,Image uploaded: NAME
Errore:   ERR,messaggio
```

#### Lista Immagini
```
Comando:  image,list
Risposta: IMAGES,COUNT,name1,size1,name2,size2,...
```

#### Elimina Immagine
```
Comando:  image,delete,NAME
Risposta: OK,Image deleted: NAME
```

#### Info Storage
```
Comando:  image,info
Risposta: IMAGE_INFO,total,used,free
```

## 💡 Suggerimenti

### Qualità Immagini
- Usa immagini con contrasto elevato (meglio su LED)
- Colori vividi funzionano meglio
- Evita dettagli troppo fini (persi a 64x64)
- Pixel art funziona perfettamente

### Performance Upload
- **Tempo medio**: ~2-3 secondi per immagine
- **Chunk size**: 3KB (ottimizzato per evitare frammentazione WebSocket)
- **Retry automatico**: Fino a 3 tentativi per chunk

### Gestione Storage
- Controlla regolarmente lo spazio libero
- Elimina immagini non più utilizzate
- Ogni immagine occupa esattamente 8192 bytes

## 🛠️ Sviluppo

### Dipendenze Flutter
```yaml
dependencies:
  image: ^4.0.0      # Conversione immagini
  file_picker: ^8.0.0 # Selezione file
```

### File Creati
- `lib/services/image_service.dart` - Servizio gestione immagini
- `lib/screens/image_management_screen.dart` - UI gestione
- `ESPCode/src/ImageManager.h/cpp` - Backend ESP32

### Conversione RGB565
```dart
// RGB888 → RGB565
final r = pixel.r.toInt();  // 0-255
final g = pixel.g.toInt();  // 0-255
final b = pixel.b.toInt();  // 0-255

final rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
```

## 🔄 Workflow Tipico

1. **Prepara immagine** (opzionale)
   - Ridimensiona manualmente a 64x64 per preview
   - Ottimizza colori per LED

2. **Upload dall'app**
   - Seleziona file
   - Assegna nome
   - Attendi conferma

3. **Verifica**
   - Controlla presenza nella lista
   - Verifica dimensione (deve essere 8192 bytes)

4. **Usa nell'effetto** (da implementare)
   - Modifica `ImageEffect` per caricare da LittleFS
   - Seleziona immagine dal nome

## 📝 Note

### Immagini Hardcoded vs Dinamiche
**Hardcoded** (attuali: mario.h, luigi.h, ecc.):
- ✅ Più veloci (in PROGMEM)
- ✅ Non usano LittleFS
- ❌ Richiedono ricompilazione
- ❌ Occupano Flash (~62KB/immagine)

**Dinamiche** (nuovo sistema):
- ✅ Caricabili da app
- ✅ Non richiedono ricompilazione
- ✅ Storage efficiente (8KB/immagine)
- ❌ Lettura da filesystem (minimamente più lenta)

### Prossimi Passi
- [ ] Modificare `ImageEffect` per supportare caricamento da nome
- [ ] Aggiungere selezione immagine dinamica nell'app
- [ ] Opzionale: Convertire immagini hardcoded e rimuovere .h files
- [ ] Aggiungere preview immagini nella lista (opzionale)

## 🐛 Troubleshooting

### Upload Fallisce
- Verifica connessione WiFi stabile
- Controlla spazio disponibile
- Prova con immagine più piccola inizialmente

### Immagine Non Appare
- Verifica nome (no spazi, caratteri speciali)
- Controlla che size = 8192 bytes
- Riavvia ESP32 se necessario

### Storage Pieno
- Elimina immagini inutilizzate
- Ogni immagine = 8192 bytes esatti
- Massimo ~175 immagini

## 📚 Riferimenti

- ESP32 LittleFS: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/spiffs.html
- RGB565 Format: https://en.wikipedia.org/wiki/High_color
- Image Package: https://pub.dev/packages/image
