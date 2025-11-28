#include "EffectManager.h"

EffectManager::EffectManager(DisplayManager* dm, unsigned long duration)
    : displayManager(dm), effectDuration(duration), 
      currentEffectIndex(-1), effectStartTime(0), autoSwitch(true) {
}

EffectManager::~EffectManager() {
    // Disattiva effetto corrente prima di distruggere
    if (currentEffectIndex >= 0 && currentEffectIndex < effects.size()) {
        effects[currentEffectIndex]->deactivate();
    }
    effects.clear();
}

void EffectManager::addEffect(Effect* effect) {
    if (effect) {
        effects.push_back(effect);
        Serial.printf("[EffectManager] Added effect: %s (total: %d)\n", 
                     effect->getName(), effects.size());
    }
}

void EffectManager::nextEffect() {
    if (effects.empty()) return;
    
    int nextIndex = (currentEffectIndex + 1) % effects.size();
    changeToEffect(nextIndex);
}

void EffectManager::setEffect(int index) {
    if (index >= 0 && index < effects.size()) {
        changeToEffect(index);
    }
}

void EffectManager::setDuration(unsigned long ms) {
    effectDuration = ms;
}

void EffectManager::start() {
    /*
    if (effects.empty()) {
        Serial.printf("[EffectManager] Started with effect: %s\n", 
                     effects[0]->getName());
    } else {
        Serial.println("[EffectManager] Warning: No effects to start! Defaulting to first effect.");
        changeToEffect(0);
    }    
    */
   //se non è selezionato nessun effetto, seleziono il primo
    if(currentEffectIndex == -1 && !effects.empty()) {
        Serial.println("[EffectManager] No effect selected, defaulting to first effect.");
        changeToEffect(0);
    } else {
        Serial.printf("[EffectManager] Starting with effect: %s\n", 
                     effects[currentEffectIndex]->getName());
    }
}

void EffectManager::update() {
    if (effects.empty() || currentEffectIndex < 0) return;
    
    Effect* current = effects[currentEffectIndex];
    if (!current) return;
    
    // Esegui l'effetto corrente
    current->execute();
    
    // Controlla se è il momento di cambiare effetto (SOLO se autoSwitch è attivo)
    if (autoSwitch) {
        Serial.println("[EffectManager] Checking for effect switch...");
        bool shouldSwitch = false;
        
        // Cambia se l'effetto è completo O se è scaduto il tempo
        if (current->isComplete()) {
            Serial.println("[EffectManager] Effect complete, switching...");
            shouldSwitch = true;
        } else if (getEffectRuntime() >= effectDuration) {
            Serial.println("[EffectManager] Effect duration elapsed, switching...");
            shouldSwitch = true;
        }
        
        if (shouldSwitch) {
            Serial.println("[EffectManager] Switching to next effect...");
            nextEffect();
        }
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
        Serial.printf("[Stats] Effect: %s | Runtime: %lu ms | FPS: %.1f | Frames: %d%s\n",
                     current->getName(),
                     current->getRuntime(),
                     current->getFPS(),
                     current->getFrameCount(),
                     autoSwitch ? "" : " [PAUSED]");
    }
}

// ========== CONTROLLO MANUALE ==========

void EffectManager::setAutoSwitch(bool enabled) {
    autoSwitch = enabled;
    if (enabled) {
        Serial.println("[EffectManager] Auto-switch ENABLED");
        effectStartTime = millis();  // Reset timer
    } else {
        Serial.println("[EffectManager] Auto-switch DISABLED (manual mode)");
    }
}

void EffectManager::pause() {
    setAutoSwitch(false);
}

void EffectManager::resume() {
    setAutoSwitch(true);
}

void EffectManager::switchToEffect(int index) {
    if (index >= 0 && index < effects.size()) {
        changeToEffect(index);
    } else {
        Serial.printf("[EffectManager] Error: Effect index %d out of range (0-%d)\n", 
                     index, effects.size() - 1);
    }
}

void EffectManager::switchToEffect(const char* name) {
    for (int i = 0; i < effects.size(); i++) {
        if (strcmp(effects[i]->getName(), name) == 0) {
            changeToEffect(i);
            return;
        }
    }
    Serial.printf("[EffectManager] Error: Effect '%s' not found\n", name);
}

const char* EffectManager::getEffectName(int index) const {
    if (index >= 0 && index < effects.size()) {
        return effects[index]->getName();
    }
    return nullptr;
}
