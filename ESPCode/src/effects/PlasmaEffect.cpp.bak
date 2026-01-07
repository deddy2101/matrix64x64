#include "PlasmaEffect.h"

PlasmaEffect::PlasmaEffect(DisplayManager* dm) 
    : Effect(dm), timeCounter(0), cycles(0), currentPaletteIndex(0) {
    
    // Inizializza le palette disponibili
    palettes[0] = HeatColors_p;
    palettes[1] = LavaColors_p;
    palettes[2] = RainbowColors_p;
    palettes[3] = RainbowStripeColors_p;
    palettes[4] = CloudColors_p;
}

void PlasmaEffect::init() {
    DEBUG_PRINTLN("Initializing Plasma Effect");
    
    // Palette casuale
    currentPaletteIndex = random(0, 5);
    currentPalette = palettes[currentPaletteIndex];
    
    timeCounter = 0;
    cycles = 0;
    
    displayManager->fillScreen(0, 0, 0);
}

void PlasmaEffect::update() {
    timeCounter++;
    cycles++;
    
    // Cambia palette ogni 1024 cicli
    if (cycles >= 1024) {
        currentPaletteIndex = random(0, 5);
        currentPalette = palettes[currentPaletteIndex];
        cycles = 0;
        timeCounter = 0;
        
        DEBUG_PRINTF("Plasma: Changed to palette %d\n", currentPaletteIndex);
    }
}

void PlasmaEffect::draw() {
    int width = displayManager->getWidth();
    int height = displayManager->getHeight();
    
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            // Algoritmo plasma classico
            int16_t v = 128;
            uint8_t wibble = sin8(timeCounter);
            
            v += sin16(x * wibble * 3 + timeCounter);
            v += cos16(y * (128 - wibble) + timeCounter);
            v += sin16(y * x * cos8(-timeCounter) / 8);
            
            CRGB color = colorFromPalette((v >> 8));
            displayManager->drawPixel(x, y, color.r, color.g, color.b);
        }
    }
}

CRGB PlasmaEffect::colorFromPalette(uint8_t index, uint8_t brightness) {
    return ColorFromPalette(currentPalette, index, brightness, LINEARBLEND);
}
