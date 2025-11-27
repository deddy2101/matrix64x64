#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <functional>
#include <time.h>
#include <sys/time.h>

// Callback per notificare cambiamenti di tempo
typedef std::function<void(int hour, int minute, int second)> TimeCallback;

// Modalità di funzionamento
enum class TimeMode {
    FAKE,       // Tempo accelerato per test
    RTC         // RTC interno ESP32 (tempo reale)
};

class TimeManager {
private:
    // Tempo corrente (cache)
    int currentHour;
    int currentMinute;
    int currentSecond;
    
    // Tempo precedente (per rilevare cambiamenti)
    int lastHour;
    int lastMinute;
    int lastSecond;
    
    // Timer per aggiornamento
    unsigned long lastUpdate;
    unsigned long updateInterval;      // ms tra un minuto e l'altro (in fake mode)
    
    // Callbacks per notifiche
    TimeCallback onSecondChange;
    TimeCallback onMinuteChange;
    TimeCallback onHourChange;
    
    // Modalità
    TimeMode mode;
    
    // Metodi privati
    void readRtcTime();
    void updateFakeTime();
    void processSerialCommand(const String& cmd);
    
public:
    TimeManager(bool fakeTime = true, unsigned long fakeSpeedMs = 5000);
    
    // Setup
    void begin(int hour = 12, int minute = 0, int second = 0);
    
    // Configurazione fake mode
    void setFakeSpeed(unsigned long ms);
    
    // Update (chiamare nel loop)
    void update();
    
    // Parsa comando stringa, ritorna true se era un comando TimeManager
    bool parseCommand(const String& cmd);
    
    // Stampa help comandi
    void printHelp();
    
    // Getters
    int getHour() const { return currentHour; }
    int getMinute() const { return currentMinute; }
    int getSecond() const { return currentSecond; }
    String getTimeString() const;
    String getFullStatus() const;
    
    // Setters
    void setTime(int hour, int minute, int second = 0);
    void setDateTime(int year, int month, int day, int hour, int minute, int second = 0);
    
    // Sync da epoch (secondi dal 1970)
    void syncFromEpoch(unsigned long epoch);
    
    // Callbacks (pattern Observer)
    void setOnSecondChange(TimeCallback callback) { onSecondChange = callback; }
    void setOnMinuteChange(TimeCallback callback) { onMinuteChange = callback; }
    void setOnHourChange(TimeCallback callback) { onHourChange = callback; }
    
    // Modalità
    void setMode(TimeMode newMode);
    void setFakeTimeMode(bool fake) { setMode(fake ? TimeMode::FAKE : TimeMode::RTC); }
    TimeMode getMode() const { return mode; }
    String getModeString() const;
    bool isFakeTime() const { return mode == TimeMode::FAKE; }
};

#endif