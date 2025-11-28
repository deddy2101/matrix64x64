#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>
#include "TimeManager.h"
#include "EffectManager.h"
#include "DisplayManager.h"
#include "Settings.h"
#include "WiFiManager.h"

// Callback per inviare risposte (usato da WebSocket)
typedef std::function<void(const String& response)> ResponseCallback;

class CommandHandler {
private:
    TimeManager* timeManager;
    EffectManager* effectManager;
    DisplayManager* displayManager;
    Settings* settings;
    WiFiManager* wifiManager;
    
    ResponseCallback responseCallback;
    
    // Processa comandi JSON
    void processJsonCommand(const JsonDocument& doc, String& response);
    
    // Comandi specifici
    void cmdSetDateTime(const JsonObject& params, JsonDocument& response);
    void cmdSetTime(const JsonObject& params, JsonDocument& response);
    void cmdSetMode(const JsonObject& params, JsonDocument& response);
    void cmdEffect(const JsonObject& params, JsonDocument& response);
    void cmdSetBrightness(const JsonObject& params, JsonDocument& response);
    void cmdSetNightTime(const JsonObject& params, JsonDocument& response);
    void cmdSetEffectDuration(const JsonObject& params, JsonDocument& response);
    void cmdSetAutoSwitch(const JsonObject& params, JsonDocument& response);
    void cmdSetWiFi(const JsonObject& params, JsonDocument& response);
    void cmdGetStatus(JsonDocument& response);
    void cmdGetSettings(JsonDocument& response);
    void cmdGetEffects(JsonDocument& response);
    void cmdSaveSettings(JsonDocument& response);
    void cmdRestart(JsonDocument& response);
    
    // Genera risposta di errore
    void errorResponse(JsonDocument& response, const char* message);
    void ackResponse(JsonDocument& response, const char* cmd, bool success = true);
    
public:
    CommandHandler();
    
    // Inizializzazione
    void begin(TimeManager* tm, EffectManager* em, DisplayManager* dm, 
               Settings* settings, WiFiManager* wm);
    
    // Imposta callback per risposte (per WebSocket)
    void setResponseCallback(ResponseCallback callback) { responseCallback = callback; }
    
    // Processa comando JSON (da WebSocket)
    String processJson(const String& json);
    
    // Processa comando seriale (legacy format)
    bool processSerial(const String& cmd);
    
    // Aggiorna luminosità in base all'ora
    void updateBrightness();
};

#endif
