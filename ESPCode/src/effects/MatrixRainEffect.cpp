#include "MatrixRainEffect.h"

MatrixRainEffect::MatrixRainEffect(DisplayManager* dm) 
    : Effect(dm) {
}

void MatrixRainEffect::init() {
    Serial.println("Initializing Matrix Rain Effect");
    initDrops();
    displayManager->fillScreen(0, 0, 0);
}

void MatrixRainEffect::initDrops() {
    int width = displayManager->getWidth();
    
    for (int i = 0; i < MAX_DROPS; i++) {
        drops[i].x = random(width);
        drops[i].y = random(-50, 0);
        drops[i].speed = random(1, 4);
        drops[i].length = random(5, 15);
        drops[i].active = true;
    }
}

void MatrixRainEffect::update() {
    int width = displayManager->getWidth();
    int height = displayManager->getHeight();
    
    for (int i = 0; i < MAX_DROPS; i++) {
        if (drops[i].active) {
            drops[i].y += drops[i].speed;
            
            // Se la goccia Ã¨ uscita dallo schermo, resettala
            if (drops[i].y > height + drops[i].length) {
                drops[i].y = random(-20, 0);
                drops[i].x = random(width);
                drops[i].speed = random(1, 4);
                drops[i].length = random(5, 15);
            }
        }
    }
}

void MatrixRainEffect::draw() {
    int width = displayManager->getWidth();
    int height = displayManager->getHeight();
    
    // Fade dello sfondo (effetto scia)
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            displayManager->drawPixel(x, y, 0, 2, 0);
        }
    }
    
    // Disegna le gocce
    for (int i = 0; i < MAX_DROPS; i++) {
        if (drops[i].active) {
            for (int j = 0; j < drops[i].length; j++) {
                int y = drops[i].y - j;
                if (y >= 0 && y < height) {
                    // Calcola brightness (piÃ¹ luminoso in testa, piÃ¹ scuro in coda)
                    int brightness = 255 - (j * 20);
                    if (brightness < 0) brightness = 0;
                    
                    displayManager->drawPixel(drops[i].x, y, 0, brightness, 0);
                }
            }
        }
    }
}