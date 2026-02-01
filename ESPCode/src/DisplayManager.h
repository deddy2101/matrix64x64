#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "Debug.h"


class DisplayManager {
private:
    MatrixPanel_I2S_DMA* display;
    uint16_t width;
    uint16_t height;
    uint8_t brightness;

    // Framebuffer for flicker-free rendering
    uint16_t* frameBuffer;
    bool bufferingEnabled;

    // Local state for buffered text rendering
    int16_t bufCursorX, bufCursorY;
    const GFXfont* currentFont;
    uint16_t currentTextColor;
    uint8_t currentTextSize;

    // Buffered text helpers
    void bufferRenderChar(char c);

public:
    DisplayManager(uint16_t panelWidth, uint16_t panelHeight,
                   uint8_t panelsNumber, uint8_t pinE);
    ~DisplayManager();

    bool begin();
    void setBrightness(uint8_t level);

    // Framebuffer control (opt-in per effect)
    void beginFrame();
    void endFrame();

    // Metodi di disegno wrapper
    void fillScreen(uint8_t r, uint8_t g, uint8_t b);
    void drawPixel(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b);
    void drawPixel(int16_t x, int16_t y, uint16_t color565);

    // Metodi per testo
    void setFont(const GFXfont* font);
    void setTextColor(uint16_t color);
    void setTextSize(uint8_t size);
    void setTextWrap(bool wrap);
    void setCursor(int16_t x, int16_t y);
    void print(const String& text);

    // Getters
    uint16_t getWidth() const { return width; }
    uint16_t getHeight() const { return height; }
    MatrixPanel_I2S_DMA* getDisplay() { return display; }

    // Conversione colori
    static uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
    static void rgb565ToRgb888(uint16_t color565, uint8_t& r, uint8_t& g, uint8_t& b);

    // OTA Progress Display
    void showOTAProgress(int percent);
    void showOTASuccess();
};

#endif
