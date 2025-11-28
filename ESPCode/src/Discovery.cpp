#include "Discovery.h"
#include <WiFi.h>


DiscoveryService::DiscoveryService(Settings* settings, uint16_t servicePort)
    : settings(settings)
    , servicePort(servicePort)
    , initialized(false)
{}

void DiscoveryService::begin() {
    if (udp.begin(DISCOVERY_PORT)) {
        initialized = true;
        DEBUG_PRINTF("[Discovery] Listening on UDP port %d\n", DISCOVERY_PORT);
    } else {
        DEBUG_PRINTLN("[Discovery] Failed to start UDP");
    }
}

void DiscoveryService::update() {
    if (!initialized) return;
    
    int packetSize = udp.parsePacket();
    if (packetSize > 0) {
        handleDiscovery();
    }
}

void DiscoveryService::handleDiscovery() {
    char buffer[64];
    int len = udp.read(buffer, sizeof(buffer) - 1);
    if (len <= 0) return;
    
    buffer[len] = '\0';
    
    // Verifica magic string
    if (strncmp(buffer, DISCOVERY_MAGIC, strlen(DISCOVERY_MAGIC)) == 0) {
        DEBUG_PRINTF("[Discovery] Request from %s:%d\n", 
                     udp.remoteIP().toString().c_str(), 
                     udp.remotePort());
        
        // Prepara risposta: LEDMATRIX_HERE,nome,ip,porta
        // Determina IP corretto (STA o AP)
        IPAddress localIP;
        if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
            localIP = WiFi.softAPIP();  // Modalità AP
        } else {
            localIP = WiFi.localIP();   // Modalità STA
        }
        
        String response = String(DISCOVERY_RESPONSE) + "," +
                         settings->getDeviceName() + "," +
                         localIP.toString() + "," +
                         String(servicePort);
        
        // Rispondi al mittente
        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        udp.print(response);
        udp.endPacket();
        
        DEBUG_PRINTF("[Discovery] Response: %s\n", response.c_str());
    }
}