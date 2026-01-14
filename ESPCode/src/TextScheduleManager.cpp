#include "TextScheduleManager.h"

TextScheduleManager::TextScheduleManager() : _nextId(1) {
}

void TextScheduleManager::begin() {
    _preferences.begin("schedtexts", false);
    load();
}

uint8_t TextScheduleManager::generateId() {
    // Trova il primo ID non usato
    while (_nextId < 255) {
        bool used = false;
        for (const auto& st : _scheduledTexts) {
            if (st.id == _nextId) {
                used = true;
                break;
            }
        }
        if (!used) {
            return _nextId++;
        }
        _nextId++;
    }
    return 0;  // Errore: nessun ID disponibile
}

int TextScheduleManager::findIndexById(uint8_t id) const {
    for (size_t i = 0; i < _scheduledTexts.size(); i++) {
        if (_scheduledTexts[i].id == id) {
            return i;
        }
    }
    return -1;
}

uint8_t TextScheduleManager::addScheduledText(const char* text, uint16_t color, uint8_t hour, uint8_t minute,
                                              uint8_t repeatDays, uint16_t year, uint8_t month, uint8_t day, uint8_t loopCount) {
    if (_scheduledTexts.size() >= 50) {  // Limite massimo
        DEBUG_PRINTLN(F("[TextSchedule] Maximum number of scheduled texts reached"));
        return 0;
    }

    ScheduledText schedule;
    schedule.id = generateId();
    if (schedule.id == 0) {
        return 0;  // Nessun ID disponibile
    }

    strncpy(schedule.text, text, sizeof(schedule.text) - 1);
    schedule.text[sizeof(schedule.text) - 1] = '\0';
    schedule.color = color;
    schedule.hour = hour % 24;
    schedule.minute = minute % 60;
    schedule.repeatDays = repeatDays;
    schedule.year = year;
    schedule.month = month;
    schedule.day = day;
    schedule.loopCount = loopCount;
    schedule.enabled = true;

    _scheduledTexts.push_back(schedule);
    save();

    DEBUG_PRINTF("[TextSchedule] Added schedule ID %d: %s at %02d:%02d (loops: %d)\n",
                 schedule.id, schedule.text, schedule.hour, schedule.minute, schedule.loopCount);

    return schedule.id;
}

bool TextScheduleManager::updateScheduledText(uint8_t id, const char* text, uint16_t color, uint8_t hour, uint8_t minute,
                                              uint8_t repeatDays, uint16_t year, uint8_t month, uint8_t day, uint8_t loopCount) {
    int index = findIndexById(id);
    if (index < 0) {
        return false;
    }

    ScheduledText& schedule = _scheduledTexts[index];
    DEBUG_PRINTF("[TextSchedule] Updating ID %d: loopCount %d -> %d\n", id, schedule.loopCount, loopCount);

    strncpy(schedule.text, text, sizeof(schedule.text) - 1);
    schedule.text[sizeof(schedule.text) - 1] = '\0';
    schedule.color = color;
    schedule.hour = hour % 24;
    schedule.minute = minute % 60;
    schedule.repeatDays = repeatDays;
    schedule.year = year;
    schedule.month = month;
    schedule.day = day;
    schedule.loopCount = loopCount;

    save();
    DEBUG_PRINTF("[TextSchedule] Updated schedule ID %d (new loopCount: %d)\n", id, schedule.loopCount);
    return true;
}

bool TextScheduleManager::deleteScheduledText(uint8_t id) {
    int index = findIndexById(id);
    if (index < 0) {
        return false;
    }

    _scheduledTexts.erase(_scheduledTexts.begin() + index);
    save();
    DEBUG_PRINTF("[TextSchedule] Deleted schedule ID %d\n", id);
    return true;
}

bool TextScheduleManager::enableScheduledText(uint8_t id, bool enabled) {
    int index = findIndexById(id);
    if (index < 0) {
        return false;
    }

    _scheduledTexts[index].enabled = enabled;
    save();
    DEBUG_PRINTF("[TextSchedule] Schedule ID %d %s\n", id, enabled ? "enabled" : "disabled");
    return true;
}

std::vector<ScheduledText> TextScheduleManager::listScheduledTexts() const {
    return _scheduledTexts;
}

ScheduledText* TextScheduleManager::getScheduledText(uint8_t id) {
    int index = findIndexById(id);
    if (index < 0) {
        return nullptr;
    }
    return &_scheduledTexts[index];
}

bool TextScheduleManager::matchesSchedule(const ScheduledText& schedule, uint8_t hour, uint8_t minute,
                                          uint16_t year, uint8_t month, uint8_t day, uint8_t weekDay) const {
    if (!schedule.enabled) {
        return false;
    }

    // Check hour and minute
    if (schedule.hour != hour || schedule.minute != minute) {
        return false;
    }

    // Check year (0 = every year)
    if (schedule.year != 0 && schedule.year != year) {
        return false;
    }

    // Check month (0 = every month)
    if (schedule.month != 0 && schedule.month != month) {
        return false;
    }

    // Check day (0 = every day)
    if (schedule.day != 0 && schedule.day != day) {
        return false;
    }

    // Check week day (0xFF = every day, otherwise bitmask)
    if (schedule.repeatDays != 0xFF) {
        // weekDay: 0=Sunday, 1=Monday, ..., 6=Saturday
        // repeatDays: bit0=Monday, bit1=Tuesday, ..., bit6=Sunday
        uint8_t dayBit;
        if (weekDay == 0) {
            dayBit = 6;  // Sunday is bit 6
        } else {
            dayBit = weekDay - 1;  // Monday=0, Tuesday=1, etc.
        }

        if (!(schedule.repeatDays & (1 << dayBit))) {
            return false;
        }
    }

    return true;
}

ScheduledText* TextScheduleManager::getActiveScheduledText(uint8_t hour, uint8_t minute,
                                                           uint16_t year, uint8_t month, uint8_t day, uint8_t weekDay) {
    // Trova la prima scritta che corrisponde all'orario attuale
    for (auto& schedule : _scheduledTexts) {
        if (matchesSchedule(schedule, hour, minute, year, month, day, weekDay)) {
            return &schedule;
        }
    }
    return nullptr;
}

String TextScheduleManager::toCSV() const {
    String csv = "SCHEDULED_TEXTS," + String(_scheduledTexts.size());

    for (const auto& schedule : _scheduledTexts) {
        csv += "," + String(schedule.id);
        csv += "," + String(schedule.text);
        csv += "," + String(schedule.color);
        csv += "," + String(schedule.hour);
        csv += "," + String(schedule.minute);
        csv += "," + String(schedule.repeatDays);
        csv += "," + String(schedule.year);
        csv += "," + String(schedule.month);
        csv += "," + String(schedule.day);
        csv += "," + String(schedule.loopCount);
        csv += "," + String(schedule.enabled ? "1" : "0");
    }

    return csv;
}

void TextScheduleManager::save() {
    // Save count
    _preferences.putUChar("count", _scheduledTexts.size());

    // Save each scheduled text
    for (size_t i = 0; i < _scheduledTexts.size(); i++) {
        String prefix = "st" + String(i) + "_";
        const ScheduledText& st = _scheduledTexts[i];

        _preferences.putUChar((prefix + "id").c_str(), st.id);
        _preferences.putString((prefix + "txt").c_str(), st.text);
        _preferences.putUShort((prefix + "col").c_str(), st.color);
        _preferences.putUChar((prefix + "h").c_str(), st.hour);
        _preferences.putUChar((prefix + "m").c_str(), st.minute);
        _preferences.putUChar((prefix + "rep").c_str(), st.repeatDays);
        _preferences.putUShort((prefix + "y").c_str(), st.year);
        _preferences.putUChar((prefix + "mon").c_str(), st.month);
        _preferences.putUChar((prefix + "d").c_str(), st.day);
        _preferences.putUChar((prefix + "loop").c_str(), st.loopCount);
        _preferences.putBool((prefix + "en").c_str(), st.enabled);
    }

    DEBUG_PRINTF("[TextSchedule] Saved %d scheduled texts\n", _scheduledTexts.size());
}

void TextScheduleManager::load() {
    _scheduledTexts.clear();
    _nextId = 1;

    uint8_t count = _preferences.getUChar("count", 0);
    DEBUG_PRINTF("[TextSchedule] Loading %d scheduled texts\n", count);

    for (uint8_t i = 0; i < count; i++) {
        String prefix = "st" + String(i) + "_";
        ScheduledText st;

        st.id = _preferences.getUChar((prefix + "id").c_str(), 0);
        String text = _preferences.getString((prefix + "txt").c_str(), "");
        strncpy(st.text, text.c_str(), sizeof(st.text) - 1);
        st.text[sizeof(st.text) - 1] = '\0';
        st.color = _preferences.getUShort((prefix + "col").c_str(), 0xFFE0);
        st.hour = _preferences.getUChar((prefix + "h").c_str(), 0);
        st.minute = _preferences.getUChar((prefix + "m").c_str(), 0);
        st.repeatDays = _preferences.getUChar((prefix + "rep").c_str(), 0xFF);
        st.year = _preferences.getUShort((prefix + "y").c_str(), 0);
        st.month = _preferences.getUChar((prefix + "mon").c_str(), 0);
        st.day = _preferences.getUChar((prefix + "d").c_str(), 0);
        st.loopCount = _preferences.getUChar((prefix + "loop").c_str(), 1);
        st.enabled = _preferences.getBool((prefix + "en").c_str(), true);

        if (st.id > 0 && st.text[0] != '\0') {
            _scheduledTexts.push_back(st);
            if (st.id >= _nextId) {
                _nextId = st.id + 1;
            }
        }
    }

    DEBUG_PRINTF("[TextSchedule] Loaded %d scheduled texts\n", _scheduledTexts.size());
    print();
}

void TextScheduleManager::print() const {
    DEBUG_PRINTLN(F("╔════════════════════════════════════════════════════╗"));
    DEBUG_PRINTLN(F("║           Scheduled Texts                          ║"));
    DEBUG_PRINTLN(F("╠════════════════════════════════════════════════════╣"));

    if (_scheduledTexts.empty()) {
        DEBUG_PRINTLN(F("║  No scheduled texts                                ║"));
    } else {
        for (const auto& st : _scheduledTexts) {
            DEBUG_PRINTF("║ [%3d] %02d:%02d %-32s║\n", st.id, st.hour, st.minute, st.text);
            DEBUG_PRINTF("║       Color: 0x%04X %s                  ║\n",
                        st.color, st.enabled ? "[ON]" : "[OFF]");

            // Print repeat pattern
            if (st.year != 0 || st.month != 0 || st.day != 0) {
                DEBUG_PRINTF("║       Date: %04d-%02d-%02d                           ║\n",
                            st.year, st.month, st.day);
            } else if (st.repeatDays != 0xFF) {
                DEBUG_PRINT(F("║       Days: "));
                if (st.repeatDays & 0x01) DEBUG_PRINT(F("Mon "));
                if (st.repeatDays & 0x02) DEBUG_PRINT(F("Tue "));
                if (st.repeatDays & 0x04) DEBUG_PRINT(F("Wed "));
                if (st.repeatDays & 0x08) DEBUG_PRINT(F("Thu "));
                if (st.repeatDays & 0x10) DEBUG_PRINT(F("Fri "));
                if (st.repeatDays & 0x20) DEBUG_PRINT(F("Sat "));
                if (st.repeatDays & 0x40) DEBUG_PRINT(F("Sun"));
                DEBUG_PRINTLN(F("                      ║"));
            } else {
                DEBUG_PRINTLN(F("║       Repeat: Every day                            ║"));
            }
            DEBUG_PRINTLN(F("╟────────────────────────────────────────────────────╢"));
        }
    }

    DEBUG_PRINTLN(F("╚════════════════════════════════════════════════════╝"));
}
