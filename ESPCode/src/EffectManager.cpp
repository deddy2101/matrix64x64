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
        DEBUG_PRINTF("[EffectManager] Added effect: %s (total: %d)\n", 
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
        DEBUG_PRINTF("[EffectManager] Started with effect: %s\n", 
                     effects[0]->getName());
    } else {
        DEBUG_PRINTLN("[EffectManager] Warning: No effects to start! Defaulting to first effect.");
        changeToEffect(0);
    }    
    */
   //se non è selezionato nessun effetto, seleziono il primo
    if(currentEffectIndex == -1 && !effects.empty()) {
        DEBUG_PRINTLN("[EffectManager] No effect selected, defaulting to first effect.");
        changeToEffect(0);
    } else {
        DEBUG_PRINTF("[EffectManager] Starting with effect: %s\n", 
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
        DEBUG_PRINTLN("[EffectManager] Checking for effect switch...");
        bool shouldSwitch = false;
        
        // Cambia se l'effetto è completo O se è scaduto il tempo
        if (current->isComplete()) {
            DEBUG_PRINTLN("[EffectManager] Effect complete, switching...");
            shouldSwitch = true;
        } else if (getEffectRuntime() >= effectDuration) {
            DEBUG_PRINTLN("[EffectManager] Effect duration elapsed, switching...");
            shouldSwitch = true;
        }
        
        if (shouldSwitch) {
            DEBUG_PRINTLN("[EffectManager] Switching to next effect...");
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
        DEBUG_PRINTF("[Stats] Effect: %s | Runtime: %lu ms | FPS: %.1f | Frames: %d%s\n",
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
        DEBUG_PRINTLN("[EffectManager] Auto-switch ENABLED");
        effectStartTime = millis();  // Reset timer
    } else {
        DEBUG_PRINTLN("[EffectManager] Auto-switch DISABLED (manual mode)");
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
        DEBUG_PRINTF("[EffectManager] Error: Effect index %d out of range (0-%d)\n", 
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
    DEBUG_PRINTF("[EffectManager] Error: Effect '%s' not found\n", name);
}

const char* EffectManager::getEffectName(int index) const {
    if (index >= 0 && index < effects.size()) {
        return effects[index]->getName();
    }
    return nullptr;
}
