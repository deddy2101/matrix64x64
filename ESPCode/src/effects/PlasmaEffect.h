#ifndef PLASMA_EFFECT_H
#define PLASMA_EFFECT_H

#include "../Effect.h"
#include <FastLED.h>

class PlasmaEffect : public Effect {
private:
    uint16_t timeCounter;
    uint16_t cycles;
    CRGBPalette16 currentPalette;
    CRGBPalette16 palettes[5];
    int currentPaletteIndex;
    
    CRGB colorFromPalette(uint8_t index, uint8_t brightness = 255);
    
public:
    PlasmaEffect(DisplayManager* dm);
    
    void init() override;
    void update() override;
    void draw() override;
    const char* getName() override { return "Plasma"; }
};

#endif
