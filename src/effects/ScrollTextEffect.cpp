#include "ScrollTextEffect.h"

ScrollTextEffect::ScrollTextEffect(DisplayManager* dm, const String& scrollText,
                                 uint8_t size, uint16_t color)
    : Effect(dm),
      text(scrollText),
      scrollX(0),
      scrollSpeed(1),
      textWidth(0),
      textHeight(0),
      textColor(color),
      textSize(size),
      completed(false) {
}

void ScrollTextEffect::init() {
    Serial.printf("[ScrollTextEffect] Initializing: \"%s\"\n", text.c_str());
    
    scrollX = displayManager->getWidth();
    completed = false;
    
    calculateTextWidth();
    
    // IMPORTANTE: Reset al font di default
    displayManager->setFont(nullptr);  // nullptr = font default di Adafruit GFX
    displayManager->setTextSize(textSize);
    displayManager->setTextWrap(false);
    displayManager->setTextColor(textColor);
    
    displayManager->fillScreen(0, 0, 0);
}

void ScrollTextEffect::update() {
    scrollX -= scrollSpeed;
    
    // Controlla se il testo è completamente uscito dallo schermo
    if (scrollX <= -textWidth) {
        completed = true;
        Serial.println("[ScrollTextEffect] Completed!");
    }
}

void ScrollTextEffect::draw() {
    if (!completed) {
        // Cancella schermo
        displayManager->fillScreen(0, 0, 0);
        
        // IMPORTANTE: Reset al font default + configura testo
        displayManager->setFont(nullptr);  // Font default
        displayManager->setTextSize(textSize);
        displayManager->setTextWrap(false);
        displayManager->setTextColor(textColor);
        
        // Calcola posizione Y centrata
        int height = displayManager->getHeight();
        int yPos = (height / 2) - (textHeight / 2);
        
        // Disegna testo
        displayManager->setCursor(scrollX, yPos);
        displayManager->print(text);
    }
}

void ScrollTextEffect::calculateTextWidth() {
    // Stima approssimativa della larghezza
    // Per Arial/font default: circa 6 pixel per carattere * textSize
    // Per dimensione 3: circa 19 pixel per carattere
    int charWidth = (textSize == 1) ? 6 : (textSize == 2) ? 12 : 19;
    textWidth = text.length() * charWidth;
    textHeight = 8 * textSize;  // Altezza standard
}

void ScrollTextEffect::setText(const String& newText) {
    text = newText;
    calculateTextWidth();
}