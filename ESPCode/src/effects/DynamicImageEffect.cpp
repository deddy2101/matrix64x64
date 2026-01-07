#include "DynamicImageEffect.h"
#include "../Debug.h"

DynamicImageEffect::DynamicImageEffect(DisplayManager* dm, ImageManager* imgMgr, unsigned long displayDuration)
    : Effect(dm)
    , _imageManager(imgMgr)
    , _imageBuffer(nullptr)
    , _currentIndex(-1)
    , _currentImageName("")
    , _imageLoaded(false)
    , _needsRedraw(true)
    , _autoSlideshow(true)
    , _displayDuration(displayDuration)
    , _lastChangeTime(0)
{
    // Alloca buffer per immagine 64x64 RGB565
    _imageBuffer = (uint16_t*)malloc(IMAGE_SIZE);
    if (!_imageBuffer) {
        DEBUG_PRINTLN(F("[DynamicImageEffect] Failed to allocate image buffer!"));
    }
}

DynamicImageEffect::~DynamicImageEffect() {
    if (_imageBuffer) {
        free(_imageBuffer);
    }
}

void DynamicImageEffect::init() {
    DEBUG_PRINTLN(F("[DynamicImageEffect] Initializing..."));

    displayManager->fillScreen(0, 0, 0);

    // Carica lista immagini disponibili
    loadImageList();

    if (_imageList.empty()) {
        DEBUG_PRINTLN(F("[DynamicImageEffect] No images found"));
        // Schermo nero - nessuna immagine disponibile
        _imageLoaded = false;
        return;
    }

    DEBUG_PRINTF("[DynamicImageEffect] Found %d images\n", _imageList.size());

    // Carica prima immagine
    _currentIndex = 0;
    _imageLoaded = loadCurrentImage();
    _needsRedraw = true;
    _lastChangeTime = millis();
}

void DynamicImageEffect::update() {
    if (!_autoSlideshow || _imageList.empty()) {
        return;
    }

    // Slideshow automatico
    unsigned long now = millis();
    if (now - _lastChangeTime >= _displayDuration) {
        nextImage();
        _lastChangeTime = now;
    }
}

void DynamicImageEffect::draw() {
    if (!_needsRedraw) {
        return;
    }

    if (!_imageLoaded || !_imageBuffer) {
        // Nessuna immagine caricata - schermo nero
        displayManager->fillScreen(0, 0, 0);
        _needsRedraw = false;
        return;
    }

    drawCurrentImage();
    _needsRedraw = false;
}

const char* DynamicImageEffect::getName() {
    static char nameBuffer[64];
    if (_imageList.empty()) {
        snprintf(nameBuffer, sizeof(nameBuffer), "Images: Empty");
    } else if (_currentImageName.length() > 0) {
        snprintf(nameBuffer, sizeof(nameBuffer), "Images: %s (%d/%d)",
                 _currentImageName.c_str(),
                 _currentIndex + 1,
                 _imageList.size());
    } else {
        snprintf(nameBuffer, sizeof(nameBuffer), "Images: %d available", _imageList.size());
    }
    return nameBuffer;
}

// ═══════════════════════════════════════════
// Controllo Manuale
// ═══════════════════════════════════════════

void DynamicImageEffect::showImage(const String& name) {
    if (!_imageManager) return;

    DEBUG_PRINTF("[DynamicImageEffect] Showing image: %s\n", name.c_str());

    // Cerca immagine nella lista
    for (int i = 0; i < _imageList.size(); i++) {
        if (_imageList[i].name == name) {
            _currentIndex = i;
            _imageLoaded = loadCurrentImage();
            _needsRedraw = true;
            _lastChangeTime = millis();
            return;
        }
    }

    DEBUG_PRINTF("[DynamicImageEffect] Image not found: %s\n", name.c_str());
}

void DynamicImageEffect::nextImage() {
    if (_imageList.empty()) return;

    _currentIndex = (_currentIndex + 1) % _imageList.size();
    _imageLoaded = loadCurrentImage();
    _needsRedraw = true;
    _lastChangeTime = millis();

    DEBUG_PRINTF("[DynamicImageEffect] Next image: %s (%d/%d)\n",
                 _currentImageName.c_str(),
                 _currentIndex + 1,
                 _imageList.size());
}

void DynamicImageEffect::previousImage() {
    if (_imageList.empty()) return;

    _currentIndex--;
    if (_currentIndex < 0) {
        _currentIndex = _imageList.size() - 1;
    }

    _imageLoaded = loadCurrentImage();
    _needsRedraw = true;
    _lastChangeTime = millis();

    DEBUG_PRINTF("[DynamicImageEffect] Previous image: %s (%d/%d)\n",
                 _currentImageName.c_str(),
                 _currentIndex + 1,
                 _imageList.size());
}

void DynamicImageEffect::setAutoSlideshow(bool enabled) {
    _autoSlideshow = enabled;
    if (enabled) {
        _lastChangeTime = millis();
    }
    DEBUG_PRINTF("[DynamicImageEffect] Auto slideshow: %s\n", enabled ? "ON" : "OFF");
}

void DynamicImageEffect::setDisplayDuration(unsigned long ms) {
    _displayDuration = ms;
    DEBUG_PRINTF("[DynamicImageEffect] Display duration: %lu ms\n", ms);
}

// ═══════════════════════════════════════════
// Helper Privati
// ═══════════════════════════════════════════

void DynamicImageEffect::loadImageList() {
    if (!_imageManager) {
        DEBUG_PRINTLN(F("[DynamicImageEffect] ImageManager not available"));
        return;
    }

    _imageList = _imageManager->listImages();
    DEBUG_PRINTF("[DynamicImageEffect] Loaded %d images from storage\n", _imageList.size());

    for (const auto& img : _imageList) {
        DEBUG_PRINTF("  - %s (%u bytes)\n", img.name.c_str(), img.size);
    }
}

bool DynamicImageEffect::loadCurrentImage() {
    if (!_imageManager || !_imageBuffer) {
        return false;
    }

    if (_currentIndex < 0 || _currentIndex >= _imageList.size()) {
        return false;
    }

    const ImageInfo& img = _imageList[_currentIndex];
    _currentImageName = img.name;

    DEBUG_PRINTF("[DynamicImageEffect] Loading image: %s\n", img.name.c_str());

    // Carica immagine da LittleFS
    bool success = _imageManager->loadImage(img.name, _imageBuffer);

    if (!success) {
        DEBUG_PRINTF("[DynamicImageEffect] Failed to load image: %s\n", img.name.c_str());
        _currentImageName = "";
        return false;
    }

    DEBUG_PRINTF("[DynamicImageEffect] Successfully loaded: %s\n", img.name.c_str());
    return true;
}

void DynamicImageEffect::drawCurrentImage() {
    if (!_imageBuffer || !displayManager) {
        return;
    }

    auto* display = displayManager->getDisplay();
    if (!display) {
        return;
    }

    // Disegna immagine pixel per pixel
    // Il buffer è RGB565 little-endian
    for (int y = 0; y < IMAGE_HEIGHT; y++) {
        for (int x = 0; x < IMAGE_WIDTH; x++) {
            int index = y * IMAGE_WIDTH + x;
            uint16_t rgb565 = _imageBuffer[index];

            // Converti RGB565 → RGB888 per drawPixelRGB888
            uint8_t r = ((rgb565 >> 11) & 0x1F) << 3;  // 5 bit → 8 bit
            uint8_t g = ((rgb565 >> 5) & 0x3F) << 2;   // 6 bit → 8 bit
            uint8_t b = (rgb565 & 0x1F) << 3;          // 5 bit → 8 bit

            display->drawPixelRGB888(x, y, r, g, b);
        }
    }
}
