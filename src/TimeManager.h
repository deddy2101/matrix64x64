#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <functional>

// Callback per notificare cambiamenti di tempo
typedef std::function<void(int hour, int minute, int second)> TimeCallback;

class TimeManager {
private:
    // Tempo corrente
    int currentHour;
    int currentMinute;
    int currentSecond;
    
    // Timer per aggiornamento
    unsigned long lastUpdate;
    unsigned long updateInterval;  // ms tra un minuto e l'altro (in fake mode)
    
    // Callbacks per notifiche
    TimeCallback onSecondChange;
    TimeCallback onMinuteChange;
    TimeCallback onHourChange;
    
    // Modalità
    bool useFakeTime;  // true = mock time, false = RTC reale
    
public:
    TimeManager(bool fakeTime = true, unsigned long fakeSpeedMs = 5000);
    
    // Setup
    void begin(int hour = 12, int minute = 0, int second = 0);
    void setFakeSpeed(unsigned long ms);  // Velocità tempo fake (ms per minuto)
    
    // Update (chiamare nel loop)
    void update();
    
    // Getters
    int getHour() const { return currentHour; }
    int getMinute() const { return currentMinute; }
    int getSecond() const { return currentSecond; }
    String getTimeString() const;
    
    // Setters (per debugging o set manuale)
    void setTime(int hour, int minute, int second = 0);
    
    // Callbacks (pattern Observer)
    void setOnSecondChange(TimeCallback callback) { onSecondChange = callback; }
    void setOnMinuteChange(TimeCallback callback) { onMinuteChange = callback; }
    void setOnHourChange(TimeCallback callback) { onHourChange = callback; }
    
    // Modalità
    void setFakeTimeMode(bool fake) { useFakeTime = fake; }
    bool isFakeTime() const { return useFakeTime; }
};

#endif
