#include "WebServerManager.h"

WebServerManager::WebServerManager(uint16_t port) 
    : server(port),
      settings(nullptr),
      commandHandler(nullptr) {
}

bool WebServerManager::begin(Settings* s, CommandHandler* cmdHandler) {
    settings = s;
    commandHandler = cmdHandler;
    
    setupCORS();
    setupRoutes();
    
    server.begin();
    Serial.println(F("[WebServer] ✓ Server started on port 80"));
    
    return true;
}

void WebServerManager::setupCORS() {
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
}

void WebServerManager::setupRoutes() {
    
    // GET / - info base
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/plain", "LED Matrix Controller - Use WebSocket at /ws");
    });
    
    // GET /api/status
    server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        String response = commandHandler->processJson("{\"cmd\":\"getStatus\"}");
        request->send(200, "application/json", response);
    });
    
    // GET /api/settings
    server.on("/api/settings", HTTP_GET, [this](AsyncWebServerRequest* request) {
        String response = commandHandler->processJson("{\"cmd\":\"getSettings\"}");
        request->send(200, "application/json", response);
    });
    
    // GET /api/effects
    server.on("/api/effects", HTTP_GET, [this](AsyncWebServerRequest* request) {
        String response = commandHandler->processJson("{\"cmd\":\"getEffects\"}");
        request->send(200, "application/json", response);
    });
    
    // OPTIONS handler per CORS preflight
    server.onNotFound([](AsyncWebServerRequest* request) {
        if (request->method() == HTTP_OPTIONS) {
            request->send(200);
        } else {
            request->send(404, "application/json", "{\"error\":\"Not found\"}");
        }
    });
}