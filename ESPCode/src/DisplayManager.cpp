#include "DisplayManager.h"

DisplayManager::DisplayManager(uint16_t panelWidth, uint16_t panelHeight, 
                               uint8_t panelsNumber, uint8_t pinE)
    : width(panelWidth * panelsNumber), height(panelHeight), brightness(200) {
    
    HUB75_I2S_CFG mxconfig;
    mxconfig.mx_height = panelHeight;
    mxconfig.chain_length = panelsNumber;
    mxconfig.gpio.e = pinE;
    mxconfig.clkphase = false;
    mxconfig.latch_blanking = 4;
    
    display = new MatrixPanel_I2S_DMA(mxconfig);
}

DisplayManager::~DisplayManager() {
    if (display) {
        delete display;
    }
}

bool DisplayManager::begin() {
    if (!display->begin()) {
        DEBUG_PRINTLN("ERROR: I2S memory allocation failed");
        return false;
    }
    display->setBrightness8(brightness);
    return true;
}

void DisplayManager::setBrightness(uint8_t level) {
    brightness = level;
    if (display) {
        display->setBrightness8(level);
    }
}

void DisplayManager::fillScreen(uint8_t r, uint8_t g, uint8_t b) {
    if (display) {
        display->fillScreenRGB888(r, g, b);
    }
}

void DisplayManager::drawPixel(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
    if (display && x >= 0 && x < width && y >= 0 && y < height) {
        display->drawPixelRGB888(x, y, r, g, b);
    }
}

void DisplayManager::drawPixel(int16_t x, int16_t y, uint16_t color565) {
    uint8_t r, g, b;
    rgb565ToRgb888(color565, r, g, b);
    drawPixel(x, y, r, g, b);
}

void DisplayManager::setFont(const GFXfont* font) {
    if (display) display->setFont(font);
}

void DisplayManager::setTextColor(uint16_t color) {
    if (display) display->setTextColor(color);
}

void DisplayManager::setTextSize(uint8_t size) {
    if (display) display->setTextSize(size);
}

void DisplayManager::setTextWrap(bool wrap) {
    if (display) display->setTextWrap(wrap);
}

void DisplayManager::setCursor(int16_t x, int16_t y) {
    if (display) display->setCursor(x, y);
}

void DisplayManager::print(const String& text) {
    if (display) display->print(text);
}

uint16_t DisplayManager::color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void DisplayManager::rgb565ToRgb888(uint16_t color565, uint8_t& r, uint8_t& g, uint8_t& b) {
    r = (color565 >> 11) << 3;
    g = ((color565 >> 5) & 0x3F) << 2;
    b = (color565 & 0x1F) << 3;
}
