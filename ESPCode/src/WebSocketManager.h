#ifndef WEBSOCKET_MANAGER_H
#define WEBSOCKET_MANAGER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "CommandHandler.h"
#include "Debug.h"


class WebSocketManager {
public:
    WebSocketManager();
    
    void init(AsyncWebServer* server, CommandHandler* cmdHandler);
    void update();
    void cleanupClients();
    
    // Broadcast a tutti i client
    void broadcast(const String& message);
    
    // Notifiche specifiche
    void notifyStatusChange();
    void notifyEffectChange();
    void notifyTimeChange();
    
    // Stats
    uint32_t getClientsConnected() const { return _ws.count(); }
    uint32_t getMessagesReceived() const { return _messagesReceived; }
    uint32_t getMessagesSent() const { return _messagesSent; }

private:
    AsyncWebSocket _ws;
    CommandHandler* _cmdHandler;

    uint32_t _messagesReceived;
    uint32_t _messagesSent;
    uint32_t _lastCleanup;

    // Buffer per messaggi frammentati
    String _fragmentBuffer;
    uint32_t _fragmentClientId;

    void onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                 AwsEventType type, void* arg, uint8_t* data, size_t len);

    void handleMessage(AsyncWebSocketClient* client, uint8_t* data, size_t len);
    void handleFragmentedMessage(AsyncWebSocketClient* client, AwsFrameInfo* info, uint8_t* data, size_t len);
};

#endif // WEBSOCKET_MANAGER_H