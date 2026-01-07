#include "CommandHandler.h"
#include "WebSocketManager.h"
#include "Version.h"

CommandHandler::CommandHandler()
    : _timeManager(nullptr)
    , _effectManager(nullptr)
    , _displayManager(nullptr)
    , _settings(nullptr)
    , _wifiManager(nullptr)
    , _wsManager(nullptr)
    , _otaInProgress(false)
    , _otaSize(0)
    , _otaWritten(0)
    , _otaExpectedChunk(0)
    , _otaExpectedMD5("")
{}

void CommandHandler::init(TimeManager* time, EffectManager* effects, DisplayManager* display, Settings* settings, WiFiManager* wifi) {
    _timeManager = time;
    _effectManager = effects;
    _displayManager = display;
    _settings = settings;
    _wifiManager = wifi;
}

void CommandHandler::setWebSocketManager(WebSocketManager* ws) {
    _wsManager = ws;
}

// ═══════════════════════════════════════════
// Parser Helper
// ═══════════════════════════════════════════

std::vector<String> CommandHandler::splitCommand(const String& cmd, char delimiter) {
    std::vector<String> parts;
    int start = 0;
    int end = cmd.indexOf(delimiter);
    
    while (end != -1) {
        parts.push_back(cmd.substring(start, end));
        start = end + 1;
        end = cmd.indexOf(delimiter, start);
    }
    parts.push_back(cmd.substring(start));
    
    return parts;
}

// ═══════════════════════════════════════════
// Process Commands
// ═══════════════════════════════════════════

String CommandHandler::processCommand(const String& command) {
    String cmd = command;
    cmd.trim();
    
    if (cmd.isEmpty()) {
        return "ERR,empty command";
    }
    
    DEBUG_PRINTF("[CMD] Processing: %s\n", cmd.c_str());
    
    std::vector<String> parts = splitCommand(cmd, ',');
    String mainCmd = parts[0];
    mainCmd.toLowerCase();
    
    // ─────────────────────────────────────────
    // Query Commands (no parameters)
    // ─────────────────────────────────────────
    if (mainCmd == "getstatus") {
        return getStatusResponse();
    }
    if (mainCmd == "geteffects") {
        return getEffectsResponse();
    }
    if (mainCmd == "getsettings") {
        return getSettingsResponse();
    }
    if (mainCmd == "getversion") {
        return getVersionResponse();
    }
    
    // ─────────────────────────────────────────
    // Action Commands
    // ─────────────────────────────────────────
    if (mainCmd == "settime") {
        return handleSetTime(parts);
    }
    if (mainCmd == "setdatetime") {
        return handleSetDateTime(parts);
    }
    if (mainCmd == "setmode") {
        return handleSetMode(parts);
    }
    if (mainCmd == "effect") {
        return handleEffect(parts);
    }
    if (mainCmd == "brightness") {
        return handleBrightness(parts);
    }
    if (mainCmd == "nighttime") {
        return handleNightTime(parts);
    }
    if (mainCmd == "duration") {
        return handleDuration(parts);
    }
    if (mainCmd == "autoswitch") {
        return handleAutoSwitch(parts);
    }
    if (mainCmd == "wifi") {
        return handleWiFi(parts);
    }
    if (mainCmd == "devicename") {
        return handleDeviceName(parts);
    }
    if (mainCmd == "save") {
        return handleSave();
    }
    if (mainCmd == "restart") {
        return handleRestart();
    }
    if (mainCmd == "ota") {
        return handleOTA(parts);
    }

    return "ERR,unknown command: " + mainCmd;
}

// ═══════════════════════════════════════════
// Legacy Serial Commands (single char)
// ═══════════════════════════════════════════

String CommandHandler::processLegacyCommand(const String& command) {
    if (command.length() == 0) return "";
    
    char cmd = command.charAt(0);
    
    switch (cmd) {
        case 'T':
        case 't':
            return getStatusResponse();
            
        case 'D':
        case 'd':
            if (_timeManager) {
                _timeManager->printHelp();
            }
            return "OK";
            
        case 'E':
        case 'e':
            if (_effectManager) {
                _effectManager->nextEffect();
            }
            return "OK,next effect";
            
        case 'M':
        case 'm':
            if (_timeManager) {
                _timeManager->setMode(_timeManager->getMode() == TimeMode::FAKE ? TimeMode::RTC : TimeMode::FAKE);
            }
            return "OK,mode toggled";
            
        case 'S':
        case 's':
            if (_settings) {
                _settings->save();
            }
            return "OK,settings saved";
            
        case '?':
            DEBUG_PRINTLN(F("\n=== LED Matrix Commands ==="));
            DEBUG_PRINTLN(F("T - Time status"));
            DEBUG_PRINTLN(F("D - Time debug"));
            DEBUG_PRINTLN(F("E - Next effect"));
            DEBUG_PRINTLN(F("M - Toggle time mode"));
            DEBUG_PRINTLN(F("S - Save settings"));
            DEBUG_PRINTLN(F("P - Pause auto-switch"));
            DEBUG_PRINTLN(F("R - Resume auto-switch"));
            DEBUG_PRINTLN(F("N - Next effect"));
            DEBUG_PRINTLN(F("0-9 - Select effect"));
            DEBUG_PRINTLN(F("\nCSV Commands: getStatus, effect,next, brightness,200, etc."));
            return "OK";
            
        case 'P':
        case 'p':
            if (_effectManager) {
                _effectManager->pause();
            }
            return "OK,paused";
            
        case 'R':
        case 'r':
            if (_effectManager) {
                _effectManager->resume();
            }
            return "OK,resumed";
            
        case 'N':
        case 'n':
            if (_effectManager) {
                _effectManager->nextEffect();
            }
            return "OK,next effect";
            
        default:
            // Check if digit (0-9) for effect selection
            if (cmd >= '0' && cmd <= '9') {
                int index = cmd - '0';
                if (_effectManager && index < _effectManager->getEffectCount()) {
                    _effectManager->switchToEffect(index);
                    return "OK,effect " + String(index);
                }
            }
            break;
    }
    
    return "";
}

// ═══════════════════════════════════════════
// Response Generators
// ═══════════════════════════════════════════

String CommandHandler::getStatusResponse() {
    String response = "STATUS";
    
    // Time
    if (_timeManager) {
        response += "," + _timeManager->getTimeString();
        response += "," + _timeManager->getDateString();
        response += "," + _timeManager->getModeString();
        response += "," + String(_timeManager->isDS3231Available() ? "1" : "0");
        if (_timeManager->isDS3231Available()) {
            response += "," + String(_timeManager->getDS3231Temperature(), 1);
        } else {
            response += ",0";
        }
    } else {
        response += ",--:--:--,--/--/----,---,0,0";
    }
    
    // Effect
    if (_effectManager) {
        Effect* current = _effectManager->getCurrentEffect();
        if (current) {
            response += "," + String(current->getName());
            response += "," + String(_effectManager->getCurrentEffectIndex());
            response += "," + String(current->getFPS(), 1);
        } else {
            response += ",none,-1,0";
        }
        response += "," + String(_effectManager->isAutoSwitch() ? "1" : "0");
        response += "," + String(_effectManager->getEffectCount());
    } else {
        response += ",none,-1,0,0,0";
    }
    
    // Brightness
    if (_settings && _timeManager) {
        int currentBright = _settings->getCurrentBrightness(_timeManager->getHour());
        response += "," + String(currentBright);
        response += "," + String(_settings->isNightTime(_timeManager->getHour()) ? "1" : "0");
    } else {
        response += ",0,0";
    }
    
    // WiFi
    if (_wifiManager) {
        response += "," + _wifiManager->getStatusString();
        response += "," + _wifiManager->getIP();
        response += "," + _wifiManager->getSSID();
        response += "," + String(_wifiManager->getRSSI());
    } else {
        response += ",disconnected,0.0.0.0,none,0";
    }
    
    // System
    response += "," + String(millis() / 1000);
    response += "," + String(ESP.getFreeHeap());
    
    return response;
}

String CommandHandler::getEffectsResponse() {
    String response = "EFFECTS";
    
    if (_effectManager) {
        for (int i = 0; i < _effectManager->getEffectCount(); i++) {
            response += ",";
            response += _effectManager->getEffectName(i);
        }
    }
    
    return response;
}

String CommandHandler::getSettingsResponse() {
    String response = "SETTINGS";

    if (_settings) {
        response += "," + String(_settings->getSSID());
        response += "," + String(_settings->isAPMode() ? "1" : "0");
        response += "," + String(_settings->getBrightnessDay());
        response += "," + String(_settings->getBrightnessNight());
        response += "," + String(_settings->getNightStartHour());
        response += "," + String(_settings->getNightEndHour());
        response += "," + String(_settings->getEffectDuration());
        response += "," + String(_settings->isAutoSwitch() ? "1" : "0");
        response += "," + String(_settings->getCurrentEffect());
        response += "," + String(_settings->getDeviceName());
    }

    return response;
}

String CommandHandler::getVersionResponse() {
    String response = "VERSION";
    response += "," + String(FIRMWARE_VERSION);
    response += "," + String(FIRMWARE_BUILD_NUMBER);
    response += "," + String(FIRMWARE_BUILD_DATE);
    response += "," + String(FIRMWARE_BUILD_TIME);
    return response;
}

String CommandHandler::getEffectChangeNotification() {
    if (_effectManager) {
        Effect* current = _effectManager->getCurrentEffect();
        if (current) {
            return "EFFECT," + String(_effectManager->getCurrentEffectIndex()) + "," + String(current->getName());
        }
    }
    return "EFFECT,-1,none";
}

String CommandHandler::getTimeChangeNotification() {
    if (_timeManager) {
        return "TIME," + _timeManager->getTimeString();
    }
    return "TIME,--:--:--";
}

// ═══════════════════════════════════════════
// Command Handlers
// ═══════════════════════════════════════════

String CommandHandler::handleSetTime(const std::vector<String>& parts) {
    if (parts.size() < 4) {
        return "ERR,settime needs HH,MM,SS";
    }
    
    int h = parts[1].toInt();
    int m = parts[2].toInt();
    int s = parts[3].toInt();
    
    if (h < 0 || h > 23 || m < 0 || m > 59 || s < 0 || s > 59) {
        return "ERR,invalid time values";
    }
    
    if (_timeManager) {
        _timeManager->setTime(h, m, s);
        if (_wsManager) {
            _wsManager->notifyTimeChange();
        }
        return "OK,time set";
    }
    
    return "ERR,time manager not available";
}

String CommandHandler::handleSetDateTime(const std::vector<String>& parts) {
    if (parts.size() < 7) {
        return "ERR,setdatetime needs YYYY,MM,DD,HH,MM,SS";
    }
    
    int year = parts[1].toInt();
    int month = parts[2].toInt();
    int day = parts[3].toInt();
    int h = parts[4].toInt();
    int m = parts[5].toInt();
    int s = parts[6].toInt();
    
    if (_timeManager) {
        _timeManager->setDateTime(year, month, day, h, m, s);
        if (_wsManager) {
            _wsManager->notifyTimeChange();
        }
        return "OK,datetime set";
    }
    
    return "ERR,time manager not available";
}

String CommandHandler::handleSetMode(const std::vector<String>& parts) {
    if (parts.size() < 2) {
        return "ERR,setmode needs rtc|fake";
    }
    
    String mode = parts[1];
    mode.toLowerCase();
    
    if (_timeManager) {
        if (mode == "rtc") {
            _timeManager->setMode(TimeMode::RTC);
            return "OK,mode rtc";
        } else if (mode == "fake") {
            _timeManager->setMode(TimeMode::FAKE);
            return "OK,mode fake";
        }
    }
    
    return "ERR,invalid mode (use rtc or fake)";
}

String CommandHandler::handleEffect(const std::vector<String>& parts) {
    if (parts.size() < 2) {
        return "ERR,effect needs parameter";
    }
    
    String action = parts[1];
    action.toLowerCase();
    
    if (!_effectManager) {
        return "ERR,effect manager not available";
    }
    
    if (action == "next") {
        _effectManager->nextEffect();
        if (_wsManager) {
            _wsManager->notifyEffectChange();
        }
        return "OK,next effect";
    }
    
    if (action == "pause") {
        _effectManager->pause();
        if (_settings) {
            _settings->setAutoSwitch(false);
        }
        return "OK,paused";
    }
    
    if (action == "resume") {
        _effectManager->resume();
        if (_settings) {
            _settings->setAutoSwitch(true);
        }
        return "OK,resumed";
    }
    
    if (action == "select" && parts.size() >= 3) {
        int index = parts[2].toInt();
        if (index >= 0 && index < _effectManager->getEffectCount()) {
            _effectManager->switchToEffect(index);
            if (_settings) {
                _settings->setCurrentEffect(index);
            }
            if (_wsManager) {
                _wsManager->notifyEffectChange();
            }
            return "OK,effect " + String(index);
        }
        return "ERR,invalid effect index";
    }
    
    if (action == "name" && parts.size() >= 3) {
        String name = parts[2];
        for (int i = 3; i < parts.size(); i++) {
            name += "," + parts[i];
        }
        _effectManager->switchToEffect(name.c_str());
        if (_wsManager) {
            _wsManager->notifyEffectChange();
        }
        return "OK,effect " + name;
    }
    
    return "ERR,invalid effect action";
}

String CommandHandler::handleBrightness(const std::vector<String>& parts) {
    if (parts.size() < 2) {
        return "ERR,brightness needs value";
    }
    
    String type = parts[1];
    type.toLowerCase();
    
    if (type == "day" && parts.size() >= 3) {
        int value = parts[2].toInt();
        if (value >= 0 && value <= 255) {
            if (_settings) {
                _settings->setBrightnessDay(value);
                updateBrightness();
            }
            return "OK,brightness day " + String(value);
        }
        return "ERR,value must be 0-255";
    }
    
    if (type == "night" && parts.size() >= 3) {
        int value = parts[2].toInt();
        if (value >= 0 && value <= 255) {
            if (_settings) {
                _settings->setBrightnessNight(value);
                updateBrightness();
            }
            return "OK,brightness night " + String(value);
        }
        return "ERR,value must be 0-255";
    }
    
    // Immediate brightness change
    int value = parts[1].toInt();
    if (value >= 0 && value <= 255) {
        if (_displayManager) {
            _displayManager->setBrightness(value);
        }
        return "OK,brightness " + String(value);
    }
    
    return "ERR,invalid brightness command";
}

String CommandHandler::handleNightTime(const std::vector<String>& parts) {
    if (parts.size() < 3) {
        return "ERR,nighttime needs START,END";
    }
    
    int start = parts[1].toInt();
    int end = parts[2].toInt();
    
    if (start < 0 || start > 23 || end < 0 || end > 23) {
        return "ERR,hours must be 0-23";
    }
    
    if (_settings) {
        _settings->setNightHours(start, end);
        updateBrightness();
        return "OK,nighttime " + String(start) + "-" + String(end);
    }
    
    return "ERR,settings not available";
}

String CommandHandler::handleDuration(const std::vector<String>& parts) {
    if (parts.size() < 2) {
        return "ERR,duration needs MS";
    }
    
    unsigned long ms = parts[1].toInt();
    
    if (ms < 1000 || ms > 300000) {
        return "ERR,duration must be 1000-300000 ms";
    }
    
    if (_effectManager) {
        _effectManager->setDuration(ms);
    }
    if (_settings) {
        _settings->setEffectDuration(ms);
    }
    
    return "OK,duration " + String(ms);
}

String CommandHandler::handleAutoSwitch(const std::vector<String>& parts) {
    if (parts.size() < 2) {
        return "ERR,autoswitch needs 0|1";
    }
    
    bool enabled = parts[1].toInt() != 0;
    
    if (_effectManager) {
        _effectManager->setAutoSwitch(enabled);
    }
    if (_settings) {
        _settings->setAutoSwitch(enabled);
    }
    
    return enabled ? "OK,autoswitch on" : "OK,autoswitch off";
}

String CommandHandler::handleWiFi(const std::vector<String>& parts) {
    if (parts.size() < 4) {
        return "ERR,wifi needs SSID,PASSWORD,AP_MODE";
    }
    
    String ssid = parts[1];
    String password = parts[2];
    bool apMode = parts[3].toInt() != 0;
    
    if (_settings) {
        _settings->setSSID(ssid.c_str());
        _settings->setPassword(password.c_str());
        _settings->setAPMode(apMode);
    }
    
    if (_wifiManager) {
        if (apMode) {
            _wifiManager->switchToAP();
        } else {
            _wifiManager->switchToSTA(ssid.c_str(), password.c_str());
        }
    }
    
    return "OK,wifi configured (restart to apply)";
}

String CommandHandler::handleDeviceName(const std::vector<String>& parts) {
    if (parts.size() < 2) {
        return "ERR,devicename needs NAME";
    }
    
    String name = parts[1];
    
    if (_settings) {
        _settings->setDeviceName(name.c_str());
        return "OK,devicename " + name + " (restart to apply)";
    }
    
    return "ERR,settings not available";
}

String CommandHandler::handleSave() {
    if (_settings) {
        _settings->save();
        return "OK,settings saved";
    }
    return "ERR,settings not available";
}

String CommandHandler::handleRestart() {
    if (_settings) {
        _settings->save();
    }
    DEBUG_PRINTLN(F("[CMD] Restarting in 2 seconds..."));
    delay(2000);
    ESP.restart();
    return "OK,restarting";
}

// ═══════════════════════════════════════════
// Utility Functions
// ═══════════════════════════════════════════

void CommandHandler::updateBrightness() {
    if (_settings && _timeManager && _displayManager) {
        int hour = _timeManager->getHour();
        uint8_t brightness = _settings->getCurrentBrightness(hour);
        _displayManager->setBrightness(brightness);
        
        DEBUG_PRINTF("[Brightness] Updated to %d (hour=%d, night=%s)\n", 
                     brightness, hour, _settings->isNightTime(hour) ? "yes" : "no");
    }
}

void CommandHandler::processSerial(const String& cmd) {
    String response;

    // Try CSV command first
    if (cmd.indexOf(',') != -1 || cmd.length() > 1) {
        response = processCommand(cmd);
    } else {
        // Try legacy single-char command
        response = processLegacyCommand(cmd);
    }

    if (!response.isEmpty()) {
        DEBUG_PRINTLN(response);
    }
}

// ═══════════════════════════════════════════
// Base64 Decode Helper
// ═══════════════════════════════════════════

static const char base64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int base64CharIndex(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

size_t CommandHandler::base64Decode(const String& input, uint8_t* output, size_t maxLen) {
    size_t inputLen = input.length();
    if (inputLen == 0 || inputLen % 4 != 0) {
        return 0;
    }

    size_t outputLen = (inputLen / 4) * 3;
    if (input[inputLen - 1] == '=') outputLen--;
    if (input[inputLen - 2] == '=') outputLen--;

    if (outputLen > maxLen) {
        return 0;
    }

    size_t j = 0;
    for (size_t i = 0; i < inputLen; i += 4) {
        int a = base64CharIndex(input[i]);
        int b = base64CharIndex(input[i + 1]);
        int c = (input[i + 2] != '=') ? base64CharIndex(input[i + 2]) : 0;
        int d = (input[i + 3] != '=') ? base64CharIndex(input[i + 3]) : 0;

        if (a < 0 || b < 0) return 0;

        uint32_t triple = (a << 18) | (b << 12) | (c << 6) | d;

        if (j < outputLen) output[j++] = (triple >> 16) & 0xFF;
        if (j < outputLen) output[j++] = (triple >> 8) & 0xFF;
        if (j < outputLen) output[j++] = triple & 0xFF;
    }

    return outputLen;
}

// ═══════════════════════════════════════════
// OTA Update Handler with ACK protocol
// ═══════════════════════════════════════════

String CommandHandler::handleOTA(const std::vector<String>& parts) {
    if (parts.size() < 2) {
        return "ERR,OTA requires subcommand";
    }

    String subCmd = parts[1];
    subCmd.toLowerCase();

    // ota,start,SIZE
    if (subCmd == "start") {
        if (parts.size() < 3) {
            return "ERR,OTA start requires size parameter";
        }

        _otaSize = parts[2].toInt();
        if (_otaSize == 0 || _otaSize > 2000000) { // Max 2MB
            return "ERR,Invalid firmware size";
        }

        DEBUG_PRINTF("[OTA] Starting update, size: %d bytes\n", _otaSize);

        // Pausa effect manager durante OTA
        if (_effectManager) {
            _effectManager->pause();
        }

        // Inizializza Update con UPDATE_SIZE_UNKNOWN per ESP32
        if (!Update.begin(_otaSize, U_FLASH)) {
            DEBUG_PRINTF("[OTA] Update.begin() failed! Error: %d\n", Update.getError());
            if (_effectManager) {
                _effectManager->resume();
            }
            return "ERR,OTA init failed";
        }

        _otaInProgress = true;
        _otaWritten = 0;
        _otaExpectedChunk = 0;

        // Mostra "OTA" sul display
        if (_displayManager) {
            _displayManager->showOTAProgress(0);
        }

        return "OTA_READY";
    }

    // ota,data,CHUNK_NUM,BASE64_CHUNK
    else if (subCmd == "data") {
        if (!_otaInProgress) {
            return "ERR,No OTA in progress";
        }

        if (parts.size() < 4) {
            return "ERR,OTA data requires chunk_num and data";
        }

        // Get chunk number
        int chunkNum = parts[2].toInt();

        // Check sequence
        if (chunkNum != _otaExpectedChunk) {
            DEBUG_PRINTF("[OTA] Wrong chunk! Expected %d, got %d\n", _otaExpectedChunk, chunkNum);
            return "OTA_NACK," + String(_otaExpectedChunk);
        }

        // Decode base64
        String base64Chunk = parts[3];

        DEBUG_PRINTF("[OTA] Chunk %d, base64 length: %d\n", chunkNum, base64Chunk.length());

        // Buffer per dati decodificati (max 4KB raw = ~5.5KB base64)
        static uint8_t decodedBuffer[4096];
        size_t decodedLen = base64Decode(base64Chunk, decodedBuffer, sizeof(decodedBuffer));

        if (decodedLen == 0) {
            DEBUG_PRINTF("[OTA] Base64 decode failed! Chunk %d, input len: %d\n",
                        chunkNum, base64Chunk.length());
            return "OTA_NACK," + String(chunkNum);
        }

        DEBUG_PRINTF("[OTA] Decoded %d bytes\n", decodedLen);

        // Write decoded chunk
        size_t written = Update.write(decodedBuffer, decodedLen);

        if (written != decodedLen) {
            Update.abort();
            _otaInProgress = false;
            if (_effectManager) {
                _effectManager->resume();
            }
            DEBUG_PRINTF("[OTA] Write failed! Expected %d, wrote %d\n", decodedLen, written);
            return "ERR,Write failed";
        }

        _otaWritten += written;
        _otaExpectedChunk++;
        int percent = (_otaWritten * 100) / _otaSize;

        // Aggiorna display ogni 5%
        if (_displayManager && percent % 5 == 0) {
            _displayManager->showOTAProgress(percent);
        }

        // Log ogni 10%
        if (percent % 10 == 0) {
            DEBUG_PRINTF("[OTA] Progress: %d/%d bytes (%d%%)\n", _otaWritten, _otaSize, percent);
        }

        // Invia ACK con numero chunk
        return "OTA_ACK," + String(chunkNum);
    }

    // ota,end,MD5
    else if (subCmd == "end") {
        if (!_otaInProgress) {
            return "ERR,No OTA in progress";
        }

        String expectedMD5 = "";
        if (parts.size() >= 3) {
            expectedMD5 = parts[2];
        }

        DEBUG_PRINTF("[OTA] Finalizing update... Written: %d/%d bytes\n", _otaWritten, _otaSize);

        if (!Update.end(true)) {
            Update.abort();
            _otaInProgress = false;
            if (_effectManager) {
                _effectManager->resume();
            }
            DEBUG_PRINTF("[OTA] Update.end() failed! Error: %d\n", Update.getError());
            return "ERR,OTA finalization failed";
        }

        // Verifica MD5 se fornito
        if (expectedMD5.length() > 0) {
            String actualMD5 = Update.md5String();
            if (!actualMD5.equalsIgnoreCase(expectedMD5)) {
                if (_effectManager) {
                    _effectManager->resume();
                }
                DEBUG_PRINTF("[OTA] MD5 mismatch! Expected: %s, Got: %s\n",
                           expectedMD5.c_str(), actualMD5.c_str());
                return "ERR,MD5 verification failed";
            }
            DEBUG_PRINTLN("[OTA] MD5 verified OK");
        }

        _otaInProgress = false;

        // Mostra successo sul display
        if (_displayManager) {
            _displayManager->showOTASuccess();
        }

        DEBUG_PRINTLN("[OTA] Update SUCCESS! Rebooting in 3 seconds...");

        // Riavvia dopo 3 secondi
        delay(3000);
        ESP.restart();

        return "OTA_SUCCESS";
    }

    // ota,abort
    else if (subCmd == "abort") {
        if (_otaInProgress) {
            Update.abort();
            _otaInProgress = false;
            _otaExpectedChunk = 0;
            if (_effectManager) {
                _effectManager->resume();
            }
            DEBUG_PRINTLN("[OTA] Update aborted");
            return "OK,OTA aborted";
        }
        return "ERR,No OTA in progress";
    }

    return "ERR,Unknown OTA subcommand: " + subCmd;
}