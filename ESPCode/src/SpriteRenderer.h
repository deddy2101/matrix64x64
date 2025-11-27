#ifndef SPRITE_RENDERER_H
#define SPRITE_RENDERER_H

#include "DisplayManager.h"
#include <Arduino.h>
#include "assets.h"  // Per _MASK e sprite definitions

class SpriteRenderer {
private:
    DisplayManager* display;
    
public:
    SpriteRenderer(DisplayManager* dm) : display(dm) {}
    
    // Disegna sprite RGB565 da PROGMEM
    void drawSprite(const uint16_t* sprite, int x, int y, int width, int height) {
        for (int dy = 0; dy < height; dy++) {
            for (int dx = 0; dx < width; dx++) {
                uint16_t color = pgm_read_word(&sprite[dy * width + dx]);
                
                if (color != _MASK) {
                    uint8_t r = (color >> 11) << 3;
                    uint8_t g = ((color >> 5) & 0x3F) << 2;
                    uint8_t b = (color & 0x1F) << 3;
                    
                    display->drawPixel(x + dx, y + dy, r, g, b);
                }
            }
        }
    }
    
    // Disegna sprite con flip orizzontale
    void drawSpriteFlipped(const uint16_t* sprite, int x, int y, 
                          int width, int height, bool flipH) {
        for (int dy = 0; dy < height; dy++) {
            for (int dx = 0; dx < width; dx++) {
                int srcX = flipH ? (width - 1 - dx) : dx;
                uint16_t color = pgm_read_word(&sprite[dy * width + srcX]);
                
                if (color != _MASK) {
                    uint8_t r = (color >> 11) << 3;
                    uint8_t g = ((color >> 5) & 0x3F) << 2;
                    uint8_t b = (color & 0x1F) << 3;
                    
                    display->drawPixel(x + dx, y + dy, r, g, b);
                }
            }
        }
    }
    
    // Disegna una tile ripetuta (per terreno, ecc.)
    void drawTile(const uint16_t* tile, int tileWidth, int tileHeight,
                  int x, int y, int width, int height) {
        for (int dy = 0; dy < height; dy++) {
            for (int dx = 0; dx < width; dx++) {
                int tileX = dx % tileWidth;
                int tileY = dy % tileHeight;
                int idx = tileY * tileWidth + tileX;
                
                uint16_t color = pgm_read_word(&tile[idx]);
                
                if (color != _MASK) {
                    uint8_t r = (color >> 11) << 3;
                    uint8_t g = ((color >> 5) & 0x3F) << 2;
                    uint8_t b = (color & 0x1F) << 3;
                    
                    display->drawPixel(x + dx, y + dy, r, g, b);
                }
            }
        }
    }
};

#endif