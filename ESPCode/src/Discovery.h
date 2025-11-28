#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <Arduino.h>
#include <WiFiUdp.h>
#include "Settings.h"

// Porta per discovery UDP
#define DISCOVERY_PORT 5555
#define DISCOVERY_MAGIC "LEDMATRIX_DISCOVER"
#define DISCOVERY_RESPONSE "LEDMATRIX_HERE"

/**
 * DiscoveryService - Risponde a richieste di discovery UDP
 * 
 * L'app invia broadcast UDP sulla porta 5555 con "LEDMATRIX_DISCOVER"
 * L'ESP32 risponde con "LEDMATRIX_HERE,<nome>,<ip>,<porta>"
 */
class DiscoveryService {
public:
    DiscoveryService(Settings* settings, uint16_t servicePort = 80);
    
    void begin();
    void update();
    
private:
    WiFiUDP udp;
    Settings* settings;
    uint16_t servicePort;
    bool initialized;
    
    void handleDiscovery();
};

#endif // DISCOVERY_H