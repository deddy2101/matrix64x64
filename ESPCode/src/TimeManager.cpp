#include "TimeManager.h"

TimeManager::TimeManager(bool fakeTime, unsigned long fakeSpeedMs)
    : ds3231Available(false),
      currentHour(12),
      currentMinute(0),
      currentSecond(0),
      currentYear(2025),
      currentMonth(1),
      currentDay(1),
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

bool TimeManager::initDS3231() {
    // Inizializza I2C con i pin specificati
    Wire.begin(SDA_PIN, SCL_PIN);
    
    // Prova a inizializzare il DS3231
    if (!ds3231.begin()) {
        Serial.println(F("[TimeManager] ⚠ DS3231 non trovato!"));
        return false;
    }
    
    // Controlla se ha perso l'alimentazione
    if (ds3231.lostPower()) {
        Serial.println(F("[TimeManager] ⚠ DS3231 ha perso alimentazione, necessita sync"));
    }
    
    Serial.println(F("[TimeManager] ✓ DS3231 inizializzato"));
    return true;
}

void TimeManager::syncFromDS3231() {
    if (!ds3231Available) return;
    
    // Legge l'ora UTC dal DS3231
    DateTime now = ds3231.now();
    
    // Il DS3231 memorizza l'ora in UTC, convertiamo a locale
    time_t utcEpoch = now.unixtime();
    time_t localEpoch = getLocalEpoch(utcEpoch);
    
    // Imposta l'RTC interno ESP32 con l'ora locale
    struct timeval tv = { .tv_sec = localEpoch, .tv_usec = 0 };
    settimeofday(&tv, NULL);
    
    // Aggiorna i valori correnti
    struct tm timeinfo;
    localtime_r(&localEpoch, &timeinfo);
    
    currentYear = timeinfo.tm_year + 1900;
    currentMonth = timeinfo.tm_mon + 1;
    currentDay = timeinfo.tm_mday;
    currentHour = timeinfo.tm_hour;
    currentMinute = timeinfo.tm_min;
    currentSecond = timeinfo.tm_sec;
    
    Serial.printf("[TimeManager] ✓ Sincronizzato da DS3231: %04d/%02d/%02d %02d:%02d:%02d\n",
                 currentYear, currentMonth, currentDay,
                 currentHour, currentMinute, currentSecond);
}

void TimeManager::syncToDS3231() {
    if (!ds3231Available) return;
    
    // Converte l'ora locale corrente a UTC per salvare nel DS3231
    struct tm timeinfo = {0};
    timeinfo.tm_year = currentYear - 1900;
    timeinfo.tm_mon = currentMonth - 1;
    timeinfo.tm_mday = currentDay;
    timeinfo.tm_hour = currentHour;
    timeinfo.tm_min = currentMinute;
    timeinfo.tm_sec = currentSecond;
    timeinfo.tm_isdst = -1;  // Lascia che mktime determini DST
    
    // mktime converte da locale a epoch (considera il TZ impostato)
    time_t localEpoch = mktime(&timeinfo);
    
    // Ora dobbiamo convertire da locale a UTC
    // Siccome abbiamo impostato TZ, localtime/mktime gestiscono il fuso
    // Per ottenere UTC, usiamo gmtime sull'epoch
    struct tm utcTime;
    gmtime_r(&localEpoch, &utcTime);
    
    // In realtà è più semplice: l'epoch è sempre UTC-based
    // mktime con TZ impostato restituisce l'epoch corretto
    // Il DS3231 vuole l'epoch UTC
    
    // Salva nel DS3231 (in UTC)
    ds3231.adjust(DateTime(localEpoch));
    
    Serial.printf("[TimeManager] ✓ DS3231 aggiornato (UTC epoch: %lu)\n", 
                 (unsigned long)localEpoch);
}

time_t TimeManager::getLocalEpoch(time_t utcEpoch) {
    // Usa le funzioni di sistema che rispettano TZ
    struct tm timeinfo;
    localtime_r(&utcEpoch, &timeinfo);
    
    // Calcola l'offset applicato
    // localtime_r ha già applicato il fuso orario
    return utcEpoch;  // L'epoch è sempre UTC, è la rappresentazione che cambia
}

void TimeManager::begin(int hour, int minute, int second) {
    // Configura timezone Italia PRIMA di tutto
    // CET-1CEST,M3.5.0,M10.5.0/3 significa:
    // - CET = Central European Time, offset -1 (ovvero UTC+1)
    // - CEST = Central European Summer Time
    // - M3.5.0 = ultima domenica di marzo alle 02:00
    // - M10.5.0/3 = ultima domenica di ottobre alle 03:00
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    
    // Prova a inizializzare il DS3231
    ds3231Available = initDS3231();
    
    if (ds3231Available && mode == TimeMode::RTC) {
        // Se abbiamo il DS3231, carica l'ora da lì
        syncFromDS3231();
    } else {
        // Altrimenti usa l'ora passata come parametro
        currentHour = hour;
        currentMinute = minute;
        currentSecond = second;
        lastUpdate = millis();
        
        if (mode == TimeMode::RTC) {
            setTime(hour, minute, second);
        }
    }
    
    Serial.println(F(""));
    Serial.println(F("╔════════════════════════════════════════════════════╗"));
    Serial.println(F("║           TimeManager - Serial Sync Ready          ║"));
    Serial.println(F("╠════════════════════════════════════════════════════╣"));
    Serial.printf(  "║  Mode: %-44s ║\n", getModeString().c_str());
    Serial.printf(  "║  Time: %02d:%02d:%02d                                    ║\n", 
                   currentHour, currentMinute, currentSecond);
    Serial.printf(  "║  Date: %04d/%02d/%02d                                 ║\n",
                   currentYear, currentMonth, currentDay);
    Serial.printf(  "║  DS3231: %-42s ║\n", 
                   ds3231Available ? "✓ Connected" : "✗ Not found");
    if (ds3231Available) {
        Serial.printf("║  DS3231 Temp: %.1f°C                              ║\n",
                     getDS3231Temperature());
    }
    Serial.println(F("╠════════════════════════════════════════════════════╣"));
    Serial.println(F("║  Serial Commands:                                  ║"));
    Serial.println(F("║    T12:30:00  - Set time (HH:MM:SS)                ║"));
    Serial.println(F("║    T12:30     - Set time (HH:MM)                   ║"));
    Serial.println(F("║    D2025/01/15 12:30:00 - Set full datetime        ║"));
    Serial.println(F("║    E1234567890 - Sync from epoch (UTC)             ║"));
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
    Serial.println(F("D2025/01/15 12:30:00 - Set datetime"));
    Serial.println(F("E<epoch>   - Sync from Unix epoch (UTC)"));
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
        
        case 'D':
        case 'd': {
            // Parse datetime: D2025/01/15 12:30:00
            int y, mo, d, h, m, s = 0;
            int parsed = sscanf(arg.c_str(), "%d/%d/%d %d:%d:%d", &y, &mo, &d, &h, &m, &s);
            
            if (parsed >= 5) {
                setDateTime(y, mo, d, h, m, s);
                Serial.printf("[TimeManager] ✓ DateTime synced: %04d/%02d/%02d %02d:%02d:%02d\n", 
                             y, mo, d, h, m, s);
            } else {
                Serial.println("[TimeManager] ✗ Invalid format. Use D2025/01/15 12:30:00");
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
            break;
    }
}

bool TimeManager::parseCommand(const String& cmd) {
    if (cmd.length() == 0) return false;
    
    char first = cmd.charAt(0);
    
    if (first == 'T' || first == 't' ||
        first == 'D' || first == 'd' ||
        first == 'E' || first == 'e' ||
        first == 'M' || first == 'm' ||
        first == 'S' || first == 's' ||
        first == '?' || first == 'h' || first == 'H') {
        processSerialCommand(cmd);
        return true;
    }
    
    return false;
}

void TimeManager::readRtcTime() {
    struct tm timeinfo;
    
    if (!getLocalTime(&timeinfo)) {
        return;
    }
    
    currentYear = timeinfo.tm_year + 1900;
    currentMonth = timeinfo.tm_mon + 1;
    currentDay = timeinfo.tm_mday;
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
    switch (mode) {
        case TimeMode::FAKE:
            updateFakeTime();
            break;
            
        case TimeMode::RTC:
            readRtcTime();
            break;
    }
    
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

String TimeManager::getDateString() const {
    char buffer[11];
    sprintf(buffer, "%04d/%02d/%02d", currentYear, currentMonth, currentDay);
    return String(buffer);
}

String TimeManager::getFullStatus() {
    String status = "\n╔══════════════════════════════════════╗\n";
    status += "║       TimeManager Status             ║\n";
    status += "╠══════════════════════════════════════╣\n";
    
    char buf[64];
    sprintf(buf, "║  Date: %04d/%02d/%02d                    ║\n", 
            currentYear, currentMonth, currentDay);
    status += buf;
    
    sprintf(buf, "║  Time: %02d:%02d:%02d                       ║\n", 
            currentHour, currentMinute, currentSecond);
    status += buf;
    
    sprintf(buf, "║  Mode: %-28s  ║\n", getModeString().c_str());
    status += buf;
    
    sprintf(buf, "║  DS3231: %-27s  ║\n", 
            ds3231Available ? "✓ Connected" : "✗ Not found");
    status += buf;
    
    if (ds3231Available) {
        sprintf(buf, "║  DS3231 Temp: %.1f°C                   ║\n", 
                getDS3231Temperature());
        status += buf;
    }
    
    if (mode == TimeMode::FAKE) {
        sprintf(buf, "║  Speed: %lu ms/min                   ║\n", updateInterval);
        status += buf;
    }
    
    // Mostra info DST
    struct tm timeinfo;
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    sprintf(buf, "║  DST: %-30s  ║\n", 
            timeinfo.tm_isdst > 0 ? "Active (ora legale)" : "Inactive (ora solare)");
    status += buf;
    
    status += "╚══════════════════════════════════════╝\n";
    return status;
}

void TimeManager::setTime(int hour, int minute, int second) {
    // Usa la data corrente, delega a setDateTime
    setDateTime(currentYear, currentMonth, currentDay, hour, minute, second);
}

void TimeManager::setDateTime(int year, int month, int day, int hour, int minute, int second) {
    currentYear = year;
    currentMonth = month;
    currentDay = day;
    currentHour = hour % 24;
    currentMinute = minute % 60;
    currentSecond = second % 60;
    
    struct tm timeinfo = {0};
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = currentHour;
    timeinfo.tm_min = currentMinute;
    timeinfo.tm_sec = currentSecond;
    timeinfo.tm_isdst = -1;  // Lascia determinare automaticamente
    
    time_t t = mktime(&timeinfo);
    struct timeval now = { .tv_sec = t, .tv_usec = 0 };
    settimeofday(&now, NULL);
    
    // Aggiorna anche il DS3231
    syncToDS3231();
    
    Serial.printf("[TimeManager] DateTime set: %04d/%02d/%02d %02d:%02d:%02d\n",
                 year, month, day, currentHour, currentMinute, currentSecond);
}

void TimeManager::syncFromEpoch(unsigned long epoch) {
    // L'epoch ricevuto è in UTC
    struct timeval now = { .tv_sec = (time_t)epoch, .tv_usec = 0 };
    settimeofday(&now, NULL);
    
    // Aggiorna i valori correnti (legge con timezone applicato)
    readRtcTime();
    
    // Salva nel DS3231 (in UTC)
    if (ds3231Available) {
        ds3231.adjust(DateTime((uint32_t)epoch));
        Serial.println("[TimeManager] ✓ DS3231 synchronized");
    }
    
    Serial.printf("[TimeManager] Synced from epoch, local time: %02d:%02d:%02d\n",
                 currentHour, currentMinute, currentSecond);
}

void TimeManager::setMode(TimeMode newMode) {
    mode = newMode;
    lastUpdate = millis();
    
    if (newMode == TimeMode::RTC) {
        // Quando passi a RTC, sincronizza
        if (ds3231Available) {
            syncFromDS3231();
        } else {
            setTime(currentHour, currentMinute, currentSecond);
        }
    }
}

String TimeManager::getModeString() const {
    switch (mode) {
        case TimeMode::FAKE: return "FAKE (accelerated)";
        case TimeMode::RTC:  return "RTC (real-time)";
        default:             return "UNKNOWN";
    }
}

float TimeManager::getDS3231Temperature() {
    if (!ds3231Available) return 0.0f;
    return ds3231.getTemperature();
}