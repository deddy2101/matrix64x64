#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <Arduino.h>
#include <vector>
#include "TimeManager.h"
#include "EffectManager.h"
#include "DisplayManager.h"
#include "Settings.h"
#include "WiFiManager.h"

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
 * 
 * Risposte (ESP32 → App):
 *   OK,comando                     - Comando eseguito
 *   ERR,messaggio                  - Errore
 *   STATUS,time,date,mode,ds3231,temp,effect,idx,fps,auto,count,bright,night,wifi,ip,ssid,rssi,uptime,heap
 *   EFFECTS,name1,name2,name3,...  - Lista nomi effetti
 *   SETTINGS,ssid,apMode,brightDay,brightNight,nightStart,nightEnd,duration,auto,effect,deviceName
 *   EFFECT,index,name              - Notifica cambio effetto
 *   TIME,HH:MM:SS                  - Notifica cambio ora
 */
class CommandHandler {
public:
    CommandHandler();
    
    void init(TimeManager* time, EffectManager* effects, DisplayManager* display, Settings* settings, WiFiManager* wifi);
    void setWebSocketManager(WebSocketManager* ws);
    
    // Processa comando e ritorna risposta
    String processCommand(const String& command);
    
    // Processa comando seriale legacy (singolo carattere)
    String processLegacyCommand(const String& command);
    
    // Genera risposte
    String getStatusResponse();
    String getEffectsResponse();
    String getSettingsResponse();
    
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
};

#endif // COMMAND_HANDLER_H