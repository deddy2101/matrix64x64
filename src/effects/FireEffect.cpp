#include "FireEffect.h"

FireEffect::FireEffect(DisplayManager* dm) 
    : Effect(dm), heat(nullptr), width(0), height(0) {
}

FireEffect::~FireEffect() {
    freeHeat();
}

void FireEffect::allocateHeat() {
    // Prima libera eventuale memoria precedente
    freeHeat();
    
    width = displayManager->getWidth();
    height = displayManager->getHeight();
    
    // Alloca matrice dinamica
    heat = new byte*[width];
    for (int x = 0; x < width; x++) {
        heat[x] = new byte[height];
        for (int y = 0; y < height; y++) {
            heat[x][y] = 0;
        }
    }
}

void FireEffect::freeHeat() {
    if (heat) {
        for (int x = 0; x < width; x++) {
            if (heat[x]) {
                delete[] heat[x];
            }
        }
        delete[] heat;
        heat = nullptr;
    }
    width = 0;
    height = 0;
}

void FireEffect::init() {
    Serial.println("[FireEffect] Initializing");
    
    allocateHeat();
    displayManager->fillScreen(0, 0, 0);
}

void FireEffect::cleanup() {
    Serial.println("[FireEffect] Cleanup - freeing heat buffer");
    freeHeat();
}

void FireEffect::update() {
    if (!heat) return;  // Safety check
    
    // Step 1: Raffredda ogni cella
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            heat[x][y] = qsub8(heat[x][y], random8(0, ((55 * 10) / height) + 2));
        }
    }
    
    // Step 2: Diffusione del calore verso l'alto
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height - 1; y++) {
            heat[x][y] = (heat[x][y + 1] + heat[x][y + 1] + heat[x][y + 1]) / 3;
        }
    }
    
    // Step 3: Accendi nuove "scintille" in fondo
    if (random8() < 120) {
        int x = random(width);
        heat[x][height - 1] = qadd8(heat[x][height - 1], random8(160, 255));
    }
}

void FireEffect::draw() {
    if (!heat) return;  // Safety check
    
    // Disegna il fuoco usando la palette HeatColors
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            CRGB color = HeatColor(heat[x][y]);
            displayManager->drawPixel(x, y, color.r, color.g, color.b);
        }
    }
}
