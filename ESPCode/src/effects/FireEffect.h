#ifndef FIRE_EFFECT_H
#define FIRE_EFFECT_H

#include "../Effect.h"
#include <FastLED.h>

class FireEffect : public Effect {
private:
    byte** heat;  // Matrice dinamica per il calore
    int width;
    int height;
    
    void allocateHeat();
    void freeHeat();
    
public:
    FireEffect(DisplayManager* dm);
    ~FireEffect();
    
    void init() override;
    void update() override;
    void draw() override;
    void cleanup() override;  // Libera la memoria allocata
    const char* getName() override { return "Fire"; }
};

#endif
