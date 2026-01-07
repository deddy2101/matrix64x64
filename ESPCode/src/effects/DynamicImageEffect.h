#ifndef DYNAMIC_IMAGE_EFFECT_H
#define DYNAMIC_IMAGE_EFFECT_H

#include "../Effect.h"
#include "../ImageManager.h"

/**
 * DynamicImageEffect - Slideshow di immagini caricate su LittleFS
 *
 * Carica e mostra immagini dinamiche salvate dall'app.
 * Supporta cambio manuale o slideshow automatico.
 */
class DynamicImageEffect : public Effect {
public:
    DynamicImageEffect(DisplayManager* dm, ImageManager* imgMgr, unsigned long displayDuration = 5000);
    ~DynamicImageEffect();

    void init() override;
    void update() override;
    void draw() override;
    const char* getName() override;

    // Controllo manuale
    void showImage(const String& name);      // Mostra immagine specifica
    void nextImage();                         // Prossima immagine
    void previousImage();                     // Immagine precedente
    void setAutoSlideshow(bool enabled);      // Abilita/disabilita slideshow
    void setDisplayDuration(unsigned long ms); // Durata visualizzazione per immagine

    // Info
    String getCurrentImageName() const { return _currentImageName; }
    int getCurrentIndex() const { return _currentIndex; }
    int getImageCount() const { return _imageList.size(); }

private:
    ImageManager* _imageManager;
    uint16_t* _imageBuffer;              // Buffer RGB565 per immagine corrente
    std::vector<ImageInfo> _imageList;   // Lista immagini disponibili

    int _currentIndex;
    String _currentImageName;
    bool _imageLoaded;
    bool _needsRedraw;

    // Slideshow
    bool _autoSlideshow;
    unsigned long _displayDuration;
    unsigned long _lastChangeTime;

    // Helper
    void loadImageList();
    bool loadCurrentImage();
    void drawCurrentImage();
};

#endif // DYNAMIC_IMAGE_EFFECT_H
