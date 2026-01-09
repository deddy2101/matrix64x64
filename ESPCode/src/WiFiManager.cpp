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

String WiFiManager::scanNetworks() {
    DEBUG_PRINTLN(F("[WiFi] Scanning networks (async)..."));

    // Avvia scan asincrono se non già in corso
    int16_t scanResult = WiFi.scanComplete();

    if (scanResult == WIFI_SCAN_RUNNING) {
        // Scan già in corso, ritorna stato
        DEBUG_PRINTLN(F("[WiFi] Scan already running"));
        return "WIFI_SCAN_RUNNING";
    }

    if (scanResult == WIFI_SCAN_FAILED) {
        // Avvia nuovo scan asincrono
        DEBUG_PRINTLN(F("[WiFi] Starting async scan..."));
        WiFi.scanNetworks(true);  // true = async
        return "WIFI_SCAN_STARTED";
    }

    if (scanResult == 0) {
        // Nessuna rete trovata, riavvia scan
        DEBUG_PRINTLN(F("[WiFi] No networks, restarting scan..."));
        WiFi.scanDelete();
        WiFi.scanNetworks(true);
        return "WIFI_SCAN_STARTED";
    }

    if (scanResult > 0) {
        // Scan completato, costruisci risposta
        DEBUG_PRINTF("[WiFi] Found %d networks\n", scanResult);

        String response = "WIFI_SCAN," + String(scanResult);

        for (int i = 0; i < scanResult; i++) {
            String ssid = WiFi.SSID(i);
            int32_t rssi = WiFi.RSSI(i);
            bool secured = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);

            // Escape virgole nell'SSID
            ssid.replace(",", "_");

            // Salta reti senza nome
            if (ssid.length() == 0) {
                continue;
            }

            response += "," + ssid;
            response += "," + String(rssi);
            response += "," + String(secured ? 1 : 0);

            DEBUG_PRINTF("[WiFi]   %d: %s (%d dBm) %s\n",
                         i, ssid.c_str(), rssi, secured ? "secured" : "open");
        }

        // Libera memoria e prepara per prossimo scan
        WiFi.scanDelete();

        return response;
    }

    // Stato sconosciuto, avvia scan
    WiFi.scanNetworks(true);
    return "WIFI_SCAN_STARTED";
}
