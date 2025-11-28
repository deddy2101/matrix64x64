#include "StarFieldEffect.h"

StarfieldEffect::StarfieldEffect(DisplayManager* dm) 
    : Effect(dm) {
}

void StarfieldEffect::init() {
    DEBUG_PRINTLN("Initializing Starfield Effect");
    initStars();
    displayManager->fillScreen(0, 0, 5);
}

void StarfieldEffect::initStars() {
    int width = displayManager->getWidth();
    int height = displayManager->getHeight();
    
    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].x = random(-width, width);
        stars[i].y = random(-height, height);
        stars[i].z = random(1, width);
    }
}

void StarfieldEffect::update() {
    int width = displayManager->getWidth();
    int height = displayManager->getHeight();
    
    for (int i = 0; i < MAX_STARS; i++) {
        // Muovi stella verso di noi (z diminuisce)
        stars[i].z -= 0.5;
        
        // Se la stella ci ha superato, ricreala in fondo
        if (stars[i].z <= 0) {
            stars[i].x = random(-width, width);
            stars[i].y = random(-height, height);
            stars[i].z = width;
        }
    }
}

void StarfieldEffect::draw() {
    int width = displayManager->getWidth();
    int height = displayManager->getHeight();
    
    // Sfondo nero/blu scuro
    displayManager->fillScreen(0, 0, 5);
    
    // Disegna stelle
    for (int i = 0; i < MAX_STARS; i++) {
        // Proiezione 3D â†’ 2D (prospettiva)
        int sx = (stars[i].x / stars[i].z) * width + width / 2;
        int sy = (stars[i].y / stars[i].z) * height + height / 2;
        
        // Disegna solo se dentro lo schermo
        if (sx >= 0 && sx < width && sy >= 0 && sy < height) {
            // LuminositÃ  basata su z (piÃ¹ vicino = piÃ¹ luminoso)
            int brightness = map(stars[i].z, 0, width, 255, 50);
            displayManager->drawPixel(sx, sy, brightness, brightness, brightness);
        }
    }
}