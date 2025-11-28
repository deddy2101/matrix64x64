#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
//#include <ESPmDNS.h>
#include "Settings.h"
#include "Debug.h"


enum class WiFiState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED_STA,
    CONNECTED_AP
};

class WiFiManager {
private:
    Settings* settings;
    WiFiState state;
    unsigned long lastAttempt;
    int retryCount;
    
    static constexpr int MAX_RETRIES = 20;
    static constexpr unsigned long RETRY_INTERVAL = 500;
    
    // AP defaults
    static constexpr const char* AP_PASSWORD = "ledmatrix123";
    
    bool startAP();
    bool startSTA();
    bool startMDNS();
    
public:
    WiFiManager(Settings* settings);
    
    // Inizializzazione
    bool begin();
    
    // Update (per gestire riconnessione)
    void update();
    
    // Stato
    WiFiState getState() const { return state; }
    bool isConnected() const { return state == WiFiState::CONNECTED_STA || state == WiFiState::CONNECTED_AP; }
    bool isAPMode() const { return state == WiFiState::CONNECTED_AP; }
    
    // Info
    String getIP() const;
    String getSSID() const;
    int getRSSI() const;
    String getStatusString() const;
    
    // Azioni
    void reconnect();
    void switchToAP();
    void switchToSTA(const char* ssid, const char* password);
};

#endif
