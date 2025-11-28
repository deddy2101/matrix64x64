#ifndef EFFECT_MANAGER_H
#define EFFECT_MANAGER_H

#include "Effect.h"
#include <vector>

class EffectManager {
private:
    std::vector<Effect*> effects;
    int currentEffectIndex;
    unsigned long effectStartTime;
    unsigned long effectDuration;  // ms
    DisplayManager* displayManager;
    bool autoSwitch;
    
    /**
     * Cambia all'effetto specificato (uso interno)
     */
    void changeToEffect(int index) {
        if (index < 0 || index >= effects.size()) return;
        
        // Disattiva effetto corrente
        if (currentEffectIndex >= 0 && currentEffectIndex < effects.size()) {
            Serial.printf("[EffectManager] Switching from effect: %s to %s\n",
                          effects[currentEffectIndex]->getName(),
                          effects[index]->getName());
            effects[currentEffectIndex]->deactivate();
        }
        
        // Attiva nuovo effetto
        currentEffectIndex = index;
        effectStartTime = millis();
        effects[currentEffectIndex]->activate();
        Serial.printf("[EffectManager] Activated effect: %s\n", effects[currentEffectIndex]->getName());
    }
    
public:
    EffectManager(DisplayManager* dm, unsigned long duration = 10000);
    ~EffectManager();
    
    // Gestione effetti
    void addEffect(Effect* effect);
    void nextEffect();
    void setEffect(int index);
    void setDuration(unsigned long ms);
    void start();
    
    // Controllo manuale
    void setAutoSwitch(bool enabled);
    bool isAutoSwitch() const { return autoSwitch; }
    void switchToEffect(int index);
    void switchToEffect(const char* name);
    void pause();
    void resume();
    
    // Loop principale
    void update();
    
    // Info
    Effect* getCurrentEffect();
    int getCurrentEffectIndex() const { return currentEffectIndex; }
    int getEffectCount() const { return effects.size(); }
    const char* getEffectName(int index) const;
    unsigned long getEffectRuntime() const { 
        return millis() - effectStartTime; 
    }
    
    // Statistiche
    void printStats();
};

#endif
