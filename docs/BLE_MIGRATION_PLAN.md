# Piano: da WiFi/WebSocket a Bluetooth LE

Stato: **approvato, da implementare** (2026-10-09). Le decisioni sotto sono chiuse: non rimetterle in discussione, segui il piano. Se un punto risulta tecnicamente impossibile, fermati e chiedi.

## Decisioni prese

| Tema | Decisione |
|---|---|
| Firmware ESP32 | **Solo BLE**. WiFi, WebServer, WebSocket, mDNS/Discovery e NTP vengono **rimossi** (non solo disattivati). Resta la seriale USB. |
| App Flutter | Parla **sia WebSocket sia BLE** (più seriale su desktop). Il WebSocket resta completo: serve ai dispositivi col firmware vecchio e per la migrazione. |
| Sicurezza | Pairing BLE con **passkey a 6 cifre mostrata sulla matrice LED**, poi bonding (non la richiede più). |
| Migrazione dispositivi esistenti | Esce prima l'app nuova; poi l'ultimo OTA via WebSocket installa il firmware BLE. |
| Ordine lavoro | Firmware e app in parallelo, dopo aver fissato il contratto (sezione 1). |
| Versioni | Firmware **3.0.0** (breaking). App **2.0.0**. |

## Fatti di partenza (verificati sul codice)

- Board `esp32doit-devkit-v1`, ESP32 classico (BLE 4.2, niente PSRAM), piattaforma espressif32 6.12.0 → Arduino core **2.0.11**.
- Partizioni `default.csv`: slot app da 1.280 KB. `firmware.bin` attuale = 1.019 KB. **Non cambiare la tabella partizioni** (non si può via OTA).
- Il protocollo è CSV a righe e indipendente dal trasporto: `CommandHandler::processCommand(String) -> String`.
- `CommandHandler` chiama `_wsManager->broadcast(...)` / `notify*()` in circa 15 punti (pong, snake, effect, time).
- L'upload immagini manda una sola riga `image,upload,NAME,BASE64` di circa 11 KB. L'OTA manda `ota,data,N,BASE64` con ACK (`OTA_ACK,N`) per ogni chunk.
- iOS **non** supporta Bluetooth Classic SPP (solo MFi) → si usa **BLE GATT**.

---

## 1. Contratto condiviso (da fissare per primo)

### GATT
UUID custom (non quelli Nordic standard, così non troviamo altri dispositivi NUS):

```
Service  : 6f1a0001-8b5c-4d2e-9a3f-1c2d3e4f5a6b
RX (app→ESP): 6f1a0002-8b5c-4d2e-9a3f-1c2d3e4f5a6b   WRITE | WRITE_NR (con cifratura+autenticazione)
TX (ESP→app): 6f1a0003-8b5c-4d2e-9a3f-1c2d3e4f5a6b   NOTIFY
```

- Advertising: flags + UUID del servizio a 128 bit; il nome (`settings.getDeviceName()`) va nella scan response.
- MTU richiesto: 247 (payload utile = MTU − 3).

### Framing
- Entrambe le direzioni: righe di testo UTF-8 terminate da `\n`. Una riga può arrivare spezzata in più write/notify: va riassemblata finché non arriva `\n`.
- Lunghezza massima di una riga lato ESP: **16 KB** (copre l'upload immagini). Se una riga la supera, va scartata e si risponde `ERR,message too large`.
- I comandi e le risposte sono **identici** a quelli di oggi (vedi `CommandHandler.h`).

### Compatibilità delle risposte (l'app non deve cambiare parser)
- `STATUS`: i campi `wifi,ip,ssid,rssi` restano in posizione con i valori `BLE,,,0`. `ntpSynced` = `0`.
- `SETTINGS`: `ssid` = vuoto, `apMode` = `0`, `ntpEnabled` = `0`.
- I comandi `wifi,…`, `wifiscan`, `ntp,…` rispondono `ERR,not supported on BLE firmware`.
- Nuovo comando `ble,forget`: cancella tutti i bond e risponde `OK,bonds cleared`. Lo stesso comando funziona anche da seriale.
- Alla fine dell'autenticazione l'ESP invia `WELCOME,LED Matrix Controller` + `STATUS,...` a quella connessione, come fa oggi il WebSocket al connect.

---

## 2. Firmware (ESPCode)

### F0. Rimozione WiFi e misure (checkpoint)
1. `platformio.ini`: togliere `ESPAsyncWebServer` e `AsyncTCP`; aggiungere `h2zero/NimBLE-Arduino@^1.4.3`. La 1.4.x è la linea compatibile con core 2.0.x: **non** usare la 2.x.
   Build flags: `-DCONFIG_BT_NIMBLE_MAX_CONNECTIONS=3`, `-DCONFIG_BT_NIMBLE_MAX_BONDS=8`.
2. Eliminare `WiFiManager.*`, `WebServerManager.*`, `WebSocketManager.*`, `Discovery.*`, `discovery.py`.
3. `TimeManager`: togliere `#include <WiFi.h>` e tutto l'NTP (`syncFromNTP`, `checkNtpSync`, `forceNTPSync`, i membri `ntp*`, i server NTP). `isNTPSynced()` può restare e ritornare `false`, se serve per il formato STATUS. Il timezone resta.
   **Da verificare**: come `setDateTime` interpreta l'ora (locale o UTC) e se il DS3231 tiene l'ora locale. L'app manderà l'**ora locale** del telefono.
4. `Settings`: **non** togliere i campi WiFi/NTP dalla struct e da NVS (compatibilità con le preferenze già salvate). Si smette solo di usarli.
5. `CommandHandler`: togliere `WiFiManager*` da `init()`, i metodi `handleWiFi`/`handleWiFiScan`/`handleNTP` diventano la risposta ERR del contratto. Sistemare `getStatusResponse`/`getSettingsResponse` come da contratto. Aggiornare il commento del protocollo nell'header.
6. `main.cpp`: togliere tutte le inizializzazioni e gli update WiFi/WS/Discovery e aggiornare il banner.
7. **Misura e riporta all'utente**: dimensione di `firmware.bin` e `ESP.getFreeHeap()` / `heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)` dopo il setup, prima con il BLE vuoto e poi con il BLE completo. Obiettivo: firmware ≤ 1.150 KB (margine per gli OTA futuri) e almeno un blocco contiguo libero da 20 KB.

### F1. Interfaccia di notifica
- Nuovo `src/Notifier.h`: interfaccia astratta con `virtual void broadcast(const String&) = 0;`.
- `CommandHandler`: sostituire `WebSocketManager* _wsManager` con `Notifier* _notifier` (`setNotifier()`). I vari `notifyEffectChange` / `notifyTimeChange` / `notifyStatusChange` diventano `_notifier->broadcast(getEffectChangeNotification())` ecc.

### F2. BleManager (`src/BleManager.{h,cpp}`, implementa `Notifier`)
- `begin(Settings*, CommandHandler*)`:
  - `NimBLEDevice::init(deviceName)`, `setMTU(247)`, `setPower(ESP_PWR_LVL_P9)`;
  - sicurezza: `setSecurityAuth(true, true, true)` (bond, MITM, secure connections) e `setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY)`;
  - creare servizio, RX e TX; RX con `WRITE | WRITE_NR | WRITE_ENC | WRITE_AUTHEN`;
  - avviare l'advertising come da contratto.
  - Verificare che venga liberata la memoria del Bluetooth Classic (`esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT)`). Se NimBLE non lo fa già da solo, chiamarlo prima dell'init.
- **Multi-connessione**: in `onConnect`, se le connessioni sono meno di 3, richiamare `NimBLEDevice::startAdvertising()`. Lo stesso in `onDisconnect` (o affidarsi a `advertiseOnDisconnect`).
- **Pairing con PIN**:
  - `onPassKeyRequest()` genera un numero casuale `esp_random() % 1000000`, lo salva e alza il flag `_pairingActive`;
  - `onAuthenticationComplete(desc)`: se `desc->sec_state.authenticated`, la connessione entra nel set degli autenticati e riceve WELCOME + STATUS; altrimenti la connessione viene chiusa. In entrambi i casi il flag si abbassa;
  - il flag si abbassa da solo anche dopo 60 s.
- **Thread safety (obbligatoria)**: le callback NimBLE girano nel task host, **non** nel `loop()`.
  - `onWrite`: appende i byte al buffer di riga di quella connessione (mappa `conn_handle → String`); per ogni riga completa fa push di `{conn_handle, String*}` in una coda FreeRTOS (`xQueueCreate(8, ...)`).
  - Mai chiamare `processCommand` dentro la callback.
  - Le scritture da connessioni non autenticate vengono ignorate.
- `update()`, chiamato dal `loop()`:
  1. consuma la coda RX → `processCommand` → mette la risposta (più `\n`) nella coda TX di quella connessione;
  2. svuota le code TX spezzando i dati in notify da (MTU−3) byte.
- Invio a una singola connessione: usare la notify verso un `conn_handle` specifico. Se l'API 1.4 della characteristic non lo espone, usare `ble_gattc_notify_custom(conn, txChar->getHandle(), ble_hs_mbuf_from_flat(...))`.
  - Se ritorna `BLE_HS_ENOMEM`, ritentare al giro dopo (non perdere dati e non bloccare il loop).
  - Mandare al massimo circa 8 notify per connessione per ogni giro di `update()`, così le animazioni restano fluide.
- `broadcast(msg)`: accoda `msg` a tutte le connessioni autenticate. Va chiamata solo dal `loop()`, quindi tutti i chiamanti attuali sono già ok.
- `ble,forget`: chiama `NimBLEDevice::deleteAllBonds()`. Va gestito in `CommandHandler`, che chiede a `BleManager`.

### F3. PIN sulla matrice
- `DisplayManager::showPairingCode(uint32_t code)`, sullo stile di `showOTAProgress`: scritta "PIN" e le 6 cifre grandi e centrate.
- `loop()`: se `bleManager->isPairingActive()`, disegna il PIN **al posto di** `effectManager->update()`; quando il flag torna giù riprendono gli effetti.

### F4. OTA
- Il protocollo resta uguale. Verificare solo che:
  - il buffer RX da 16 KB regga un chunk `ota,data` (l'app via BLE manderà chunk da 2 KB grezzi);
  - il riavvio differito (`checkPendingRestart`) lasci partire `OTA_SUCCESS` prima del reboot (la coda TX va svuotata: se serve, allungare l'attesa o controllare che la coda sia vuota).
- `EffectManager::pause()` resta attivo durante l'OTA come oggi.

### F5. Pulizia
- `Version.h` → `3.0.0`, numero di build +1.
- Aggiornare `ESPCode/README.md` e la sezione protocollo in `CommandHandler.h`.

---

## 3. App (FlutterApp/clockapp)

### A0. Dipendenza BLE
- Proposta: `flutter_blue_plus`, ultima versione stabile. **Prima di aggiungerla, controlla la licenza attuale del pacchetto** (in passato è cambiata per l'uso commerciale). Se è un problema, usa `universal_ble` e dillo all'utente.
- Permessi:
  - Android `AndroidManifest.xml`: `BLUETOOTH_SCAN` (`android:usesPermissionFlags="neverForLocation"`), `BLUETOOTH_CONNECT`; `BLUETOOTH`, `BLUETOOTH_ADMIN` e `ACCESS_FINE_LOCATION` con `android:maxSdkVersion="30"`.
  - iOS `Info.plist`: `NSBluetoothAlwaysUsageDescription` (in italiano e inglese, come le altre stringhe).
  - macOS: `NSBluetoothAlwaysUsageDescription` e l'entitlement `com.apple.security.device.bluetooth` (Debug e Release).
- `permission_service.dart`: aggiungere `requestBluetoothPermissions()` e `hasBluetoothPermissions()`.

### A1. BleLink (`lib/services/ble_link.dart`)
Classe piccola che nasconde i dettagli BLE a `DeviceService`:
- `Stream<BleScanResult> scan()`: filtra sull'UUID del servizio; restituisce nome, id e RSSI.
- `Future<bool> connect(String deviceId)`:
  - connette e chiede MTU 247 (su iOS è automatico);
  - su Android chiede la connection priority alta;
  - scopre i servizi e si iscrive alle notify su TX.
- `Stream<String> lines`: riassembla le notify in righe separate da `\n`.
- `Future<void> sendLine(String line)`:
  - **serializza gli invii con una coda interna**, così due `send()` concorrenti non mescolano i byte;
  - spezza in pezzi da (MTU−3) e usa write **without response**. Il controllo di flusso a livello di app lo fanno già gli ACK di OTA e immagini.
- `Stream<bool> connectionState` e `disconnect()`.
- Il pairing lo gestisce il sistema operativo: la prima write su RX scatena il dialog del PIN. Il primo invio (`setDateTime`) deve avere un timeout lungo (60 s) per dare tempo all'utente di digitare il PIN.

### A2. DeviceService
- `enum ConnectionType { none, serial, websocket, ble }`.
- `connectBle(String deviceId, String name)`. Allo stato connected fa, in quest'ordine:
  1. **`setDateTime` con `DateTime.now()`** (sincronizzazione automatica dell'ora);
  2. `getStatus`, `getEffects`, `getSettings` (come fa oggi dopo il WS).
- Le righe ricevute passano per lo stesso `_dataController` / `_parseResponse` di WS e seriale.
- `send()`: nuovo ramo `ConnectionType.ble` → `bleLink.sendLine(command)`.
- Riconnessione: copiare la logica di `_scheduleReconnect`, compresa la modalità OTA, usando l'id BLE. Il watchdog ping/`_rxStaleTimeout` vale anche per il BLE.
- `disconnect()` chiude anche il BLE.

### A3. Discovery
- `device_discovery_screen.dart`: la scansione BLE è la sezione principale ("Dispositivi Bluetooth"). La scansione WiFi/UDP resta sotto, come sezione secondaria ("Dispositivi WiFi (firmware precedente)"). La seriale desktop resta com'è.
- Il widget del dispositivo mostra l'RSSI con `signal_indicator.dart`.
- Se il Bluetooth è spento, mostrare un messaggio chiaro (e su Android l'invito ad accenderlo).

### A4. OTA
- `ota_service.dart`: chunk size in base al trasporto: WS 3072 (come oggi), BLE 2048. Timeout ACK per BLE: 15 s, come oggi.
- **Migrazione WS → BLE**: se l'OTA va a buon fine su `ConnectionType.websocket` e il firmware installato è ≥ 3.0.0 (la versione è nel manifest di `firmware_repository.dart`):
  - non provare a riconnettersi via WS;
  - mostrare "Firmware Bluetooth installato. Ricollegati al display via Bluetooth" e tornare alla discovery.
- Anche i messaggi di riconnessione che parlano di "modalità AP" vanno mostrati solo con il WS.

### A5. UI
- Con `ConnectionType.ble` nascondere tutto ciò che è WiFi: `wifi_config_dialog`, scansione WiFi, IP/SSID in `connection_card.dart` e `status_row.dart`. Mostrare invece "Bluetooth" e il nome del dispositivo.
- Nelle impostazioni (solo BLE): un'azione "Dimentica telefoni associati" che manda `ble,forget`, con conferma.
- Giochi (Pong, Snake): nessuna modifica logica. Verificare solo che `pong,setpos` mandato a raffica non intasi la coda: se ci sono già invii in attesa, tenere solo l'ultimo `setpos` per giocatore (coalescing in `BleLink` o nel controller).

### A6. Documenti
- `privacy_policy.html` e i file `assets/playstore/store_listing_*.txt`: citare il Bluetooth.
- `pubspec.yaml` → `2.0.0+21`.

---

## 4. Test (prima di dichiarare finito)

Firmware, con monitor seriale aperto:
- [ ] build ok; dimensioni e heap riportati all'utente (F0.7)
- [ ] l'advertising si vede da nRF Connect con il nome giusto
- [ ] il primo collegamento mostra il PIN sulla matrice; un PIN sbagliato porta alla disconnessione; i collegamenti successivi non chiedono il PIN
- [ ] 2 telefoni connessi insieme: entrambi ricevono `PONG_STATE`, ciascuno muove la sua racchetta
- [ ] dopo un upload immagine da 64×64 il loop non ha rallentamenti visibili
- [ ] OTA via BLE di un firmware completo; annotare tempo e velocità
- [ ] `ble,forget` da seriale e da app

App (Android **e** iPhone veri, più macOS se possibile):
- [ ] scan → connect → l'ora sul display si allinea al telefono
- [ ] riconnessione automatica dopo `restart` e dopo un'uscita dal raggio BLE
- [ ] OTA via BLE completo, con verifica della nuova versione
- [ ] **migrazione**: dispositivo con firmware 2.6.0 via WS → OTA verso 3.0.0 → messaggio "ricollegati via Bluetooth" → connessione BLE ok
- [ ] un dispositivo vecchio via WS funziona ancora (effetti, giochi, impostazioni)
- [ ] permessi negati → messaggio chiaro, nessun crash

## Fuori scope (per ora)
- OTA binario su una characteristic dedicata (senza base64). Da valutare solo se l'OTA BLE risulta troppo lento dopo i test.
- Cambio della tabella partizioni.

---

## Note di implementazione (scostamenti dal piano)

Stato al 2026-10-09: codice scritto e compilato (firmware `pio run` ok, app `flutter analyze` senza errori nel codice di `lib/`), **mai provato su hardware**.

- **Libreria BLE dell'app: `universal_ble` (BSD-3), non `flutter_blue_plus`.** La licenza di `flutter_blue_plus` richiede un accordo commerciale per usi for-profit. `universal_ble` copre anche Windows, Linux e web.
- **Permessi in app:** li chiede `UniversalBle.requestPermissions()`, quindi `permission_service.dart` non è stato modificato. I permessi nativi sono nei manifest (Android, iOS, macOS).
- **Coda RX del firmware:** coda FreeRTOS di puntatori a `String` (16 elementi), come da piano; se piena, il comando viene scartato.
- **PIN:** generato in `onConnect` solo per telefoni non ancora associati; per quelli associati la cifratura riparte con le chiavi salvate. `onPassKeyRequest` restituisce lo stesso codice.
- **`WELCOME` come segnale di "pronto":** il firmware lo manda solo a collegamento cifrato, autenticato e con notify attive; l'app lo aspetta (timeout 90 s) prima di dichiararsi connessa.
- **Scrittura:** tutti gli invii dell'app sono write *senza risposta*. Se i test mostrano perdite negli invii lunghi (OTA, immagini), mettere `BleLink.bulkWithResponse = true`.
- **Migrazione:** `OtaService` riceve `targetVersion`; se si aggiorna via WebSocket verso una 3.x non attende la riconnessione e mostra un avviso persistente nella schermata di ricerca (`DeviceService.notice`). Con un `.bin` scelto da file la versione non è nota e vale solo il messaggio generico di fine attesa.
- **Non fatto:** riscrittura dei README (solo un avviso in testa), pubblicazione di un firmware 3.0.0 nel `firmware-server`, test su hardware.

---

## Accesso a due PIN (sostituisce il pairing "PIN a schermo intero" di F3)

Deciso il 2026-10-09. **Un telefono nuovo ha bisogno di due PIN; uno già associato di nessuno.**

1. Il telefono si collega e si iscrive alla caratteristica **UNLOCK** (`6f1a0004-8b5c-4d2e-9a3f-1c2d3e4f5a6b`, WRITE + NOTIFY, **non cifrata**). Il firmware risponde `KNOWN` (telefono già associato: cifratura con le chiavi salvate, nessun PIN) o `LOCKED`.
2. `LOCKED`: l'app scrive sulla UNLOCK il **PIN statico** (6 cifre, ASCII). Risposte: `OK`, `ERR,wrong,N` (N tentativi rimasti), `ERR,locked,S` (blocco di S secondi, connessione chiusa).
3. Solo dopo `OK` il firmware apre per 60 s la finestra del **PIN dinamico**: avvia la cifratura, la libreria chiede il PIN (`onPassKeyRequest`), il display lo mostra **in sovrimpressione** (alto a destra, cifre bianche su fondo nero, luminosità forzata a 255 finché è visibile) e l'utente lo digita nella finestra di sistema del telefono.
4. A pairing riuscito il firmware invia `WELCOME` + `STATUS` (come prima) e il telefono è associato.

- Senza `OK` il pairing viene rifiutato e **non compare nessun PIN**: chi passa e si collega non ottiene nulla sul display.
- 3 PIN statici errati di fila → blocco di 5 minuti.
- PIN statico: di fabbrica `BLE_DEFAULT_STATIC_PIN` in `BleManager.h` (**da cambiare prima di produrre i display**), poi modificabile con `ble,setpin,NNNNNN` (salvato in NVS, namespace `ble`). L'app lo ricorda per display (`shared_preferences`) e lo riusa nelle riconnessioni automatiche.
- Telefono che ha "dimenticato" il display (che invece lo ricorda): la cifratura fallisce, il firmware cancella quel bond e invia `LOCKED`: si ripassa dai due PIN.
- Da verificare su hardware: riconoscimento di un telefono associato (`isBonded` con indirizzi casuali), gestione del bond stantio, comportamento di iOS/Android alla richiesta di cifratura.
- L'overlay è disegnato da `DisplayManager::endFrame()` per gli effetti con buffer (solo `MarioClockEffect`) e da `drawPairingOverlay()` dopo ogni `effectManager->update()` per gli altri: con questi ultimi può lampeggiare leggermente se l'effetto ridisegna tutto lo schermo a ogni frame.
