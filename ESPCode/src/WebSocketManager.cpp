#include "WebSocketManager.h"

WebSocketManager::WebSocketManager()
    : _ws("/ws")
    , _cmdHandler(nullptr)
    , _messagesReceived(0)
    , _messagesSent(0)
    , _lastCleanup(0)
    , _fragmentBuffer("")
    , _fragmentClientId(0)
{}

void WebSocketManager::init(AsyncWebServer* server, CommandHandler* cmdHandler) {
    _cmdHandler = cmdHandler;
    
    _ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client,
                       AwsEventType type, void* arg, uint8_t* data, size_t len) {
        this->onEvent(server, client, type, arg, data, len);
    });
    
    server->addHandler(&_ws);
    
    DEBUG_PRINTLN("[WS] WebSocket initialized on /ws");
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
            DEBUG_PRINTF("[WS] Client #%u connected from %s\n", 
                         client->id(), client->remoteIP().toString().c_str());
            // Invia messaggio di benvenuto con stato
            client->text("WELCOME,LED Matrix Controller");
            if (_cmdHandler) {
                client->text(_cmdHandler->getStatusResponse());
            }
            break;
            
        case WS_EVT_DISCONNECT:
            DEBUG_PRINTF("[WS] Client #%u disconnected\n", client->id());
            break;
            
        case WS_EVT_DATA: {
            AwsFrameInfo* info = (AwsFrameInfo*)arg;

            // Messaggio completo in un singolo frame
            if (info->final && info->index == 0 && info->len == len) {
                handleMessage(client, data, len);
            }
            // Messaggio frammentato - riassembla
            else {
                handleFragmentedMessage(client, info, data, len);
            }
            break;
        }
            
        case WS_EVT_PONG:
            // Pong ricevuto
            break;
            
        case WS_EVT_ERROR:
            DEBUG_PRINTF("[WS] Client #%u error\n", client->id());
            break;
    }
}

void WebSocketManager::handleFragmentedMessage(AsyncWebSocketClient* client, AwsFrameInfo* info, uint8_t* data, size_t len) {
    // Primo frame - inizializza buffer
    if (info->index == 0) {
        _fragmentBuffer = "";
        _fragmentBuffer.reserve(info->len);
        _fragmentClientId = client->id();
        DEBUG_PRINTF("[WS] Starting fragmented message from #%u: total=%u bytes\n",
                     client->id(), info->len);
    }

    // Verifica che sia lo stesso client
    if (_fragmentClientId != client->id()) {
        DEBUG_PRINTF("[WS] Fragment from wrong client! Expected #%u, got #%u\n",
                     _fragmentClientId, client->id());
        return;
    }

    // Aggiungi frammento al buffer
    for (size_t i = 0; i < len; i++) {
        _fragmentBuffer += (char)data[i];
    }

    // Ultimo frame - processa messaggio completo
    if (info->final && (info->index + len) == info->len) {
        DEBUG_PRINTF("[WS] Fragmented message complete: %u bytes\n", _fragmentBuffer.length());

        String message = _fragmentBuffer;
        _fragmentBuffer = "";
        _fragmentClientId = 0;

        // Processa come messaggio normale
        message.trim();
        _messagesReceived++;

        // Non stampare i dati OTA completi (troppo grandi)
        if (message.startsWith("ota,data,")) {
            int firstComma = message.indexOf(',');
            int secondComma = message.indexOf(',', firstComma + 1);
            String preview = message.substring(0, secondComma + 1) + "... [" + String(message.length()) + " bytes]";
            DEBUG_PRINTF("[WS] Received from #%u: %s\n", client->id(), preview.c_str());
        } else {
            DEBUG_PRINTF("[WS] Received from #%u: %s\n", client->id(), message.c_str());
        }

        if (_cmdHandler && !message.isEmpty()) {
            String response = _cmdHandler->processCommand(message);
            if (!response.isEmpty()) {
                client->text(response);
                _messagesSent++;
                DEBUG_PRINTF("[WS] Response: %s\n", response.c_str());
            }
        }
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

    // Non stampare i dati OTA completi (troppo grandi)
    if (message.startsWith("ota,data,")) {
        int firstComma = message.indexOf(',');
        int secondComma = message.indexOf(',', firstComma + 1);
        String preview = message.substring(0, secondComma + 1) + "... [" + String(len) + " bytes]";
        DEBUG_PRINTF("[WS] Received from #%u: %s\n", client->id(), preview.c_str());
    } else {
        DEBUG_PRINTF("[WS] Received from #%u: %s\n", client->id(), message.c_str());
    }

    if (_cmdHandler && !message.isEmpty()) {
        String response = _cmdHandler->processCommand(message);
        if (!response.isEmpty()) {
            client->text(response);
            _messagesSent++;
            DEBUG_PRINTF("[WS] Response: %s\n", response.c_str());
        }
    }
}