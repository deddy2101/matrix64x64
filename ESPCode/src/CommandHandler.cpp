#include "CommandHandler.h"

CommandHandler::CommandHandler() 
    : timeManager(nullptr),
      effectManager(nullptr),
      displayManager(nullptr),
      settings(nullptr),
      wifiManager(nullptr),
      responseCallback(nullptr) {
}

void CommandHandler::begin(TimeManager* tm, EffectManager* em, DisplayManager* dm,
                           Settings* s, WiFiManager* wm) {
    timeManager = tm;
    effectManager = em;
    displayManager = dm;
    settings = s;
    wifiManager = wm;
    
    Serial.println(F("[CommandHandler] Initialized"));
}

// ═══════════════════════════════════════════════════════════════
// JSON Processing
// ═══════════════════════════════════════════════════════════════

String CommandHandler::processJson(const String& json) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    
    JsonDocument response;
    
    if (error) {
        errorResponse(response, "Invalid JSON");
        String output;
        serializeJson(response, output);
        return output;
    }
    
    if (!doc.containsKey("cmd")) {
        errorResponse(response, "Missing 'cmd' field");
        String output;
        serializeJson(response, output);
        return output;
    }
    
    String cmd = doc["cmd"].as<String>();
    JsonObject params = doc.as<JsonObject>();
    
    // Route del comando
    if (cmd == "setDateTime") {
        cmdSetDateTime(params, response);
    }
    else if (cmd == "setTime") {
        cmdSetTime(params, response);
    }
    else if (cmd == "setMode") {
        cmdSetMode(params, response);
    }
    else if (cmd == "effect") {
        cmdEffect(params, response);
    }
    else if (cmd == "setBrightness") {
        cmdSetBrightness(params, response);
    }
    else if (cmd == "setNightTime") {
        cmdSetNightTime(params, response);
    }
    else if (cmd == "setEffectDuration") {
        cmdSetEffectDuration(params, response);
    }
    else if (cmd == "setAutoSwitch") {
        cmdSetAutoSwitch(params, response);
    }
    else if (cmd == "setWiFi") {
        cmdSetWiFi(params, response);
    }
    else if (cmd == "getStatus") {
        cmdGetStatus(response);
    }
    else if (cmd == "getSettings") {
        cmdGetSettings(response);
    }
    else if (cmd == "getEffects") {
        cmdGetEffects(response);
    }
    else if (cmd == "saveSettings") {
        cmdSaveSettings(response);
    }
    else if (cmd == "restart") {
        cmdRestart(response);
    }
    else {
        errorResponse(response, "Unknown command");
    }
    
    String output;
    serializeJson(response, output);
    return output;
}

// ═══════════════════════════════════════════════════════════════
// Command Implementations
// ═══════════════════════════════════════════════════════════════

void CommandHandler::cmdSetDateTime(const JsonObject& params, JsonDocument& response) {
    if (!params.containsKey("year") || !params.containsKey("month") || 
        !params.containsKey("day") || !params.containsKey("hour") || 
        !params.containsKey("minute")) {
        errorResponse(response, "Missing date/time parameters");
        return;
    }
    
    int year = params["year"];
    int month = params["month"];
    int day = params["day"];
    int hour = params["hour"];
    int minute = params["minute"];
    int second = params["second"] | 0;
    
    timeManager->setDateTime(year, month, day, hour, minute, second);
    ackResponse(response, "setDateTime");
}

void CommandHandler::cmdSetTime(const JsonObject& params, JsonDocument& response) {
    if (!params.containsKey("hour") || !params.containsKey("minute")) {
        errorResponse(response, "Missing hour/minute");
        return;
    }
    
    int hour = params["hour"];
    int minute = params["minute"];
    int second = params["second"] | 0;
    
    timeManager->setTime(hour, minute, second);
    ackResponse(response, "setTime");
}

void CommandHandler::cmdSetMode(const JsonObject& params, JsonDocument& response) {
    if (!params.containsKey("mode")) {
        errorResponse(response, "Missing mode parameter");
        return;
    }
    
    String mode = params["mode"].as<String>();
    
    if (mode == "rtc" || mode == "RTC") {
        timeManager->setMode(TimeMode::RTC);
    } else if (mode == "fake" || mode == "FAKE") {
        timeManager->setMode(TimeMode::FAKE);
    } else {
        errorResponse(response, "Invalid mode (use 'rtc' or 'fake')");
        return;
    }
    
    ackResponse(response, "setMode");
}

void CommandHandler::cmdEffect(const JsonObject& params, JsonDocument& response) {
    if (!params.containsKey("action")) {
        errorResponse(response, "Missing action parameter");
        return;
    }
    
    String action = params["action"].as<String>();
    
    if (action == "next") {
        effectManager->nextEffect();
    }
    else if (action == "pause") {
        effectManager->pause();
    }
    else if (action == "resume") {
        effectManager->resume();
    }
    else if (action == "select") {
        if (params.containsKey("index")) {
            int index = params["index"];
            effectManager->switchToEffect(index);
            settings->setCurrentEffect(index);
        } else if (params.containsKey("name")) {
            String name = params["name"].as<String>();
            effectManager->switchToEffect(name.c_str());
        } else {
            errorResponse(response, "Missing index or name for select");
            return;
        }
    }
    else {
        errorResponse(response, "Invalid action");
        return;
    }
    
    ackResponse(response, "effect");
}

void CommandHandler::cmdSetBrightness(const JsonObject& params, JsonDocument& response) {
    bool updated = false;
    
    if (params.containsKey("day")) {
        settings->setBrightnessDay(params["day"]);
        updated = true;
    }
    if (params.containsKey("night")) {
        settings->setBrightnessNight(params["night"]);
        updated = true;
    }
    if (params.containsKey("value")) {
        // Imposta direttamente
        displayManager->setBrightness(params["value"]);
    }
    
    if (updated) {
        updateBrightness();
    }
    
    ackResponse(response, "setBrightness");
}

void CommandHandler::cmdSetNightTime(const JsonObject& params, JsonDocument& response) {
    if (!params.containsKey("start") || !params.containsKey("end")) {
        errorResponse(response, "Missing start/end hours");
        return;
    }
    
    settings->setNightHours(params["start"], params["end"]);
    updateBrightness();
    
    ackResponse(response, "setNightTime");
}

void CommandHandler::cmdSetEffectDuration(const JsonObject& params, JsonDocument& response) {
    if (!params.containsKey("ms")) {
        errorResponse(response, "Missing ms parameter");
        return;
    }
    
    unsigned long duration = params["ms"];
    settings->setEffectDuration(duration);
    effectManager->setDuration(duration);
    
    ackResponse(response, "setEffectDuration");
}

void CommandHandler::cmdSetAutoSwitch(const JsonObject& params, JsonDocument& response) {
    if (!params.containsKey("enabled")) {
        errorResponse(response, "Missing enabled parameter");
        return;
    }
    
    bool enabled = params["enabled"];
    settings->setAutoSwitch(enabled);
    
    if (enabled) {
        effectManager->resume();
    } else {
        effectManager->pause();
    }
    
    ackResponse(response, "setAutoSwitch");
}

void CommandHandler::cmdSetWiFi(const JsonObject& params, JsonDocument& response) {
    if (params.containsKey("ssid") && params.containsKey("password")) {
        String ssid = params["ssid"].as<String>();
        String password = params["password"].as<String>();
        bool apMode = params["apMode"] | false;
        
        settings->setSSID(ssid.c_str());
        settings->setPassword(password.c_str());
        settings->setAPMode(apMode);
        settings->save();
        
        ackResponse(response, "setWiFi");
        response["message"] = "WiFi settings saved. Restart to apply.";
    }
    else if (params.containsKey("apMode")) {
        settings->setAPMode(params["apMode"]);
        ackResponse(response, "setWiFi");
    }
    else {
        errorResponse(response, "Missing ssid/password");
    }
}

void CommandHandler::cmdGetStatus(JsonDocument& response) {
    response["type"] = "status";
    
    // Time
    response["time"] = timeManager->getTimeString();
    response["date"] = timeManager->getDateString();
    response["timeMode"] = timeManager->getModeString();
    response["ds3231"] = timeManager->isDS3231Available();
    
    if (timeManager->isDS3231Available()) {
        response["temperature"] = timeManager->getDS3231Temperature();
    }
    
    // Effect
    Effect* current = effectManager->getCurrentEffect();
    if (current) {
        response["effect"] = current->getName();
        response["effectIndex"] = effectManager->getCurrentEffectIndex();
        response["fps"] = current->getFPS();
    }
    response["autoSwitch"] = effectManager->isAutoSwitch();
    response["effectCount"] = effectManager->getEffectCount();
    
    // Display
    response["brightness"] = settings->getCurrentBrightness(timeManager->getHour());
    response["isNight"] = settings->isNightTime(timeManager->getHour());
    
    // WiFi
    response["wifiStatus"] = wifiManager->getStatusString();
    response["ip"] = wifiManager->getIP();
    response["ssid"] = wifiManager->getSSID();
    if (!wifiManager->isAPMode()) {
        response["rssi"] = wifiManager->getRSSI();
    }
    
    // System
    response["uptime"] = millis() / 1000;
    response["freeHeap"] = ESP.getFreeHeap();
}

void CommandHandler::cmdGetSettings(JsonDocument& response) {
    response["type"] = "settings";
    
    // Parse settings JSON into response
    String settingsJson = settings->toJson();
    JsonDocument settingsDoc;
    deserializeJson(settingsDoc, settingsJson);
    
    response["data"] = settingsDoc;
}

void CommandHandler::cmdGetEffects(JsonDocument& response) {
    response["type"] = "effects";
    
    JsonArray effects = response["list"].to<JsonArray>();
    for (int i = 0; i < effectManager->getEffectCount(); i++) {
        JsonObject effect = effects.add<JsonObject>();
        effect["index"] = i;
        effect["name"] = effectManager->getEffectName(i);
        effect["current"] = (i == effectManager->getCurrentEffectIndex());
    }
}

void CommandHandler::cmdSaveSettings(JsonDocument& response) {
    settings->save();
    ackResponse(response, "saveSettings");
}

void CommandHandler::cmdRestart(JsonDocument& response) {
    ackResponse(response, "restart");
    response["message"] = "Restarting in 1 second...";
    
    // Invia risposta prima di riavviare
    String output;
    serializeJson(response, output);
    if (responseCallback) {
        responseCallback(output);
    }
    
    delay(1000);
    ESP.restart();
}

// ═══════════════════════════════════════════════════════════════
// Helper Methods
// ═══════════════════════════════════════════════════════════════

void CommandHandler::errorResponse(JsonDocument& response, const char* message) {
    response["type"] = "error";
    response["message"] = message;
}

void CommandHandler::ackResponse(JsonDocument& response, const char* cmd, bool success) {
    response["type"] = "ack";
    response["cmd"] = cmd;
    response["success"] = success;
}

void CommandHandler::updateBrightness() {
    if (displayManager && settings && timeManager) {
        uint8_t brightness = settings->getCurrentBrightness(timeManager->getHour());
        displayManager->setBrightness(brightness);
    }
}

// ═══════════════════════════════════════════════════════════════
// Serial Command Processing (Legacy)
// ═══════════════════════════════════════════════════════════════

bool CommandHandler::processSerial(const String& cmd) {
    if (cmd.length() == 0) return false;
    
    char first = cmd.charAt(0);
    
    // Prima prova TimeManager
    if (timeManager->parseCommand(cmd)) {
        return true;
    }
    
    // Poi comandi effetti
    switch (first) {
        case 'p':
            effectManager->pause();
            Serial.println(F("[CMD] Paused"));
            return true;
            
        case 'r':
            effectManager->resume();
            Serial.println(F("[CMD] Resumed"));
            return true;
            
        case 'n':
            effectManager->nextEffect();
            Serial.println(F("[CMD] Next effect"));
            return true;
            
        case '0'...'9':
            effectManager->switchToEffect(first - '0');
            return true;
            
        case 'b':
        case 'B': {
            // Brightness: B200 o b200
            int brightness = cmd.substring(1).toInt();
            if (brightness >= 0 && brightness <= 255) {
                displayManager->setBrightness(brightness);
                Serial.printf("[CMD] Brightness: %d\n", brightness);
            }
            return true;
        }
        
        case 'w':
        case 'W':
            // WiFi info
            Serial.printf("[WiFi] Status: %s\n", wifiManager->getStatusString().c_str());
            Serial.printf("[WiFi] IP: %s\n", wifiManager->getIP().c_str());
            return true;
    }
    
    return false;
}
