#ifndef STARFIELD_EFFECT_H
#define STARFIELD_EFFECT_H

#include "../Effect.h"

#define MAX_STARS 50

struct Star {
    float x;
    float y;
    float z;
};

class StarfieldEffect : public Effect {
private:
    Star stars[MAX_STARS];
    
    void initStars();
    
public:
    StarfieldEffect(DisplayManager* dm);
    
    void init() override;
    void update() override;
    void draw() override;
    const char* getName() override { return "Starfield"; }
};

#endif