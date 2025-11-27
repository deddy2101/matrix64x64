#ifndef MATRIX_RAIN_EFFECT_H
#define MATRIX_RAIN_EFFECT_H

#include "../Effect.h"

#define MAX_DROPS 20

struct Drop {
    int x;
    int y;
    int speed;
    int length;
    bool active;
};

class MatrixRainEffect : public Effect {
private:
    Drop drops[MAX_DROPS];
    
    void initDrops();
    
public:
    MatrixRainEffect(DisplayManager* dm);
    
    void init() override;
    void update() override;
    void draw() override;
    const char* getName() override { return "Matrix Rain"; }
};

#endif