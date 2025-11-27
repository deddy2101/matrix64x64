#include "TimeManager.h"

TimeManager::TimeManager(bool fakeTime, unsigned long fakeSpeedMs)
    : currentHour(12),
      currentMinute(0),
      currentSecond(0),
      lastUpdate(0),
      updateInterval(fakeSpeedMs),
      useFakeTime(fakeTime),
      onSecondChange(nullptr),
      onMinuteChange(nullptr),
      onHourChange(nullptr) {
}

void TimeManager::begin(int hour, int minute, int second) {
    currentHour = hour;
    currentMinute = minute;
    currentSecond = second;
    lastUpdate = millis();
    
    Serial.printf("[TimeManager] Initialized: %02d:%02d:%02d (Fake: %s, Speed: %lu ms/min)\n",
                 currentHour, currentMinute, currentSecond,
                 useFakeTime ? "YES" : "NO", updateInterval);
}

void TimeManager::setFakeSpeed(unsigned long ms) {
    updateInterval = ms;
    Serial.printf("[TimeManager] Fake speed set to %lu ms per minute\n", ms);
}

void TimeManager::update() {
    if (useFakeTime) {
        // Modalità FAKE - avanza velocemente per test
        if (millis() - lastUpdate >= updateInterval) {
            lastUpdate = millis();
            
            int oldHour = currentHour;
            int oldMinute = currentMinute;
            
            // Incrementa minuti direttamente in fake mode
            currentMinute++;
            
            if (currentMinute >= 60) {
                currentMinute = 0;
                currentHour++;
                
                if (currentHour >= 24) {
                    currentHour = 0;
                }
                
                // Notifica cambio ora
                Serial.printf("[TimeManager] Hour changed: %02d:00\n", currentHour);
                if (onHourChange) {
                    onHourChange(currentHour, currentMinute, currentSecond);
                }
            }
            
            // Notifica cambio minuto
            Serial.printf("[TimeManager] Time: %02d:%02d\n", currentHour, currentMinute);
            if (onMinuteChange) {
                onMinuteChange(currentHour, currentMinute, currentSecond);
            }
        }
    } else {
        // Modalità RTC REALE - qui integrerai il tuo RTC
        // TODO: Leggi da RTC hardware
        // esempio:
        // DateTime now = rtc.now();
        // int newHour = now.hour();
        // int newMinute = now.minute();
        // int newSecond = now.second();
        //
        // if (newSecond != currentSecond) {
        //     currentSecond = newSecond;
        //     if (onSecondChange) onSecondChange(currentHour, currentMinute, currentSecond);
        // }
        // if (newMinute != currentMinute) {
        //     currentMinute = newMinute;
        //     if (onMinuteChange) onMinuteChange(currentHour, currentMinute, currentSecond);
        // }
        // if (newHour != currentHour) {
        //     currentHour = newHour;
        //     if (onHourChange) onHourChange(currentHour, currentMinute, currentSecond);
        // }
    }
}

String TimeManager::getTimeString() const {
    char buffer[9];
    sprintf(buffer, "%02d:%02d:%02d", currentHour, currentMinute, currentSecond);
    return String(buffer);
}

void TimeManager::setTime(int hour, int minute, int second) {
    currentHour = hour % 24;
    currentMinute = minute % 60;
    currentSecond = second % 60;
    
    Serial.printf("[TimeManager] Time set to: %02d:%02d:%02d\n", 
                 currentHour, currentMinute, currentSecond);
}
