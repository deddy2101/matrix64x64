#ifndef TEXT_SCHEDULE_MANAGER_H
#define TEXT_SCHEDULE_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <vector>
#include "Debug.h"

// Struttura per una scritta programmata
struct ScheduledText {
    uint8_t id;                 // ID univoco (0-255)
    char text[128];             // Testo da visualizzare
    uint16_t color;             // Colore RGB565
    uint8_t hour;               // Ora (0-23)
    uint8_t minute;             // Minuto (0-59)
    uint8_t repeatDays;         // Bitmask giorni: bit0=Lun, bit1=Mar, ..., bit6=Dom, 0xFF=ogni giorno
    uint16_t year;              // Anno specifico (0 = ogni anno)
    uint8_t month;              // Mese specifico (1-12, 0 = ogni mese)
    uint8_t day;                // Giorno specifico (1-31, 0 = ogni giorno)
    uint8_t loopCount;          // Numero di loop (0 = infinito, 1+ = numero di ripetizioni)
    bool enabled;               // Abilitato/disabilitato

    ScheduledText()
        : id(0), color(0xFFE0), hour(0), minute(0),
          repeatDays(0xFF), year(0), month(0), day(0), loopCount(1), enabled(true) {
        text[0] = '\0';
    }
};

class TextScheduleManager {
public:
    TextScheduleManager();

    // Inizializzazione
    void begin();

    // CRUD operations
    uint8_t addScheduledText(const char* text, uint16_t color, uint8_t hour, uint8_t minute,
                            uint8_t repeatDays = 0xFF, uint16_t year = 0, uint8_t month = 0, uint8_t day = 0, uint8_t loopCount = 1);
    bool updateScheduledText(uint8_t id, const char* text, uint16_t color, uint8_t hour, uint8_t minute,
                            uint8_t repeatDays = 0xFF, uint16_t year = 0, uint8_t month = 0, uint8_t day = 0, uint8_t loopCount = 1);
    bool deleteScheduledText(uint8_t id);
    bool enableScheduledText(uint8_t id, bool enabled);

    // Query
    std::vector<ScheduledText> listScheduledTexts() const;
    ScheduledText* getScheduledText(uint8_t id);
    ScheduledText* getActiveScheduledText(uint8_t hour, uint8_t minute, uint16_t year, uint8_t month, uint8_t day, uint8_t weekDay);
    int getCount() const { return _scheduledTexts.size(); }

    // Serialization
    String toCSV() const;

    // Storage
    void save();
    void load();

    // Debug
    void print() const;

private:
    std::vector<ScheduledText> _scheduledTexts;
    Preferences _preferences;
    uint8_t _nextId;

    // Helper
    uint8_t generateId();
    int findIndexById(uint8_t id) const;
    bool matchesSchedule(const ScheduledText& schedule, uint8_t hour, uint8_t minute,
                        uint16_t year, uint8_t month, uint8_t day, uint8_t weekDay) const;
};

#endif // TEXT_SCHEDULE_MANAGER_H
