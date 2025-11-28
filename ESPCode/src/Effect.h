#ifndef EFFECT_H
#define EFFECT_H

#include "DisplayManager.h"
#include <Arduino.h>
#include "Debug.h"


class Effect {
protected:
    DisplayManager* displayManager;
    unsigned long startTime;
    unsigned long lastUpdate;
    uint16_t frameCount;
    bool initialized;
    
public:
    Effect(DisplayManager* dm) 
        : displayManager(dm), startTime(0), lastUpdate(0), 
          frameCount(0), initialized(false) {}
    
    virtual ~Effect() {}
    
    // ========== METODI VIRTUALI PURI ==========
    // Ogni effetto DEVE implementare questi metodi
    
    /**
     * Inizializza l'effetto. Chiamato automaticamente quando l'effetto viene attivato.
     * Usare per:
     * - Reset delle variabili di stato
     * - Allocazione risorse
     * - Pulizia dello schermo
     */
    virtual void init() = 0;
    
    /**
     * Aggiorna la logica dell'effetto (chiamato ogni frame)
     */
    virtual void update() = 0;
    
    /**
     * Disegna l'effetto sullo schermo (chiamato ogni frame dopo update)
     */
    virtual void draw() = 0;
    
    /**
     * Ritorna il nome dell'effetto
     */
    virtual const char* getName() = 0;
    
    // ========== METODI VIRTUALI OPZIONALI ==========
    
    /**
     * Ritorna true se l'effetto ha completato il suo ciclo
     * (es: ScrollText che è uscito dallo schermo)
     */
    virtual bool isComplete() { return false; }
    
    /**
     * Cleanup opzionale chiamato quando l'effetto viene disattivato.
     * Usare per liberare risorse, fermare animazioni, etc.
     * Default: non fa nulla
     */
    virtual void cleanup() {}
    
    // ========== GESTIONE CICLO DI VITA ==========
    
    /**
     * Attiva l'effetto: chiama init() e resetta i contatori
     */
    void activate() {
        if (initialized) {
            cleanup();  // Pulisci stato precedente se era già attivo
        }
        
        frameCount = 0;
        startTime = millis();
        lastUpdate = startTime;
        
        init();  // Chiama l'init dell'effetto concreto
        initialized = true;
        
        DEBUG_PRINTF("[Effect] Activated: %s\n", getName());
    }
    
    /**
     * Disattiva l'effetto: chiama cleanup()
     */
    void deactivate() {
        if (initialized) {
            cleanup();
            initialized = false;
            DEBUG_PRINTF("[Effect] Deactivated: %s\n", getName());
        }
    }
    
    /**
     * Esegue un frame dell'effetto (update + draw)
     * Chiama activate() automaticamente se non inizializzato
     */
    void execute() {
        if (!initialized) {
            DEBUG_PRINTF("[Effect] Activating effect: %s\n", getName());
            activate();
        }
        update();
        draw();
        frameCount++;
        lastUpdate = millis();
    }
    
    /**
     * Reset completo dell'effetto (per riavviarlo da zero)
     */
    virtual void reset() {
        deactivate();
        // Al prossimo execute() verrà chiamato activate() -> init()
    }
    
    // ========== GETTERS ==========
    
    bool isInitialized() const { return initialized; }
    unsigned long getRuntime() const { return initialized ? (millis() - startTime) : 0; }
    unsigned long getLastUpdateTime() const { return lastUpdate; }
    uint16_t getFrameCount() const { return frameCount; }
    
    float getFPS() const {
        unsigned long runtime = getRuntime();
        return runtime > 0 ? (frameCount * 1000.0f / runtime) : 0;
    }
};

#endif
