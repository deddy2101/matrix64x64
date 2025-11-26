#include "EffectManager.h"

EffectManager::EffectManager(DisplayManager* dm, unsigned long duration)
    : displayManager(dm), effectDuration(duration), 
      currentEffectIndex(0), effectStartTime(0) {
}

EffectManager::~EffectManager() {
    // Gli effetti vengono eliminati dal chiamante
    effects.clear();
}

void EffectManager::addEffect(Effect* effect) {
    if (effect) {
        effects.push_back(effect);
        Serial.printf("Added effect: %s (total: %d)\n", 
                     effect->getName(), effects.size());
    }
}

void EffectManager::nextEffect() {
    if (effects.empty()) return;
    
    // Reset effetto corrente
    if (effects[currentEffectIndex]) {
        effects[currentEffectIndex]->reset();
    }
    
    // Passa al prossimo
    currentEffectIndex = (currentEffectIndex + 1) % effects.size();
    effectStartTime = millis();
    
    Serial.printf("Switching to effect: %s\n", 
                 effects[currentEffectIndex]->getName());
}

void EffectManager::setEffect(int index) {
    if (index >= 0 && index < effects.size()) {
        if (effects[currentEffectIndex]) {
            effects[currentEffectIndex]->reset();
        }
        currentEffectIndex = index;
        effectStartTime = millis();
        
        Serial.printf("Set to effect: %s\n", 
                     effects[currentEffectIndex]->getName());
    }
}

void EffectManager::setDuration(unsigned long ms) {
    effectDuration = ms;
}

void EffectManager::update() {
    if (effects.empty()) return;
    
    Effect* current = effects[currentEffectIndex];
    if (!current) return;
    
    // Esegui l'effetto corrente
    current->execute();
    
    // Controlla se è il momento di cambiare effetto
    bool shouldSwitch = false;
    
    // Cambia se l'effetto è completo O se è scaduto il tempo
    if (current->isComplete()) {
        shouldSwitch = true;
    } else if (getEffectRuntime() >= effectDuration) {
        shouldSwitch = true;
    }
    
    if (shouldSwitch) {
        nextEffect();
    }
}

Effect* EffectManager::getCurrentEffect() {
    if (currentEffectIndex >= 0 && currentEffectIndex < effects.size()) {
        return effects[currentEffectIndex];
    }
    return nullptr;
}

void EffectManager::printStats() {
    Effect* current = getCurrentEffect();
    if (current) {
        Serial.printf("Effect: %s | Runtime: %lu ms | FPS: %.1f | Frames: %d\n",
                     current->getName(),
                     current->getRuntime(),
                     current->getFPS(),
                     current->getFrameCount());
    }
}
