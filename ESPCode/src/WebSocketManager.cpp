#include "WebSocketManager.h"

WebSocketManager::WebSocketManager()
    : _ws("/ws")
    , _cmdHandler(nullptr)
    , _messagesReceived(0)
    , _messagesSent(0)
    , _lastCleanup(0)
{}

void WebSocketManager::init(AsyncWebServer* server, CommandHandler* cmdHandler) {
    _cmdHandler = cmdHandler;
    
    _ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client,
                       AwsEventType type, void* arg, uint8_t* data, size_t len) {
        this->onEvent(server, client, type, arg, data, len);
    });
    
    server->addHandler(&_ws);
    
    Serial.println("[WS] WebSocket initialized on /ws");
}

void WebSocketManager::update() {
    // Cleanup ogni secondo
    uint32_t now = millis();
    if (now - _lastCleanup >= 1000) {
        _ws.cleanupClients();
        _lastCleanup = now;
    }
}

void WebSocketManager::cleanupClients() {
    _ws.cleanupClients();
}

void WebSocketManager::broadcast(const String& message) {
    if (_ws.count() > 0) {
        _ws.textAll(message);
        _messagesSent++;
    }
}

void WebSocketManager::notifyStatusChange() {
    if (_cmdHandler) {
        broadcast(_cmdHandler->getStatusResponse());
    }
}

void WebSocketManager::notifyEffectChange() {
    if (_cmdHandler) {
        broadcast(_cmdHandler->getEffectChangeNotification());
    }
}

void WebSocketManager::notifyTimeChange() {
    if (_cmdHandler) {
        broadcast(_cmdHandler->getTimeChangeNotification());
    }
}

void WebSocketManager::onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("[WS] Client #%u connected from %s\n", 
                         client->id(), client->remoteIP().toString().c_str());
            // Invia messaggio di benvenuto con stato
            client->text("WELCOME,LED Matrix Controller");
            if (_cmdHandler) {
                client->text(_cmdHandler->getStatusResponse());
            }
            break;
            
        case WS_EVT_DISCONNECT:
            Serial.printf("[WS] Client #%u disconnected\n", client->id());
            break;
            
        case WS_EVT_DATA:
            handleMessage(client, data, len);
            break;
            
        case WS_EVT_PONG:
            // Pong ricevuto
            break;
            
        case WS_EVT_ERROR:
            Serial.printf("[WS] Client #%u error\n", client->id());
            break;
    }
}

void WebSocketManager::handleMessage(AsyncWebSocketClient* client, uint8_t* data, size_t len) {
    _messagesReceived++;
    
    // Converti in stringa
    String message;
    message.reserve(len + 1);
    for (size_t i = 0; i < len; i++) {
        message += (char)data[i];
    }
    message.trim();
    
    Serial.printf("[WS] Received from #%u: %s\n", client->id(), message.c_str());
    
    if (_cmdHandler && !message.isEmpty()) {
        String response = _cmdHandler->processCommand(message);
        if (!response.isEmpty()) {
            client->text(response);
            _messagesSent++;
            Serial.printf("[WS] Response: %s\n", response.c_str());
        }
    }
}