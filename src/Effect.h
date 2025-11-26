#ifndef EFFECT_H
#define EFFECT_H

#include "DisplayManager.h"
#include <Arduino.h>

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
    
    // Metodi da implementare nelle classi derivate
    virtual void init() = 0;
    virtual void update() = 0;
    virtual void draw() = 0;
    virtual bool isComplete() { return false; }
    virtual const char* getName() = 0;
    
    // Metodo template per eseguire un ciclo completo
    void execute() {
        if (!initialized) {
            init();
            initialized = true;
            startTime = millis();
        }
        update();
        draw();
        frameCount++;
    }
    
    // Reset effect
    virtual void reset() {
        initialized = false;
        frameCount = 0;
        startTime = 0;
        lastUpdate = 0;
    }
    
    // Getters
    unsigned long getRuntime() const { return millis() - startTime; }
    uint16_t getFrameCount() const { return frameCount; }
    float getFPS() const {
        unsigned long runtime = getRuntime();
        return runtime > 0 ? (frameCount * 1000.0f / runtime) : 0;
    }
};

#endif
