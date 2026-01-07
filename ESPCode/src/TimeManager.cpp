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
      ntpEnabled(true),
      ntpSynced(false),
      lastNtpSync(0),
      ntpSyncInterval(3600000),  // 1 ora in ms
      mode(fakeTime ? TimeMode::FAKE : TimeMode::RTC) {
    // I vettori si inizializzano automaticamente vuoti
}

bool TimeManager::initDS3231() {
    // Inizializza I2C con i pin specificati
    Wire.begin(SDA_PIN, SCL_PIN);
    
    // Prova a inizializzare il DS3231
    if (!ds3231.begin()) {
        DEBUG_PRINTLN(F("[TimeManager] ⚠ DS3231 non trovato!"));
        return false;
    }
    
    // Controlla se ha perso l'alimentazione
    if (ds3231.lostPower()) {
        DEBUG_PRINTLN(F("[TimeManager] ⚠ DS3231 ha perso alimentazione, necessita sync"));
    }
    
    DEBUG_PRINTLN(F("[TimeManager] ✓ DS3231 inizializzato"));
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
    
    DEBUG_PRINTF("[TimeManager] ✓ Sincronizzato da DS3231: %04d/%02d/%02d %02d:%02d:%02d\n",
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
    
    // Salva nel DS3231 (in UTC)
    ds3231.adjust(DateTime(localEpoch));
    
    DEBUG_PRINTF("[TimeManager] ✓ DS3231 aggiornato (UTC epoch: %lu)\n", 
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

// ═══════════════════════════════════════════
// NTP Sync
// ═══════════════════════════════════════════

bool TimeManager::syncFromNTP() {
    if (!ntpEnabled) {
        DEBUG_PRINTLN(F("[TimeManager] NTP disabled"));
        return false;
    }

    // Verifica connessione WiFi
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN(F("[TimeManager] NTP sync failed: WiFi not connected"));
        return false;
    }

    DEBUG_PRINTLN(F("[TimeManager] Starting NTP sync..."));

    // Configura NTP con timezone Italia
    configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);

    // Attendi sincronizzazione (max 10 secondi)
    struct tm timeinfo;
    int retries = 0;
    while (!getLocalTime(&timeinfo, 1000) && retries < 10) {
        DEBUG_PRINT(F("."));
        retries++;
    }
    DEBUG_PRINTLN();

    if (retries >= 10) {
        DEBUG_PRINTLN(F("[TimeManager] NTP sync timeout"));
        return false;
    }

    // Aggiorna valori correnti
    currentYear = timeinfo.tm_year + 1900;
    currentMonth = timeinfo.tm_mon + 1;
    currentDay = timeinfo.tm_mday;
    currentHour = timeinfo.tm_hour;
    currentMinute = timeinfo.tm_min;
    currentSecond = timeinfo.tm_sec;

    // Aggiorna anche il DS3231 se disponibile
    syncToDS3231();

    ntpSynced = true;
    lastNtpSync = millis();

    DEBUG_PRINTF("[TimeManager] NTP sync OK: %04d/%02d/%02d %02d:%02d:%02d\n",
                 currentYear, currentMonth, currentDay,
                 currentHour, currentMinute, currentSecond);

    return true;
}

void TimeManager::checkNtpSync() {
    if (!ntpEnabled || mode == TimeMode::FAKE) return;

    // Prima sync all'avvio (se WiFi connesso e non ancora sincronizzato)
    if (!ntpSynced && WiFi.status() == WL_CONNECTED) {
        syncFromNTP();
        return;
    }

    // Sync periodico (ogni ntpSyncInterval ms, default 1 ora)
    if (ntpSynced && (millis() - lastNtpSync >= ntpSyncInterval)) {
        if (WiFi.status() == WL_CONNECTED) {
            DEBUG_PRINTLN(F("[TimeManager] Periodic NTP sync..."));
            syncFromNTP();
        }
    }
}

void TimeManager::forceNTPSync() {
    DEBUG_PRINTLN(F("[TimeManager] Forced NTP sync requested"));
    ntpSynced = false;  // Reset per forzare nuovo sync
    syncFromNTP();
}

void TimeManager::setTimezone(const char* tz) {
    if (tz && strlen(tz) > 0) {
        setenv("TZ", tz, 1);
        tzset();
        DEBUG_PRINTF("[TimeManager] Timezone set to: %s\n", tz);
    }
}

void TimeManager::begin(int hour, int minute, int second) {
    // Configura timezone Italia PRIMA di tutto (default, verrà sovrascritto se configurato)
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    
    // Prova a inizializzare il DS3231
    ds3231Available = initDS3231();
    
    if (ds3231Available && mode == TimeMode::RTC) {
        // Se abbiamo il DS3231, carica l'ora da lì
        syncFromDS3231();
        // ✅ Salva lastHour/minute/second per evitare callback inutili all'avvio
        lastHour = currentHour;
        lastMinute = currentMinute;
        lastSecond = currentSecond;
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
    
    DEBUG_PRINTLN(F(""));
    DEBUG_PRINTLN(F("╔════════════════════════════════════════════════════╗"));
    DEBUG_PRINTLN(F("║           TimeManager - Serial Sync Ready          ║"));
    DEBUG_PRINTLN(F("╠════════════════════════════════════════════════════╣"));
    DEBUG_PRINTF(  "║  Mode: %-44s ║\n", getModeString().c_str());
    DEBUG_PRINTF(  "║  Time: %02d:%02d:%02d                                    ║\n", 
                   currentHour, currentMinute, currentSecond);
    DEBUG_PRINTF(  "║  Date: %04d/%02d/%02d                                 ║\n",
                   currentYear, currentMonth, currentDay);
    DEBUG_PRINTF(  "║  DS3231: %-42s ║\n", 
                   ds3231Available ? "✓ Connected" : "✗ Not found");
    if (ds3231Available) {
        DEBUG_PRINTF("║  DS3231 Temp: %.1f°C                              ║\n",
                     getDS3231Temperature());
    }
    DEBUG_PRINTLN(F("╠════════════════════════════════════════════════════╣"));
    DEBUG_PRINTLN(F("║  Serial Commands:                                  ║"));
    DEBUG_PRINTLN(F("║    T12:30:00  - Set time (HH:MM:SS)                ║"));
    DEBUG_PRINTLN(F("║    T12:30     - Set time (HH:MM)                   ║"));
    DEBUG_PRINTLN(F("║    D2025/01/15 12:30:00 - Set full datetime        ║"));
    DEBUG_PRINTLN(F("║    E1234567890 - Sync from epoch (UTC)             ║"));
    DEBUG_PRINTLN(F("║    Mfake     - Switch to fake/fast mode            ║"));
    DEBUG_PRINTLN(F("║    Mrtc      - Switch to RTC real-time mode        ║"));
    DEBUG_PRINTLN(F("║    S         - Show current status                 ║"));
    DEBUG_PRINTLN(F("║    ?         - Show this help                      ║"));
    DEBUG_PRINTLN(F("╚════════════════════════════════════════════════════╝"));
    DEBUG_PRINTLN(F(""));
}

void TimeManager::printHelp() {
    DEBUG_PRINTLN(F("\n=== TimeManager Commands ==="));
    DEBUG_PRINTLN(F("T12:30:00  - Set time HH:MM:SS"));
    DEBUG_PRINTLN(F("T12:30     - Set time HH:MM"));
    DEBUG_PRINTLN(F("D2025/01/15 12:30:00 - Set datetime"));
    DEBUG_PRINTLN(F("E<epoch>   - Sync from Unix epoch (UTC)"));
    DEBUG_PRINTLN(F("Mfake      - Fast time mode"));
    DEBUG_PRINTLN(F("Mrtc       - Real time mode"));
    DEBUG_PRINTLN(F("S          - Show status"));
    DEBUG_PRINTLN(F("?          - This help\n"));
}

void TimeManager::setFakeSpeed(unsigned long ms) {
    updateInterval = ms;
    DEBUG_PRINTF("[TimeManager] Fake speed: %lu ms/min\n", ms);
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
                DEBUG_PRINTF("[TimeManager] ✓ Time synced: %02d:%02d:%02d\n", h, m, s);
            } else {
                DEBUG_PRINTLN("[TimeManager] ✗ Invalid format. Use T12:30:00 or T12:30");
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
                DEBUG_PRINTF("[TimeManager] ✓ DateTime synced: %04d/%02d/%02d %02d:%02d:%02d\n", 
                             y, mo, d, h, m, s);
            } else {
                DEBUG_PRINTLN("[TimeManager] ✗ Invalid format. Use D2025/01/15 12:30:00");
            }
            break;
        }
        
        case 'E':
        case 'e': {
            // Parse epoch: E1234567890
            unsigned long epoch = strtoul(arg.c_str(), NULL, 10);
            if (epoch > 0) {
                syncFromEpoch(epoch);
                DEBUG_PRINTF("[TimeManager] ✓ Synced from epoch: %lu\n", epoch);
            } else {
                DEBUG_PRINTLN("[TimeManager] ✗ Invalid epoch");
            }
            break;
        }
        
        case 'M':
        case 'm': {
            // Mode switch: Mfake or Mrtc
            arg.toLowerCase();
            if (arg == "fake" || arg == "f") {
                setMode(TimeMode::FAKE);
                DEBUG_PRINTLN("[TimeManager] ✓ Switched to FAKE mode");
            } else if (arg == "rtc" || arg == "r") {
                setMode(TimeMode::RTC);
                DEBUG_PRINTLN("[TimeManager] ✓ Switched to RTC mode");
            } else {
                DEBUG_PRINTLN("[TimeManager] ✗ Use Mfake or Mrtc");
            }
            break;
        }
        
        case 'S':
        case 's': {
            DEBUG_PRINTLN(getFullStatus());
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
        DEBUG_PRINTLN("[TimeManager] ✗ Failed to obtain time");
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

// ✅ METODO UPDATE AGGIORNATO PER CHIAMARE TUTTI I CALLBACK
void TimeManager::update() {
    // Check NTP sync (all'avvio e ogni ora)
    checkNtpSync();

    switch (mode) {
        case TimeMode::FAKE:
            updateFakeTime();
            break;

        case TimeMode::RTC:
            readRtcTime();
            break;
    }
    
    // Callback per secondi
    if (currentSecond != lastSecond) {
        lastSecond = currentSecond;
        for (auto& callback : onSecondChangeCallbacks) {
            if (callback) callback(currentHour, currentMinute, currentSecond);
        }
    }
    
    // Callback per minuti
    if (currentMinute != lastMinute) {
        lastMinute = currentMinute;
        
        DEBUG_PRINTF("[TimeManager] ⚡ Minute changed: %02d:%02d -> calling %d callbacks\n",
                     currentHour, currentMinute, onMinuteChangeCallbacks.size());
        
        for (auto& callback : onMinuteChangeCallbacks) {
            if (callback) callback(currentHour, currentMinute, currentSecond);
        }
    }
    
    // Callback per ore
    if (currentHour != lastHour) {
        lastHour = currentHour;
        for (auto& callback : onHourChangeCallbacks) {
            if (callback) callback(currentHour, currentMinute, currentSecond);
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

    // NTP status
    sprintf(buf, "║  NTP: %-30s  ║\n",
            !ntpEnabled ? "Disabled" :
            !ntpSynced ? "Not synced" :
            "✓ Synced");
    status += buf;

    if (ntpSynced) {
        unsigned long sinceSyncSec = (millis() - lastNtpSync) / 1000;
        unsigned long mins = sinceSyncSec / 60;
        sprintf(buf, "║  Last NTP sync: %lu min ago            ║\n", mins);
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
    
    // Mostra numero callbacks registrati
    sprintf(buf, "║  Callbacks: %d second, %d minute, %d hour  ║\n",
            onSecondChangeCallbacks.size(),
            onMinuteChangeCallbacks.size(),
            onHourChangeCallbacks.size());
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
    
    DEBUG_PRINTF("[TimeManager] DateTime set: %04d/%02d/%02d %02d:%02d:%02d\n",
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
        DEBUG_PRINTLN("[TimeManager] ✓ DS3231 synchronized");
    }
    
    DEBUG_PRINTF("[TimeManager] Synced from epoch, local time: %02d:%02d:%02d\n",
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