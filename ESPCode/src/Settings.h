#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include <Preferences.h>
#include "Debug.h"


// Struttura configurazione
struct Config {
    // WiFi
    char ssid[33];
    char password[65];
    bool useAP;  // true = Access Point, false = Station
    
    // Display
    uint8_t brightnessDay;
    uint8_t brightnessNight;
    uint8_t nightStartHour;
    uint8_t nightEndHour;
    
    // Effects
    unsigned long effectDuration;  // ms
    bool autoSwitch;
    int currentEffect;  // -1 = auto, >= 0 = fisso
    
    // Device
    char deviceName[33];  // per mDNS

    // Scroll Text
    char scrollText[128];  // Testo scorrevole configurabile
};

class Settings {
private:
    Preferences preferences;
    Config config;
    bool dirty;  // true se ci sono modifiche non salvate
    
    // Valori di default
    void setDefaults();
    
public:
    Settings();
    
    // Inizializzazione
    void begin();
    
    // Carica/Salva
    void load();
    void save();
    bool isDirty() const { return dirty; }
    
    // ═══════════════════════════════════════════
    // WiFi
    // ═══════════════════════════════════════════
    const char* getSSID() const { return config.ssid; }
    const char* getPassword() const { return config.password; }
    bool isAPMode() const { return config.useAP; }
    
    void setSSID(const char* ssid);
    void setPassword(const char* password);
    void setAPMode(bool useAP);
    
    // ═══════════════════════════════════════════
    // Display
    // ═══════════════════════════════════════════
    uint8_t getBrightnessDay() const { return config.brightnessDay; }
    uint8_t getBrightnessNight() const { return config.brightnessNight; }
    uint8_t getNightStartHour() const { return config.nightStartHour; }
    uint8_t getNightEndHour() const { return config.nightEndHour; }
    
    void setBrightnessDay(uint8_t value);
    void setBrightnessNight(uint8_t value);
    void setNightHours(uint8_t start, uint8_t end);
    
    // Calcola la luminosità corrente in base all'ora
    uint8_t getCurrentBrightness(int currentHour) const;
    bool isNightTime(int currentHour) const;
    
    // ═══════════════════════════════════════════
    // Effects
    // ═══════════════════════════════════════════
    unsigned long getEffectDuration() const { return config.effectDuration; }
    bool isAutoSwitch() const { return config.autoSwitch; }
    int getCurrentEffect() const { return config.currentEffect; }
    
    void setEffectDuration(unsigned long ms);
    void setAutoSwitch(bool enabled);
    void setCurrentEffect(int index);
    
    // ═══════════════════════════════════════════
    // Device
    // ═══════════════════════════════════════════
    const char* getDeviceName() const { return config.deviceName; }
    void setDeviceName(const char* name);

    // ═══════════════════════════════════════════
    // Scroll Text
    // ═══════════════════════════════════════════
    const char* getScrollText() const { return config.scrollText; }
    void setScrollText(const char* text);
    
    // ═══════════════════════════════════════════
    // CSV Serialization (simple format)
    // ═══════════════════════════════════════════
    String toCSV() const;
    
    // Debug
    void print() const;
};

#endif