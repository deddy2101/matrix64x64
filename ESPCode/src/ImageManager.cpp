#include "ImageManager.h"
#include "Debug.h"

// Helper base64 decode (stesso di CommandHandler)
static const char base64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int base64CharIndex(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

ImageManager::ImageManager() : _initialized(false) {}

bool ImageManager::begin() {
    if (!LittleFS.begin(true)) {  // format on fail
        DEBUG_PRINTLN(F("[ImageMgr] Failed to mount LittleFS"));
        return false;
    }

    _initialized = true;
    DEBUG_PRINTF("[ImageMgr] Initialized. Total: %u KB, Used: %u KB, Free: %u KB\n",
                 getTotalSpace() / 1024,
                 getUsedSpace() / 1024,
                 getFreeSpace() / 1024);

    return true;
}

String ImageManager::getImagePath(const String& name) {
    return "/images/" + name + ".img";
}

bool ImageManager::uploadImage(const String& name, const String& base64Data) {
    if (!_initialized) {
        DEBUG_PRINTLN(F("[ImageMgr] Not initialized"));
        return false;
    }

    // Verifica dimensione base64 (~8KB per 64x64 RGB565)
    if (base64Data.length() < 8000 || base64Data.length() > 12000) {
        DEBUG_PRINTF("[ImageMgr] Invalid base64 size: %d\n", base64Data.length());
        return false;
    }

    // Decodifica base64
    uint8_t* imageData = (uint8_t*)malloc(IMAGE_SIZE);
    if (!imageData) {
        DEBUG_PRINTLN(F("[ImageMgr] Failed to allocate buffer"));
        return false;
    }

    size_t decodedSize = base64Decode(base64Data, imageData, IMAGE_SIZE);
    if (decodedSize != IMAGE_SIZE) {
        DEBUG_PRINTF("[ImageMgr] Decode failed: expected %d, got %d\n", IMAGE_SIZE, decodedSize);
        free(imageData);
        return false;
    }

    // Crea directory se non esiste
    if (!LittleFS.exists("/images")) {
        LittleFS.mkdir("/images");
    }

    // Salva file
    String path = getImagePath(name);
    File file = LittleFS.open(path, "w");
    if (!file) {
        DEBUG_PRINTF("[ImageMgr] Failed to create file: %s\n", path.c_str());
        free(imageData);
        return false;
    }

    size_t written = file.write(imageData, IMAGE_SIZE);
    file.close();
    free(imageData);

    if (written != IMAGE_SIZE) {
        DEBUG_PRINTF("[ImageMgr] Write failed: %d/%d bytes\n", written, IMAGE_SIZE);
        LittleFS.remove(path);
        return false;
    }

    DEBUG_PRINTF("[ImageMgr] Image '%s' uploaded successfully (%d bytes)\n", name.c_str(), IMAGE_SIZE);
    return true;
}

bool ImageManager::loadImage(const String& name, uint16_t* buffer) {
    if (!_initialized || !buffer) {
        return false;
    }

    String path = getImagePath(name);
    if (!LittleFS.exists(path)) {
        DEBUG_PRINTF("[ImageMgr] Image not found: %s\n", path.c_str());
        return false;
    }

    File file = LittleFS.open(path, "r");
    if (!file) {
        DEBUG_PRINTF("[ImageMgr] Failed to open: %s\n", path.c_str());
        return false;
    }

    size_t read = file.read((uint8_t*)buffer, IMAGE_SIZE);
    file.close();

    if (read != IMAGE_SIZE) {
        DEBUG_PRINTF("[ImageMgr] Read failed: %d/%d bytes\n", read, IMAGE_SIZE);
        return false;
    }

    return true;
}

std::vector<ImageInfo> ImageManager::listImages() {
    std::vector<ImageInfo> images;

    if (!_initialized) {
        return images;
    }

    File root = LittleFS.open("/images");
    if (!root || !root.isDirectory()) {
        return images;
    }

    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory() && String(file.name()).endsWith(".img")) {
            ImageInfo info;
            info.path = file.name();
            info.name = String(file.name());
            info.name.replace("/images/", "");
            info.name.replace(".img", "");
            info.size = file.size();
            images.push_back(info);
        }
        file = root.openNextFile();
    }

    return images;
}

bool ImageManager::deleteImage(const String& name) {
    if (!_initialized) {
        return false;
    }

    String path = getImagePath(name);
    if (LittleFS.remove(path)) {
        DEBUG_PRINTF("[ImageMgr] Deleted: %s\n", name.c_str());
        return true;
    }

    return false;
}

size_t ImageManager::getTotalSpace() {
    return LittleFS.totalBytes();
}

size_t ImageManager::getUsedSpace() {
    return LittleFS.usedBytes();
}

size_t ImageManager::getFreeSpace() {
    return getTotalSpace() - getUsedSpace();
}

bool ImageManager::exists(const String& name) {
    if (!_initialized) {
        return false;
    }
    return LittleFS.exists(getImagePath(name));
}

// Base64 decode
size_t ImageManager::base64Decode(const String& input, uint8_t* output, size_t maxLen) {
    size_t inputLen = input.length();
    if (inputLen == 0 || inputLen % 4 != 0) {
        return 0;
    }

    size_t outputLen = (inputLen / 4) * 3;
    if (input[inputLen - 1] == '=') outputLen--;
    if (input[inputLen - 2] == '=') outputLen--;

    if (outputLen > maxLen) {
        return 0;
    }

    size_t j = 0;
    for (size_t i = 0; i < inputLen; i += 4) {
        int a = base64CharIndex(input[i]);
        int b = base64CharIndex(input[i + 1]);
        int c = (input[i + 2] != '=') ? base64CharIndex(input[i + 2]) : 0;
        int d = (input[i + 3] != '=') ? base64CharIndex(input[i + 3]) : 0;

        if (a < 0 || b < 0) return 0;

        uint32_t triple = (a << 18) | (b << 12) | (c << 6) | d;

        if (j < outputLen) output[j++] = (triple >> 16) & 0xFF;
        if (j < outputLen) output[j++] = (triple >> 8) & 0xFF;
        if (j < outputLen) output[j++] = triple & 0xFF;
    }

    return outputLen;
}
