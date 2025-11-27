#include "TimeManager.h"

TimeManager::TimeManager(bool fakeTime, unsigned long fakeSpeedMs)
    : currentHour(12),
      currentMinute(0),
      currentSecond(0),
      lastHour(-1),
      lastMinute(-1),
      lastSecond(-1),
      lastUpdate(0),
      updateInterval(fakeSpeedMs),
      mode(fakeTime ? TimeMode::FAKE : TimeMode::RTC),
      onSecondChange(nullptr),
      onMinuteChange(nullptr),
      onHourChange(nullptr) {
}

void TimeManager::begin(int hour, int minute, int second) {
    currentHour = hour;
    currentMinute = minute;
    currentSecond = second;
    lastUpdate = millis();
    
    // Configura timezone Italia
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    
    if (mode == TimeMode::RTC) {
        // Imposta l'RTC con l'ora iniziale
        setTime(hour, minute, second);
    }
    
    Serial.println(F(""));
    Serial.println(F("╔════════════════════════════════════════════════════╗"));
    Serial.println(F("║           TimeManager - Serial Sync Ready          ║"));
    Serial.println(F("╠════════════════════════════════════════════════════╣"));
    Serial.printf(  "║  Mode: %-44s ║\n", getModeString().c_str());
    Serial.printf(  "║  Time: %02d:%02d:%02d                                    ║\n", 
                   currentHour, currentMinute, currentSecond);
    Serial.println(F("╠════════════════════════════════════════════════════╣"));
    Serial.println(F("║  Serial Commands:                                  ║"));
    Serial.println(F("║    T12:30:00  - Set time (HH:MM:SS)                ║"));
    Serial.println(F("║    T12:30     - Set time (HH:MM)                   ║"));
    Serial.println(F("║    Tnow       - Sync to PC time (use script)       ║"));
    Serial.println(F("║    E1234567890 - Sync from epoch                   ║"));
    Serial.println(F("║    Mfake     - Switch to fake/fast mode            ║"));
    Serial.println(F("║    Mrtc      - Switch to RTC real-time mode        ║"));
    Serial.println(F("║    S         - Show current status                 ║"));
    Serial.println(F("║    ?         - Show this help                      ║"));
    Serial.println(F("╚════════════════════════════════════════════════════╝"));
    Serial.println(F(""));
}

void TimeManager::printHelp() {
    Serial.println(F("\n=== TimeManager Commands ==="));
    Serial.println(F("T12:30:00  - Set time HH:MM:SS"));
    Serial.println(F("T12:30     - Set time HH:MM"));
    Serial.println(F("E<epoch>   - Sync from Unix epoch"));
    Serial.println(F("Mfake      - Fast time mode"));
    Serial.println(F("Mrtc       - Real time mode"));
    Serial.println(F("S          - Show status"));
    Serial.println(F("?          - This help\n"));
}

void TimeManager::setFakeSpeed(unsigned long ms) {
    updateInterval = ms;
    Serial.printf("[TimeManager] Fake speed: %lu ms/min\n", ms);
}

void TimeManager::processSerialCommand(const String& cmd) {
    if (cmd.length() == 0) return;
    
    char cmdType = cmd.charAt(0);
    String arg = cmd.substring(1);
    arg.trim();
    
    switch (cmdType) {
        case 'T':
        case 't': {
            // Parse time: T12:30:00 or T12:30
            int h, m, s = 0;
            int parsed = sscanf(arg.c_str(), "%d:%d:%d", &h, &m, &s);
            
            if (parsed >= 2) {
                setTime(h, m, s);
                Serial.printf("[TimeManager] ✓ Time synced: %02d:%02d:%02d\n", h, m, s);
            } else {
                Serial.println("[TimeManager] ✗ Invalid format. Use T12:30:00 or T12:30");
            }
            break;
        }
        
        case 'E':
        case 'e': {
            // Parse epoch: E1234567890
            unsigned long epoch = strtoul(arg.c_str(), NULL, 10);
            if (epoch > 0) {
                syncFromEpoch(epoch);
                Serial.printf("[TimeManager] ✓ Synced from epoch: %lu\n", epoch);
            } else {
                Serial.println("[TimeManager] ✗ Invalid epoch");
            }
            break;
        }
        
        case 'M':
        case 'm': {
            // Mode switch: Mfake or Mrtc
            arg.toLowerCase();
            if (arg == "fake" || arg == "f") {
                setMode(TimeMode::FAKE);
                Serial.println("[TimeManager] ✓ Switched to FAKE mode");
            } else if (arg == "rtc" || arg == "r") {
                setMode(TimeMode::RTC);
                Serial.println("[TimeManager] ✓ Switched to RTC mode");
            } else {
                Serial.println("[TimeManager] ✗ Use Mfake or Mrtc");
            }
            break;
        }
        
        case 'S':
        case 's': {
            Serial.println(getFullStatus());
            break;
        }
        
        case '?':
        case 'h':
        case 'H': {
            printHelp();
            break;
        }
        
        default:
            // Comando non riconosciuto, ignora silenziosamente
            // (potrebbe essere per l'EffectManager)
            break;
    }
}

// Parsa un comando stringa (chiamato dal main)
// Ritorna true se era un comando TimeManager
bool TimeManager::parseCommand(const String& cmd) {
    if (cmd.length() == 0) return false;
    
    char first = cmd.charAt(0);
    
    // Controlla se è un comando nostro
    if (first == 'T' || first == 't' ||
        first == 'E' || first == 'e' ||
        first == 'M' || first == 'm' ||
        first == 'S' || first == 's' ||
        first == '?' || first == 'h' || first == 'H') {
        processSerialCommand(cmd);
        return true;
    }
    
    return false;  // Non era un comando TimeManager
}

void TimeManager::readRtcTime() {
    struct tm timeinfo;
    
    if (!getLocalTime(&timeinfo)) {
        return;
    }
    
    currentHour = timeinfo.tm_hour;
    currentMinute = timeinfo.tm_min;
    currentSecond = timeinfo.tm_sec;
}

void TimeManager::updateFakeTime() {
    if (millis() - lastUpdate >= updateInterval) {
        lastUpdate = millis();
        
        currentMinute++;
        
        if (currentMinute >= 60) {
            currentMinute = 0;
            currentHour++;
            
            if (currentHour >= 24) {
                currentHour = 0;
            }
        }
    }
}

void TimeManager::update() {
    // Aggiorna tempo in base alla modalità
    switch (mode) {
        case TimeMode::FAKE:
            updateFakeTime();
            break;
            
        case TimeMode::RTC:
            readRtcTime();
            break;
    }
    
    // Rileva cambiamenti e notifica
    if (currentSecond != lastSecond) {
        lastSecond = currentSecond;
        if (onSecondChange) {
            onSecondChange(currentHour, currentMinute, currentSecond);
        }
    }
    
    if (currentMinute != lastMinute) {
        lastMinute = currentMinute;
        if (onMinuteChange) {
            onMinuteChange(currentHour, currentMinute, currentSecond);
        }
    }
    
    if (currentHour != lastHour) {
        lastHour = currentHour;
        if (onHourChange) {
            onHourChange(currentHour, currentMinute, currentSecond);
        }
    }
}

String TimeManager::getTimeString() const {
    char buffer[9];
    sprintf(buffer, "%02d:%02d:%02d", currentHour, currentMinute, currentSecond);
    return String(buffer);
}

String TimeManager::getFullStatus() const {
    String status = "\n╔══════════════════════════════╗\n";
    status += "║     TimeManager Status       ║\n";
    status += "╠══════════════════════════════╣\n";
    
    char buf[48];
    sprintf(buf, "║  Time: %02d:%02d:%02d              ║\n", 
            currentHour, currentMinute, currentSecond);
    status += buf;
    
    sprintf(buf, "║  Mode: %-20s ║\n", getModeString().c_str());
    status += buf;
    
    if (mode == TimeMode::FAKE) {
        sprintf(buf, "║  Speed: %lu ms/min          ║\n", updateInterval);
        status += buf;
    }
    
    status += "╚══════════════════════════════╝\n";
    return status;
}

void TimeManager::setTime(int hour, int minute, int second) {
    currentHour = hour % 24;
    currentMinute = minute % 60;
    currentSecond = second % 60;
    
    // Imposta l'RTC interno dell'ESP32
    struct tm timeinfo = {0};
    timeinfo.tm_year = 2025 - 1900;  // Anno default
    timeinfo.tm_mon = 0;              // Gennaio
    timeinfo.tm_mday = 1;
    timeinfo.tm_hour = currentHour;
    timeinfo.tm_min = currentMinute;
    timeinfo.tm_sec = currentSecond;
    
    time_t t = mktime(&timeinfo);
    struct timeval now = { .tv_sec = t, .tv_usec = 0 };
    settimeofday(&now, NULL);
}

void TimeManager::setDateTime(int year, int month, int day, int hour, int minute, int second) {
    struct tm timeinfo = {0};
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = second;
    
    time_t t = mktime(&timeinfo);
    struct timeval now = { .tv_sec = t, .tv_usec = 0 };
    settimeofday(&now, NULL);
    
    currentHour = hour;
    currentMinute = minute;
    currentSecond = second;
    
    Serial.printf("[TimeManager] DateTime: %04d/%02d/%02d %02d:%02d:%02d\n",
                 year, month, day, hour, minute, second);
}

void TimeManager::syncFromEpoch(unsigned long epoch) {
    struct timeval now = { .tv_sec = (time_t)epoch, .tv_usec = 0 };
    settimeofday(&now, NULL);
    
    // Aggiorna subito i valori correnti
    readRtcTime();
    
    Serial.printf("[TimeManager] Synced from epoch, time: %02d:%02d:%02d\n",
                 currentHour, currentMinute, currentSecond);
}

void TimeManager::setMode(TimeMode newMode) {
    mode = newMode;
    lastUpdate = millis();
    
    if (newMode == TimeMode::RTC) {
        // Quando passi a RTC, imposta l'ora corrente nell'RTC
        setTime(currentHour, currentMinute, currentSecond);
    }
}

String TimeManager::getModeString() const {
    switch (mode) {
        case TimeMode::FAKE: return "FAKE (accelerated)";
        case TimeMode::RTC:  return "RTC (real-time)";
        default:             return "UNKNOWN";
    }
}