#include "Settings.h"

Settings::Settings() : dirty(false) {
    setDefaults();
}

void Settings::setDefaults() {
    // WiFi defaults
    strcpy(config.ssid, "");
    strcpy(config.password, "");
    config.useAP = true;  // Default: Access Point
    
    // Display defaults
    config.brightnessDay = 200;
    config.brightnessNight = 30;
    config.nightStartHour = 22;
    config.nightEndHour = 7;
    
    // Effects defaults
    config.effectDuration = 10000;  // 10 secondi
    config.autoSwitch = true;
    config.currentEffect = -1;  // Auto
    
    // Device defaults
    strcpy(config.deviceName, "ledmatrix");

    // Scroll Text defaults
    strcpy(config.scrollText, "PROSSIMA FERMATA FIRENZE");
    config.scrollTextColor = 0xFFE0;  // Giallo (RGB565)

    // NTP/Timezone defaults
    config.ntpEnabled = true;
    strcpy(config.timezone, "CET-1CEST,M3.5.0,M10.5.0/3");  // Italia
}

void Settings::begin() {
    preferences.begin("ledmatrix", false);
    load();
}

void Settings::load() {
    // WiFi
    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");
    strncpy(config.ssid, ssid.c_str(), sizeof(config.ssid) - 1);
    strncpy(config.password, password.c_str(), sizeof(config.password) - 1);
    config.useAP = preferences.getBool("useAP", true);
    
    // Display
    config.brightnessDay = preferences.getUChar("brightDay", 200);
    config.brightnessNight = preferences.getUChar("brightNight", 30);
    config.nightStartHour = preferences.getUChar("nightStart", 22);
    config.nightEndHour = preferences.getUChar("nightEnd", 7);
    
    // Effects
    config.effectDuration = preferences.getULong("effectDur", 10000);
    config.autoSwitch = preferences.getBool("autoSwitch", true);
    config.currentEffect = preferences.getInt("currEffect", -1);
    
    // Device
    String deviceName = preferences.getString("deviceName", "ledmatrix");
    strncpy(config.deviceName, deviceName.c_str(), sizeof(config.deviceName) - 1);

    // Scroll Text
    String scrollText = preferences.getString("scrollText", "PROSSIMA FERMATA FIRENZE");
    strncpy(config.scrollText, scrollText.c_str(), sizeof(config.scrollText) - 1);
    config.scrollTextColor = preferences.getUShort("scrollColor", 0xFFE0);

    // NTP/Timezone
    config.ntpEnabled = preferences.getBool("ntpEnabled", true);
    String timezone = preferences.getString("timezone", "CET-1CEST,M3.5.0,M10.5.0/3");
    strncpy(config.timezone, timezone.c_str(), sizeof(config.timezone) - 1);

    dirty = false;
    
    DEBUG_PRINTLN(F("[Settings] Loaded from NVS"));
    print();
}

void Settings::save() {
    // WiFi
    preferences.putString("ssid", config.ssid);
    preferences.putString("password", config.password);
    preferences.putBool("useAP", config.useAP);
    
    // Display
    preferences.putUChar("brightDay", config.brightnessDay);
    preferences.putUChar("brightNight", config.brightnessNight);
    preferences.putUChar("nightStart", config.nightStartHour);
    preferences.putUChar("nightEnd", config.nightEndHour);
    
    // Effects
    preferences.putULong("effectDur", config.effectDuration);
    preferences.putBool("autoSwitch", config.autoSwitch);
    preferences.putInt("currEffect", config.currentEffect);
    
    // Device
    preferences.putString("deviceName", config.deviceName);

    // Scroll Text
    preferences.putString("scrollText", config.scrollText);
    preferences.putUShort("scrollColor", config.scrollTextColor);

    // NTP/Timezone
    preferences.putBool("ntpEnabled", config.ntpEnabled);
    preferences.putString("timezone", config.timezone);

    dirty = false;

    DEBUG_PRINTLN(F("[Settings] Saved to NVS"));
}

// ═══════════════════════════════════════════
// WiFi Setters
// ═══════════════════════════════════════════

void Settings::setSSID(const char* ssid) {
    strncpy(config.ssid, ssid, sizeof(config.ssid) - 1);
    config.ssid[sizeof(config.ssid) - 1] = '\0';
    dirty = true;
}

void Settings::setPassword(const char* password) {
    strncpy(config.password, password, sizeof(config.password) - 1);
    config.password[sizeof(config.password) - 1] = '\0';
    dirty = true;
}

void Settings::setAPMode(bool useAP) {
    config.useAP = useAP;
    dirty = true;
}

// ═══════════════════════════════════════════
// Display Setters
// ═══════════════════════════════════════════

void Settings::setBrightnessDay(uint8_t value) {
    config.brightnessDay = value;
    dirty = true;
}

void Settings::setBrightnessNight(uint8_t value) {
    config.brightnessNight = value;
    dirty = true;
}

void Settings::setNightHours(uint8_t start, uint8_t end) {
    config.nightStartHour = start % 24;
    config.nightEndHour = end % 24;
    dirty = true;
}

bool Settings::isNightTime(int currentHour) const {
    if (config.nightStartHour > config.nightEndHour) {
        // Notte passa per mezzanotte (es. 22-7)
        return currentHour >= config.nightStartHour || currentHour < config.nightEndHour;
    } else {
        // Notte nello stesso giorno (es. 1-5)
        return currentHour >= config.nightStartHour && currentHour < config.nightEndHour;
    }
}

uint8_t Settings::getCurrentBrightness(int currentHour) const {
    return isNightTime(currentHour) ? config.brightnessNight : config.brightnessDay;
}

// ═══════════════════════════════════════════
// Effects Setters
// ═══════════════════════════════════════════

void Settings::setEffectDuration(unsigned long ms) {
    config.effectDuration = ms;
    dirty = true;
}

void Settings::setAutoSwitch(bool enabled) {
    config.autoSwitch = enabled;
    dirty = true;
}

void Settings::setCurrentEffect(int index) {
    config.currentEffect = index;
    dirty = true;
}

// ═══════════════════════════════════════════
// Device Setters
// ═══════════════════════════════════════════

void Settings::setDeviceName(const char* name) {
    strncpy(config.deviceName, name, sizeof(config.deviceName) - 1);
    config.deviceName[sizeof(config.deviceName) - 1] = '\0';
    dirty = true;
}

// ═══════════════════════════════════════════
// Scroll Text Setters
// ═══════════════════════════════════════════

void Settings::setScrollText(const char* text) {
    strncpy(config.scrollText, text, sizeof(config.scrollText) - 1);
    config.scrollText[sizeof(config.scrollText) - 1] = '\0';
    dirty = true;
}

void Settings::setScrollTextColor(uint16_t color) {
    config.scrollTextColor = color;
    dirty = true;
}

// ═══════════════════════════════════════════
// NTP/Timezone Setters
// ═══════════════════════════════════════════

void Settings::setNTPEnabled(bool enabled) {
    config.ntpEnabled = enabled;
    dirty = true;
}

void Settings::setTimezone(const char* tz) {
    strncpy(config.timezone, tz, sizeof(config.timezone) - 1);
    config.timezone[sizeof(config.timezone) - 1] = '\0';
    dirty = true;
}

// ═══════════════════════════════════════════
// CSV Serialization
// ═══════════════════════════════════════════

String Settings::toCSV() const {
    String csv = "SETTINGS";
    csv += "," + String(config.ssid);
    csv += "," + String(config.useAP ? "1" : "0");
    csv += "," + String(config.brightnessDay);
    csv += "," + String(config.brightnessNight);
    csv += "," + String(config.nightStartHour);
    csv += "," + String(config.nightEndHour);
    csv += "," + String(config.effectDuration);
    csv += "," + String(config.autoSwitch ? "1" : "0");
    csv += "," + String(config.currentEffect);
    csv += "," + String(config.deviceName);
    csv += "," + String(config.scrollText);
    csv += "," + String(config.scrollTextColor);
    csv += "," + String(config.ntpEnabled ? "1" : "0");
    csv += "," + String(config.timezone);
    return csv;
}

void Settings::print() const {
    DEBUG_PRINTLN(F("╔═════════════════════════════════════╗"));
    DEBUG_PRINTLN(F("║        Current Settings             ║"));
    DEBUG_PRINTLN(F("╠═════════════════════════════════════╣"));
    DEBUG_PRINTF("║  WiFi SSID: %-24s║\n", config.ssid[0] ? config.ssid : "(not set)");
    DEBUG_PRINTF("║  WiFi Mode: %-24s║\n", config.useAP ? "Access Point" : "Station");
    DEBUG_PRINTF("║  Brightness Day: %-19d║\n", config.brightnessDay);
    DEBUG_PRINTF("║  Brightness Night: %-17d║\n", config.brightnessNight);
    DEBUG_PRINTF("║  Night Hours: %02d:00 - %02d:00        ║\n", 
                 config.nightStartHour, config.nightEndHour);
    DEBUG_PRINTF("║  Effect Duration: %-14lu ms║\n", config.effectDuration);
    DEBUG_PRINTF("║ Current Effect: %-16s║\n", config.currentEffect >= 0 ? String(config.currentEffect).c_str() : "Auto");
    DEBUG_PRINTF("║  Auto Switch: %-22s║\n", config.autoSwitch ? "ON" : "OFF");
    DEBUG_PRINTF("║  Device Name: %-22s║\n", config.deviceName);
    DEBUG_PRINTF("║  Scroll Text: %-22s║\n", config.scrollText[0] ? config.scrollText : "(not set)");
    DEBUG_PRINTF("║  NTP Enabled: %-22s║\n", config.ntpEnabled ? "ON" : "OFF");
    DEBUG_PRINTF("║  Timezone: %-25s║\n", config.timezone);
    DEBUG_PRINTLN(F("╚═════════════════════════════════════╝"));
}