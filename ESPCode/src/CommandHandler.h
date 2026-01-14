#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <Arduino.h>
#include <Update.h>
#include "TimeManager.h"
#include "EffectManager.h"
#include "DisplayManager.h"
#include "Settings.h"
#include "WiFiManager.h"
#include "ImageManager.h"
#include "TextScheduleManager.h"
#include "Debug.h"

// Struttura per parsing comandi senza allocazioni dinamiche
#define MAX_CMD_PARTS 10
struct ParsedCommand {
    String parts[MAX_CMD_PARTS];
    int count;

    ParsedCommand() : count(0) {}

    // Operator[] per compatibilità con vector
    String& operator[](int index) {
        return parts[index];
    }

    const String& operator[](int index) const {
        return parts[index];
    }

    int size() const {
        return count;
    }
};


// Forward declaration
class WebSocketManager;
class ScrollTextEffect;
class PongEffect;
class SnakeEffect;

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
 *   scrolltext,TEXT[,COLOR]        - Imposta testo scorrevole (COLOR opzionale RGB565)
 *   pong,join,1|2                  - Giocatore si unisce (1=sinistra, 2=destra)
 *   pong,leave,1|2                 - Giocatore esce
 *   pong,move,1|2,up|down|stop     - Muovi paddle
 *   pong,setpos,1|2,0-100          - Imposta posizione paddle (0=fondo, 100=cima)
 *   pong,start                     - Avvia partita
 *   pong,pause                     - Pausa partita
 *   pong,resume                    - Riprendi partita
 *   pong,reset                     - Reset partita
 *   pong,state                     - Richiedi stato gioco
 *   snake,join                     - Giocatore si unisce
 *   snake,leave                    - Giocatore esce
 *   snake,dir,u|d|l|r              - Cambia direzione (up/down/left/right)
 *   snake,start                    - Avvia partita
 *   snake,pause                    - Pausa partita
 *   snake,resume                   - Riprendi partita
 *   snake,reset                    - Reset partita
 *   snake,state                    - Richiedi stato gioco
 *   ntp,enable                     - Abilita NTP sync
 *   ntp,disable                    - Disabilita NTP sync
 *   ntp,sync                       - Forza sync NTP ora
 *   timezone,TZ_STRING             - Imposta timezone (es: CET-1CEST,M3.5.0,M10.5.0/3)
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
 *   schedtext,list                 - Lista scritte programmate
 *   schedtext,add,TEXT,COLOR,HH,MM[,REPEATDAYS,YEAR,MONTH,DAY,LOOPCOUNT] - Aggiungi scritta programmata
 *   schedtext,update,ID,TEXT,COLOR,HH,MM[,REPEATDAYS,YEAR,MONTH,DAY,LOOPCOUNT] - Aggiorna scritta programmata
 *   schedtext,delete,ID            - Elimina scritta programmata
 *   schedtext,enable,ID            - Abilita scritta programmata
 *   schedtext,disable,ID           - Disabilita scritta programmata
 *
 * Risposte (ESP32 → App):
 *   OK,comando                     - Comando eseguito
 *   ERR,messaggio                  - Errore
 *   OTA_READY                      - Pronto per ricevere firmware
 *   OTA_PROGRESS,bytes,percent     - Progresso upload
 *   OTA_SUCCESS                    - Update completato (riavvio imminente)
 *   OTA_ERROR,messaggio            - Errore durante OTA
 *   STATUS,time,date,mode,ds3231,temp,effect,idx,fps,auto,count,bright,night,wifi,ip,ssid,rssi,uptime,heap,ntpSynced
 *   EFFECTS,name1,name2,name3,...  - Lista nomi effetti
 *   SETTINGS,ssid,apMode,brightDay,brightNight,nightStart,nightEnd,duration,auto,effect,deviceName,scrollText,ntpEnabled,timezone
 *   VERSION,version,buildNumber,buildDate,buildTime - Versione firmware
 *   EFFECT,index,name              - Notifica cambio effetto
 *   TIME,HH:MM:SS                  - Notifica cambio ora
 *   PONG_STATE,state,score1,score2,p1Mode,p2Mode,ballX,ballY - Stato gioco Pong
 *   SNAKE_STATE,state,score,highScore,level,length,foodX,foodY,foodType,direction,playerJoined - Stato gioco Snake
 *   SCHEDULED_TEXTS,count,id1,text1,color1,hour1,min1,repeat1,year1,month1,day1,loop1,enabled1,... - Lista scritte programmate
 */
class CommandHandler {
public:
    CommandHandler();

    void init(TimeManager* time, EffectManager* effects, DisplayManager* display, Settings* settings, WiFiManager* wifi, ImageManager* imgMgr = nullptr, TextScheduleManager* schedMgr = nullptr);
    void setWebSocketManager(WebSocketManager* ws);
    void setScrollTextEffect(ScrollTextEffect* scrollText);
    void setPongEffect(PongEffect* pong);
    void setSnakeEffect(SnakeEffect* snake);
    
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

    // OTA Watchdog
    void checkOTAWatchdog();              // Chiamare nel loop per monitorare timeout
    static void checkOTABootStatus();     // Chiamare nel setup per verificare boot dopo OTA

private:
    TimeManager* _timeManager;
    EffectManager* _effectManager;
    DisplayManager* _displayManager;
    Settings* _settings;
    WiFiManager* _wifiManager;
    WebSocketManager* _wsManager;
    ImageManager* _imageManager;
    TextScheduleManager* _scheduleManager;
    ScrollTextEffect* _scrollTextEffect;
    PongEffect* _pongEffect;
    SnakeEffect* _snakeEffect;
    
    // Parser helper
    ParsedCommand splitCommand(const String& cmd, char delimiter = ',');

    // Handler specifici
    String handleSetTime(const ParsedCommand& parts);
    String handleSetDateTime(const ParsedCommand& parts);
    // String handleSetMode(const ParsedCommand& parts);
    String handleEffect(const ParsedCommand& parts);
    String handleBrightness(const ParsedCommand& parts);
    String handleNightTime(const ParsedCommand& parts);
    String handleDuration(const ParsedCommand& parts);
    String handleAutoSwitch(const ParsedCommand& parts);
    String handleWiFi(const ParsedCommand& parts);
    String handleDeviceName(const ParsedCommand& parts);
    String handleScrollText(const ParsedCommand& parts);
    String handlePong(const ParsedCommand& parts);
    String handleSnake(const ParsedCommand& parts);
    String handleNTP(const ParsedCommand& parts);
    String handleTimezone(const ParsedCommand& parts);
    String handleSave();
    String handleRestart();
    String handleOTA(const ParsedCommand& parts);
    String handleImage(const ParsedCommand& parts);
    String handleScheduledText(const ParsedCommand& parts);
    String handleWiFiScan();

    // OTA state
    bool _otaInProgress;
    size_t _otaSize;
    size_t _otaWritten;
    int _otaExpectedChunk;
    String _otaExpectedMD5;
    unsigned long _otaStartTime;      // Timestamp inizio OTA
    unsigned long _otaLastActivity;   // Ultima attività OTA

    // Base64 decode helper
    size_t base64Decode(const String& input, uint8_t* output, size_t maxLen);

    // OTA watchdog constants
    static constexpr unsigned long OTA_TIMEOUT_MS = 300000;        // 5 minuti timeout totale
    static constexpr unsigned long OTA_CHUNK_TIMEOUT_MS = 30000;   // 30s timeout tra chunk
};

#endif // COMMAND_HANDLER_H