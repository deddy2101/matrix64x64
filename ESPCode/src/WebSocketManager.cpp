#include "WebSocketManager.h"

// Istanza statica
WebSocketManager* WebSocketManager::instance = nullptr;

WebSocketManager::WebSocketManager() 
    : ws("/ws"),
      commandHandler(nullptr),
      messagesReceived(0),
      messagesSent(0),
      clientsConnected(0) {
    instance = this;
}

void WebSocketManager::begin(AsyncWebServer* server, CommandHandler* cmdHandler) {
    commandHandler = cmdHandler;
    
    // Registra callback
    ws.onEvent(onWebSocketEvent);
    
    // Aggiungi handler al server
    server->addHandler(&ws);
    
    // Imposta callback per risposte nel CommandHandler
    commandHandler->setResponseCallback([this](const String& response) {
        broadcast(response);
    });
    
    Serial.println(F("[WebSocket] Server initialized on /ws"));
}

void WebSocketManager::onWebSocketEvent(AsyncWebSocket* server,
                                         AsyncWebSocketClient* client,
                                         AwsEventType type,
                                         void* arg,
                                         uint8_t* data,
                                         size_t len) {
    if (!instance) return;
    
    switch (type) {
        case WS_EVT_CONNECT:
            instance->handleConnect(client);
            break;
            
        case WS_EVT_DISCONNECT:
            instance->handleDisconnect(client);
            break;
            
        case WS_EVT_DATA:
            instance->handleMessage(client, data, len);
            break;
            
        case WS_EVT_PONG:
            // Client ha risposto al ping
            break;
            
        case WS_EVT_ERROR:
            Serial.printf("[WebSocket] Error from client #%u\n", client->id());
            break;
    }
}

void WebSocketManager::handleConnect(AsyncWebSocketClient* client) {
    clientsConnected++;
    Serial.printf("[WebSocket] Client #%u connected from %s\n", 
                 client->id(), 
                 client->remoteIP().toString().c_str());
    
    // Invia messaggio di benvenuto con stato attuale
    JsonDocument welcome;
    welcome["type"] = "welcome";
    welcome["message"] = "Connected to LED Matrix Controller";
    welcome["clientId"] = client->id();
    
    String output;
    serializeJson(welcome, output);
    client->text(output);
    
    // Invia stato corrente
    if (commandHandler) {
        String status = commandHandler->processJson("{\"cmd\":\"getStatus\"}");
        client->text(status);
    }
}

void WebSocketManager::handleDisconnect(AsyncWebSocketClient* client) {
    if (clientsConnected > 0) clientsConnected--;
    Serial.printf("[WebSocket] Client #%u disconnected\n", client->id());
}

void WebSocketManager::handleMessage(AsyncWebSocketClient* client, uint8_t* data, size_t len) {
    messagesReceived++;
    
    // Null-terminate
    data[len] = 0;
    String message = String((char*)data);
    message.trim();
    
    Serial.printf("[WebSocket] Received from #%u: %s\n", client->id(), message.c_str());
    
    if (message.length() == 0) return;
    
    // Controlla se è JSON
    if (message.startsWith("{")) {
        // Processa come JSON
        if (commandHandler) {
            String response = commandHandler->processJson(message);
            client->text(response);
            messagesSent++;
        }
    } else {
        // Fallback: processa come comando seriale legacy
        if (commandHandler) {
            bool handled = commandHandler->processSerial(message);
            
            JsonDocument response;
            if (handled) {
                response["type"] = "ack";
                response["cmd"] = message;
                response["success"] = true;
            } else {
                response["type"] = "error";
                response["message"] = "Unknown command";
            }
            
            String output;
            serializeJson(response, output);
            client->text(output);
            messagesSent++;
        }
    }
}

void WebSocketManager::broadcast(const String& message) {
    ws.textAll(message);
    messagesSent++;
}

void WebSocketManager::send(uint32_t clientId, const String& message) {
    ws.text(clientId, message);
    messagesSent++;
}

void WebSocketManager::notifyStatusChange() {
    if (commandHandler && clientsConnected > 0) {
        String status = commandHandler->processJson("{\"cmd\":\"getStatus\"}");
        broadcast(status);
    }
}

void WebSocketManager::notifyEffectChange(const char* effectName, int index) {
    if (clientsConnected == 0) return;
    
    JsonDocument doc;
    doc["type"] = "effectChange";
    doc["name"] = effectName;
    doc["index"] = index;
    
    String output;
    serializeJson(doc, output);
    broadcast(output);
}

void WebSocketManager::notifyTimeChange(const String& time) {
    if (clientsConnected == 0) return;
    
    JsonDocument doc;
    doc["type"] = "timeUpdate";
    doc["time"] = time;
    
    String output;
    serializeJson(doc, output);
    broadcast(output);
}

void WebSocketManager::cleanupClients() {
    ws.cleanupClients();
}
