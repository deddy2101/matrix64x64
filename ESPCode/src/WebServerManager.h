#ifndef WEBSERVER_MANAGER_H
#define WEBSERVER_MANAGER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "Settings.h"
#include "CommandHandler.h"

class WebServerManager {
private:
    AsyncWebServer server;
    Settings* settings;
    CommandHandler* commandHandler;
    
    // Setup routes
    void setupRoutes();
    void setupCORS();
    
public:
    WebServerManager(uint16_t port = 80);
    
    // Inizializzazione
    bool begin(Settings* settings, CommandHandler* cmdHandler);
    
    // Accesso al server (per WebSocket)
    AsyncWebServer* getServer() { return &server; }
};

#endif