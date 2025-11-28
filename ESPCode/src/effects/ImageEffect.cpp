#include "ImageEffect.h"

ImageEffect::ImageEffect(DisplayManager* dm, DrawImageFunction drawFunc, const String& name)
    : Effect(dm),
      spriteRenderer(new SpriteRenderer(dm)),
      drawFunction(drawFunc),
      imageName(name),
      imageDrawn(false) {
}

ImageEffect::~ImageEffect() {
    delete spriteRenderer;
}

void ImageEffect::init() {
    DEBUG_PRINTF("Initializing Image Effect: %s\n", imageName.c_str());
    
    displayManager->fillScreen(0, 0, 0);
    imageDrawn = false;
}

void ImageEffect::update() {
    // Le immagini sono statiche, non c'Ã¨ update
}

void ImageEffect::draw() {
    // Disegna l'immagine solo una volta
    if (!imageDrawn && drawFunction) {
        // Chiama la funzione draw_bosco, draw_casa, etc.
        // passando il puntatore al display
        drawFunction((void*)displayManager->getDisplay(), 0, 0);
        imageDrawn = true;
    }
}

const char* ImageEffect::getName() {
    static char nameBuffer[32];
    snprintf(nameBuffer, sizeof(nameBuffer), "Image: %s", imageName.c_str());
    return nameBuffer;
}