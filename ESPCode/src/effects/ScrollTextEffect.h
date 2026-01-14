#ifndef SCROLL_TEXT_EFFECT_H
#define SCROLL_TEXT_EFFECT_H

#include "../Effect.h"

class ScrollTextEffect : public Effect {
private:
    String text;
    int scrollX;
    int scrollSpeed;
    int textWidth;
    int textHeight;
    uint16_t textColor;
    uint8_t textSize;
    bool completed;

    int loopCount;           // Numero di loop da eseguire (0 = infinito)
    int currentLoop;         // Loop corrente

    void calculateTextWidth();
    void resetScroll();

public:
    ScrollTextEffect(DisplayManager* dm, const String& scrollText,
                    uint8_t size = 3, uint16_t color = 0xFFE0);

    void init() override;
    void update() override;
    void draw() override;
    bool isComplete() override { return completed; }
    const char* getName() override { return "Scroll Text"; }

    // Configurazione
    void setText(const String& newText);
    void setSpeed(int speed) { scrollSpeed = speed; }
    void setColor(uint16_t color) { textColor = color; }
    void setSize(uint8_t size) { textSize = size; }
    void setLoopCount(int count) { loopCount = count; }  // 0 = infinito
    int getLoopCount() const { return loopCount; }
};

#endif
