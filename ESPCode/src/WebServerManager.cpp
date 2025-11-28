#include "WebServerManager.h"

WebServerManager::WebServerManager(uint16_t port)
    : _server(port)
    , _cmdHandler(nullptr)
{}

void WebServerManager::init(CommandHandler* cmdHandler) {
    _cmdHandler = cmdHandler;
    setupRoutes();
    _server.begin();
    DEBUG_PRINTLN("[HTTP] Web server started");
}

void WebServerManager::setupRoutes() {
    // CORS headers
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
    
    // Root - Info
    _server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/plain", 
            "LED Matrix Controller\n"
            "---------------------\n"
            "WebSocket: ws://<ip>/ws\n"
            "API: /api/status, /api/effects, /api/settings\n"
            "\n"
            "Protocol: CSV-based commands\n"
            "Example: getStatus, effect,next, brightness,200\n"
        );
    });
    
    // API - Status
    _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (_cmdHandler) {
            request->send(200, "text/plain", _cmdHandler->getStatusResponse());
        } else {
            request->send(500, "text/plain", "ERR,not initialized");
        }
    });
    
    // API - Effects list
    _server.on("/api/effects", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (_cmdHandler) {
            request->send(200, "text/plain", _cmdHandler->getEffectsResponse());
        } else {
            request->send(500, "text/plain", "ERR,not initialized");
        }
    });
    
    // API - Settings
    _server.on("/api/settings", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (_cmdHandler) {
            request->send(200, "text/plain", _cmdHandler->getSettingsResponse());
        } else {
            request->send(500, "text/plain", "ERR,not initialized");
        }
    });
    
    // API - Command (POST)
    _server.on("/api/cmd", HTTP_POST, [](AsyncWebServerRequest* request) {
        request->send(400, "text/plain", "ERR,use body");
    }, NULL, [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
        if (_cmdHandler) {
            String cmd;
            cmd.reserve(len + 1);
            for (size_t i = 0; i < len; i++) {
                cmd += (char)data[i];
            }
            String response = _cmdHandler->processCommand(cmd);
            request->send(200, "text/plain", response);
        } else {
            request->send(500, "text/plain", "ERR,not initialized");
        }
    });
    
    // API - Command (GET with query param)
    _server.on("/api/cmd", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (request->hasParam("c")) {
            String cmd = request->getParam("c")->value();
            if (_cmdHandler) {
                String response = _cmdHandler->processCommand(cmd);
                request->send(200, "text/plain", response);
            } else {
                request->send(500, "text/plain", "ERR,not initialized");
            }
        } else {
            request->send(400, "text/plain", "ERR,missing param c");
        }
    });
    
    // 404
    _server.onNotFound([](AsyncWebServerRequest* request) {
        request->send(404, "text/plain", "ERR,not found");
    });
}