#include "WiFiManager.h"

WiFiManager::WiFiManager(Settings* settings) 
    : settings(settings), 
      state(WiFiState::DISCONNECTED),
      lastAttempt(0),
      retryCount(0) {
}

bool WiFiManager::begin() {
    DEBUG_PRINTLN(F("[WiFi] Initializing..."));
    
    // Prova prima la modalità configurata
    if (settings->isAPMode() || strlen(settings->getSSID()) == 0) {
        // Nessun SSID configurato o AP mode richiesto
        return startAP();
    } else {
        // Prova STA, fallback su AP
        if (startSTA()) {
            return true;
        } else {
            DEBUG_PRINTLN(F("[WiFi] STA failed, falling back to AP mode"));
            return startAP();
        }
    }
}

bool WiFiManager::startSTA() {
    DEBUG_PRINTF("[WiFi] Connecting to: %s\n", settings->getSSID());
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(settings->getSSID(), settings->getPassword());
    
    state = WiFiState::CONNECTING;
    retryCount = 0;
    
    while (WiFi.status() != WL_CONNECTED && retryCount < MAX_RETRIES) {
        delay(RETRY_INTERVAL);
        DEBUG_PRINT(".");
        retryCount++;
    }
    DEBUG_PRINTLN();
    
    if (WiFi.status() == WL_CONNECTED) {
        state = WiFiState::CONNECTED_STA;
        DEBUG_PRINTLN(F("[WiFi] ✓ Connected to WiFi"));
        DEBUG_PRINTF("[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());
        DEBUG_PRINTF("[WiFi] RSSI: %d dBm\n", WiFi.RSSI());
        
        return true;
    }
    
    DEBUG_PRINTLN(F("[WiFi] ✗ Connection failed"));
    state = WiFiState::DISCONNECTED;
    return false;
}

bool WiFiManager::startAP() {
    DEBUG_PRINTLN(F("[WiFi] Starting Access Point..."));
    
    WiFi.mode(WIFI_AP);
    
    // Configura IP statico per AP
    IPAddress local_IP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    
    WiFi.softAPConfig(local_IP, gateway, subnet);
    
    String apName = String(settings->getDeviceName());
    
    if (WiFi.softAP(apName.c_str(), AP_PASSWORD)) {
        state = WiFiState::CONNECTED_AP;
        DEBUG_PRINTLN(F("[WiFi] ✓ Access Point started"));
        DEBUG_PRINTF("[WiFi] SSID: %s\n", apName.c_str());
        DEBUG_PRINTF("[WiFi] Password: %s\n", AP_PASSWORD);
        DEBUG_PRINTF("[WiFi] IP: %s\n", WiFi.softAPIP().toString().c_str());
        
        return true;
    }
    
    DEBUG_PRINTLN(F("[WiFi] ✗ Failed to start AP"));
    state = WiFiState::DISCONNECTED;
    return false;
}



void WiFiManager::update() {
    // Controlla disconnessione in modalità STA
    if (state == WiFiState::CONNECTED_STA && WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN(F("[WiFi] Connection lost, reconnecting..."));
        state = WiFiState::DISCONNECTED;
        reconnect();
    }
}

void WiFiManager::reconnect() {
    if (state == WiFiState::CONNECTED_AP) {
        // Già in AP mode, non fare nulla
        return;
    }
    
    if (!settings->isAPMode() && strlen(settings->getSSID()) > 0) {
        if (!startSTA()) {
            startAP();
        }
    } else {
        startAP();
    }
}

void WiFiManager::switchToAP() {
    WiFi.disconnect();
    settings->setAPMode(true);
    startAP();
}

void WiFiManager::switchToSTA(const char* ssid, const char* password) {
    WiFi.disconnect();
    settings->setSSID(ssid);
    settings->setPassword(password);
    settings->setAPMode(false);
    
    if (!startSTA()) {
        DEBUG_PRINTLN(F("[WiFi] STA failed, reverting to AP"));
        startAP();
    }
}

String WiFiManager::getIP() const {
    if (state == WiFiState::CONNECTED_AP) {
        return WiFi.softAPIP().toString();
    } else if (state == WiFiState::CONNECTED_STA) {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}

String WiFiManager::getSSID() const {
    if (state == WiFiState::CONNECTED_AP) {
        return settings->getDeviceName();
    } else if (state == WiFiState::CONNECTED_STA) {
        return WiFi.SSID();
    }
    return "";
}

int WiFiManager::getRSSI() const {
    if (state == WiFiState::CONNECTED_STA) {
        return WiFi.RSSI();
    }
    return 0;
}

String WiFiManager::getStatusString() const {
    switch (state) {
        case WiFiState::DISCONNECTED: return "Disconnected";
        case WiFiState::CONNECTING: return "Connecting...";
        case WiFiState::CONNECTED_STA: return "Connected (STA)";
        case WiFiState::CONNECTED_AP: return "Access Point";
        default: return "Unknown";
    }
}
