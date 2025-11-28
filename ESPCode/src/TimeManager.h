#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <functional>
#include <time.h>
#include <sys/time.h>
#include <Wire.h>
#include <RTClib.h>

// Callback per notificare cambiamenti di tempo
typedef std::function<void(int hour, int minute, int second)> TimeCallback;

// Modalità di funzionamento
enum class TimeMode {
    FAKE,       // Tempo accelerato per test
    RTC         // RTC interno ESP32 (tempo reale)
};

class TimeManager {
private:
    // DS3231 RTC esterno
    RTC_DS3231 ds3231;
    bool ds3231Available;
    
    // Pin I2C (default ESP32)
    static constexpr int SDA_PIN = 21;
    static constexpr int SCL_PIN = 22;
    
    // Tempo corrente (cache)
    int currentHour;
    int currentMinute;
    int currentSecond;
    
    // Data corrente
    int currentYear;
    int currentMonth;
    int currentDay;
    
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
    
    // DS3231
    bool initDS3231();
    void syncFromDS3231();
    void syncToDS3231();
    
    // Timezone - converte UTC a ora locale (Italia)
    void applyTimezone(struct tm* timeinfo);
    time_t getLocalEpoch(time_t utcEpoch);
    
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
    int getYear() const { return currentYear; }
    int getMonth() const { return currentMonth; }
    int getDay() const { return currentDay; }
    String getTimeString() const;
    String getDateString() const;
    String getFullStatus();
    
    // Setters
    void setTime(int hour, int minute, int second = 0);
    void setDateTime(int year, int month, int day, int hour, int minute, int second = 0);
    
    // Sync da epoch (secondi dal 1970) - si aspetta epoch UTC
    void syncFromEpoch(unsigned long epoch);
    
    // DS3231 status
    bool isDS3231Available() const { return ds3231Available; }
    float getDS3231Temperature();
    
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
