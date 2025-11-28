#ifndef WEBSERVER_MANAGER_H
#define WEBSERVER_MANAGER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "CommandHandler.h"

class WebServerManager {
public:
    WebServerManager(uint16_t port = 80);
    
    void init(CommandHandler* cmdHandler);
    AsyncWebServer* getServer() { return &_server; }

private:
    AsyncWebServer _server;
    CommandHandler* _cmdHandler;
    
    void setupRoutes();
};

#endif // WEBSERVER_MANAGER_H