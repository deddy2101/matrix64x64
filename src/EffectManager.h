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
    
public:
    EffectManager(DisplayManager* dm, unsigned long duration = 10000);
    ~EffectManager();
    
    // Gestione effetti
    void addEffect(Effect* effect);
    void nextEffect();
    void setEffect(int index);
    void setDuration(unsigned long ms);
    
    // Loop principale
    void update();
    
    // Info
    Effect* getCurrentEffect();
    int getCurrentEffectIndex() const { return currentEffectIndex; }
    int getEffectCount() const { return effects.size(); }
    unsigned long getEffectRuntime() const { 
        return millis() - effectStartTime; 
    }
    
    // Statistiche
    void printStats();
};

#endif
