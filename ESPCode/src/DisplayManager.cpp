#include "DisplayManager.h"

DisplayManager::DisplayManager(uint16_t panelWidth, uint16_t panelHeight,
                               uint8_t panelsNumber, uint8_t pinE)
    : width(panelWidth * panelsNumber), height(panelHeight), brightness(200),
      frameBuffer(nullptr), bufferingEnabled(false),
      bufCursorX(0), bufCursorY(0), currentFont(nullptr),
      currentTextColor(0xFFFF), currentTextSize(1) {

    HUB75_I2S_CFG mxconfig;
    mxconfig.mx_height = panelHeight;
    mxconfig.chain_length = panelsNumber;
    mxconfig.gpio.e = pinE;
    mxconfig.clkphase = false;
    mxconfig.latch_blanking = 4;

    display = new MatrixPanel_I2S_DMA(mxconfig);

    // Allocate framebuffer (RGB565, ~8KB for 64x64)
    frameBuffer = new uint16_t[width * height];
}

DisplayManager::~DisplayManager() {
    delete[] frameBuffer;
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

// ═══════════════════════════════════════════
// Framebuffer control
// ═══════════════════════════════════════════

void DisplayManager::beginFrame() {
    bufferingEnabled = true;
}

void DisplayManager::endFrame() {
    if (!bufferingEnabled || !display) {
        bufferingEnabled = false;
        return;
    }

    // Flush entire buffer to display in one pass
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint16_t c = frameBuffer[y * width + x];
            uint8_t r = (c >> 11) << 3;
            uint8_t g = ((c >> 5) & 0x3F) << 2;
            uint8_t b = (c & 0x1F) << 3;
            display->drawPixelRGB888(x, y, r, g, b);
        }
    }

    bufferingEnabled = false;
}

// ═══════════════════════════════════════════
// Drawing methods (buffer-aware)
// ═══════════════════════════════════════════

void DisplayManager::fillScreen(uint8_t r, uint8_t g, uint8_t b) {
    if (bufferingEnabled) {
        uint16_t c = color565(r, g, b);
        int total = width * height;
        for (int i = 0; i < total; i++) {
            frameBuffer[i] = c;
        }
    } else if (display) {
        display->fillScreenRGB888(r, g, b);
    }
}

void DisplayManager::drawPixel(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;

    if (bufferingEnabled) {
        frameBuffer[y * width + x] = color565(r, g, b);
    } else if (display) {
        display->drawPixelRGB888(x, y, r, g, b);
    }
}

void DisplayManager::drawPixel(int16_t x, int16_t y, uint16_t col565) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;

    if (bufferingEnabled) {
        frameBuffer[y * width + x] = col565;
    } else {
        uint8_t r, g, b;
        rgb565ToRgb888(col565, r, g, b);
        if (display) display->drawPixelRGB888(x, y, r, g, b);
    }
}

void DisplayManager::setFont(const GFXfont* font) {
    currentFont = font;
    if (display) display->setFont(font);
}

void DisplayManager::setTextColor(uint16_t color) {
    currentTextColor = color;
    if (display) display->setTextColor(color);
}

void DisplayManager::setTextSize(uint8_t size) {
    currentTextSize = size;
    if (display) display->setTextSize(size);
}

void DisplayManager::setTextWrap(bool wrap) {
    if (display) display->setTextWrap(wrap);
}

void DisplayManager::setCursor(int16_t x, int16_t y) {
    bufCursorX = x;
    bufCursorY = y;
    if (display) display->setCursor(x, y);
}

void DisplayManager::print(const String& text) {
    if (bufferingEnabled && currentFont) {
        // Render GFX font directly into framebuffer
        for (unsigned int i = 0; i < text.length(); i++) {
            bufferRenderChar(text.charAt(i));
        }
    } else if (display) {
        display->print(text);
    }
}

void DisplayManager::bufferRenderChar(char c) {
    if (!currentFont) return;

    uint8_t first = pgm_read_byte(&currentFont->first);
    uint8_t last = pgm_read_byte(&currentFont->last);

    if (c < first || c > last) return;

    GFXglyph* glyphTable = (GFXglyph*)pgm_read_ptr(&currentFont->glyph);
    GFXglyph glyph;
    memcpy_P(&glyph, &glyphTable[c - first], sizeof(GFXglyph));

    uint8_t* bitmapData = (uint8_t*)pgm_read_ptr(&currentFont->bitmap);

    uint16_t bitmapOffset = glyph.bitmapOffset;
    uint8_t gw = glyph.width;
    uint8_t gh = glyph.height;
    uint8_t xAdvance = glyph.xAdvance;
    int8_t xo = glyph.xOffset;
    int8_t yo = glyph.yOffset;

    uint8_t bit = 0;
    uint8_t bits = 0;

    for (int yy = 0; yy < gh; yy++) {
        for (int xx = 0; xx < gw; xx++) {
            if (!(bit & 7)) {
                bits = pgm_read_byte(&bitmapData[bitmapOffset++]);
            }
            if (bits & 0x80) {
                int px = bufCursorX + (xo + xx) * currentTextSize;
                int py = bufCursorY + (yo + yy) * currentTextSize;
                // Draw scaled pixel
                for (int sy = 0; sy < currentTextSize; sy++) {
                    for (int sx = 0; sx < currentTextSize; sx++) {
                        int fx = px + sx;
                        int fy = py + sy;
                        if (fx >= 0 && fx < width && fy >= 0 && fy < height) {
                            frameBuffer[fy * width + fx] = currentTextColor;
                        }
                    }
                }
            }
            bits <<= 1;
            bit++;
        }
    }

    bufCursorX += xAdvance * currentTextSize;
}

uint16_t DisplayManager::color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void DisplayManager::rgb565ToRgb888(uint16_t color565, uint8_t& r, uint8_t& g, uint8_t& b) {
    r = (color565 >> 11) << 3;
    g = ((color565 >> 5) & 0x3F) << 2;
    b = (color565 & 0x1F) << 3;
}

// ═══════════════════════════════════════════
// OTA Progress Display
// ═══════════════════════════════════════════

void DisplayManager::showOTAProgress(int percent) {
    if (!display) return;

    display->clearScreen();

    // ✅ Reset font al default (importante se un effetto ha impostato font custom)
    display->setFont(nullptr);

    // Titolo "OTA"
    display->setTextSize(2);
    display->setTextColor(color565(255, 165, 0)); // Arancione
    display->setCursor(14, 12);
    display->print("OTA");

    // Percentuale
    display->setTextSize(2);
    display->setTextColor(color565(0, 255, 255)); // Cyan
    display->setCursor(8, 30);
    display->print(String(percent) + "%");

    // Barra di progresso (48x8 pixel, centrata)
    int barWidth = 48;
    int barHeight = 6;
    int barX = (width - barWidth) / 2;
    int barY = 54;

    // Bordo barra
    for (int x = 0; x < barWidth; x++) {
        display->drawPixelRGB888(barX + x, barY, 100, 100, 100);
        display->drawPixelRGB888(barX + x, barY + barHeight - 1, 100, 100, 100);
    }
    for (int y = 0; y < barHeight; y++) {
        display->drawPixelRGB888(barX, barY + y, 100, 100, 100);
        display->drawPixelRGB888(barX + barWidth - 1, barY + y, 100, 100, 100);
    }

    // Riempimento barra (proporzionale al progresso)
    int fillWidth = ((barWidth - 4) * percent) / 100;
    for (int x = 0; x < fillWidth; x++) {
        for (int y = 2; y < barHeight - 2; y++) {
            // Gradiente da verde a cyan
            uint8_t r = 0;
            uint8_t g = 255 - (x * 100 / barWidth);
            uint8_t b = (x * 255 / barWidth);
            display->drawPixelRGB888(barX + 2 + x, barY + y, r, g, b);
        }
    }
}

void DisplayManager::showOTASuccess() {
    if (!display) return;

    display->clearScreen();

    // ✅ Reset font al default
    display->setFont(nullptr);

    // Checkmark grande e centrato (16x16 circa)
    // Centro: x=32, y=20
    uint8_t green = 255;

    // Parte corta del checkmark (sinistra-basso)
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            display->drawPixelRGB888(24 + i, 28 + j, 0, green, 0);
            display->drawPixelRGB888(25 + i, 29 + j, 0, green, 0);
            display->drawPixelRGB888(26 + i, 30 + j, 0, green, 0);
        }
    }

    // Parte lunga del checkmark (centro-alto destra)
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            display->drawPixelRGB888(27 + i, 29 - j, 0, green, 0);
            display->drawPixelRGB888(28 + i, 28 - j, 0, green, 0);
            display->drawPixelRGB888(29 + i, 27 - j, 0, green, 0);
            display->drawPixelRGB888(30 + i, 26 - j, 0, green, 0);
            display->drawPixelRGB888(31 + i, 25 - j, 0, green, 0);
            display->drawPixelRGB888(32 + i, 24 - j, 0, green, 0);
            display->drawPixelRGB888(33 + i, 23 - j, 0, green, 0);
            display->drawPixelRGB888(34 + i, 22 - j, 0, green, 0);
            display->drawPixelRGB888(35 + i, 21 - j, 0, green, 0);
            display->drawPixelRGB888(36 + i, 20 - j, 0, green, 0);
        }
    }

    // Testo "OK!" sotto il checkmark
    display->setTextSize(2);
    display->setTextColor(color565(0, 255, 0)); // Verde
    display->setCursor(20, 45);
    display->print("OK!");
}
