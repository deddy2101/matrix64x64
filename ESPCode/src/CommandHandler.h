#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <Arduino.h>
#include <vector>
#include <Update.h>
#include "TimeManager.h"
#include "EffectManager.h"
#include "DisplayManager.h"
#include "Settings.h"
#include "WiFiManager.h"
#include "ImageManager.h"
#include "Debug.h"


// Forward declaration
class WebSocketManager;

/**
 * CommandHandler - Gestione comandi con protocollo CSV
 * 
 * PROTOCOLLO:
 * 
 * Richieste (App → ESP32):
 *   getStatus                      - Richiedi stato completo
 *   getEffects                     - Lista effetti disponibili
 *   getSettings                    - Impostazioni correnti
 *   getVersion                     - Versione firmware corrente
 *   setTime,HH,MM,SS               - Imposta ora
 *   setDateTime,YYYY,MM,DD,HH,MM,SS - Imposta data/ora
 *   setMode,rtc|fake               - Modalità tempo
 *   effect,next                    - Prossimo effetto
 *   effect,pause                   - Pausa auto-switch
 *   effect,resume                  - Riprendi auto-switch
 *   effect,select,INDEX            - Seleziona effetto
 *   effect,name,NAME               - Seleziona per nome
 *   brightness,day,VALUE           - Luminosità giorno (0-255)
 *   brightness,night,VALUE         - Luminosità notte (0-255)
 *   brightness,VALUE               - Luminosità immediata
 *   nighttime,START,END            - Orari notte (0-23)
 *   duration,MS                    - Durata effetti in ms
 *   autoswitch,0|1                 - Auto-switch on/off
 *   wifi,SSID,PASSWORD,AP_MODE     - Configura WiFi (AP_MODE: 0=STA, 1=AP)
 *   devicename,NAME                - Nome dispositivo
 *   save                           - Salva impostazioni
 *   restart                        - Riavvia ESP32
 *   ota,start,SIZE                 - Inizia OTA update (SIZE in bytes)
 *   ota,data,BASE64_CHUNK          - Invia chunk dati firmware (base64)
 *   ota,end,MD5                    - Finalizza OTA con verifica MD5
 *   ota,abort                      - Annulla OTA in corso
 *   image,upload,NAME,BASE64       - Upload immagine (nome + RGB565 base64)
 *   image,list                     - Lista immagini salvate
 *   image,delete,NAME              - Elimina immagine
 *   image,info                     - Info storage immagini
 *   image,show,NAME                - Mostra immagine specifica
 *   image,next                     - Prossima immagine nello slideshow
 *   image,prev                     - Immagine precedente nello slideshow
 *   image,slideshow,0|1            - Abilita/disabilita slideshow automatico
 *
 * Risposte (ESP32 → App):
 *   OK,comando                     - Comando eseguito
 *   ERR,messaggio                  - Errore
 *   OTA_READY                      - Pronto per ricevere firmware
 *   OTA_PROGRESS,bytes,percent     - Progresso upload
 *   OTA_SUCCESS                    - Update completato (riavvio imminente)
 *   OTA_ERROR,messaggio            - Errore durante OTA
 *   STATUS,time,date,mode,ds3231,temp,effect,idx,fps,auto,count,bright,night,wifi,ip,ssid,rssi,uptime,heap
 *   EFFECTS,name1,name2,name3,...  - Lista nomi effetti
 *   SETTINGS,ssid,apMode,brightDay,brightNight,nightStart,nightEnd,duration,auto,effect,deviceName
 *   VERSION,version,buildNumber,buildDate,buildTime - Versione firmware
 *   EFFECT,index,name              - Notifica cambio effetto
 *   TIME,HH:MM:SS                  - Notifica cambio ora
 */
class CommandHandler {
public:
    CommandHandler();

    void init(TimeManager* time, EffectManager* effects, DisplayManager* display, Settings* settings, WiFiManager* wifi, ImageManager* imgMgr = nullptr);
    void setWebSocketManager(WebSocketManager* ws);
    
    // Processa comando e ritorna risposta
    String processCommand(const String& command);
    
    // Processa comando seriale legacy (singolo carattere)
    String processLegacyCommand(const String& command);
    
    // Genera risposte
    String getStatusResponse();
    String getEffectsResponse();
    String getSettingsResponse();
    String getVersionResponse();
    
    // Notifiche
    String getEffectChangeNotification();
    String getTimeChangeNotification();
    
    // Utility
    void updateBrightness();
    void processSerial(const String& cmd);

private:
    TimeManager* _timeManager;
    EffectManager* _effectManager;
    DisplayManager* _displayManager;
    Settings* _settings;
    WiFiManager* _wifiManager;
    WebSocketManager* _wsManager;
    ImageManager* _imageManager;
    
    // Parser helper
    std::vector<String> splitCommand(const String& cmd, char delimiter = ',');
    
    // Handler specifici
    String handleSetTime(const std::vector<String>& parts);
    String handleSetDateTime(const std::vector<String>& parts);
    String handleSetMode(const std::vector<String>& parts);
    String handleEffect(const std::vector<String>& parts);
    String handleBrightness(const std::vector<String>& parts);
    String handleNightTime(const std::vector<String>& parts);
    String handleDuration(const std::vector<String>& parts);
    String handleAutoSwitch(const std::vector<String>& parts);
    String handleWiFi(const std::vector<String>& parts);
    String handleDeviceName(const std::vector<String>& parts);
    String handleSave();
    String handleRestart();
    String handleOTA(const std::vector<String>& parts);
    String handleImage(const std::vector<String>& parts);

    // OTA state
    bool _otaInProgress;
    size_t _otaSize;
    size_t _otaWritten;
    int _otaExpectedChunk;
    String _otaExpectedMD5;

    // Base64 decode helper
    size_t base64Decode(const String& input, uint8_t* output, size_t maxLen);
};

#endif // COMMAND_HANDLER_H