#ifndef IMAGE_MANAGER_H
#define IMAGE_MANAGER_H

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <vector>

// Formato immagine: raw RGB565 64x64 pixels
#define IMAGE_WIDTH 64
#define IMAGE_HEIGHT 64
#define IMAGE_SIZE (IMAGE_WIDTH * IMAGE_HEIGHT * 2)  // 2 bytes per pixel (RGB565)

struct ImageInfo {
    String name;
    String path;
    size_t size;
};

class ImageManager {
public:
    ImageManager();

    bool begin();

    // Upload immagine da base64
    bool uploadImage(const String& name, const String& base64Data);

    // Carica immagine in buffer RGB565
    bool loadImage(const String& name, uint16_t* buffer);

    // Lista immagini disponibili
    std::vector<ImageInfo> listImages();

    // Elimina immagine
    bool deleteImage(const String& name);

    // Info storage
    size_t getTotalSpace();
    size_t getUsedSpace();
    size_t getFreeSpace();

    // Verifica se immagine esiste
    bool exists(const String& name);

private:
    bool _initialized;

    String getImagePath(const String& name);
    size_t base64Decode(const String& input, uint8_t* output, size_t maxLen);
};

#endif // IMAGE_MANAGER_H
