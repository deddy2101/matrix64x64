#ifndef IMAGE_EFFECT_H
#define IMAGE_EFFECT_H

#include "../Effect.h"
#include "../SpriteRenderer.h"

// Forward declaration per le funzioni draw_* dal tuo codice originale
// Queste sono definite nei file bosco.h, casa.h, etc.
typedef void (*DrawImageFunction)(void*, int, int);

class ImageEffect : public Effect {
private:
    SpriteRenderer* spriteRenderer;
    DrawImageFunction drawFunction;
    String imageName;
    bool imageDrawn;
    
public:
    // Costruttore che accetta una funzione draw_*
    ImageEffect(DisplayManager* dm, DrawImageFunction drawFunc, const String& name);
    ~ImageEffect();
    
    void init() override;
    void update() override;
    void draw() override;
    const char* getName() override;
};

#endif