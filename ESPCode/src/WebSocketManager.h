#ifndef WEBSOCKET_MANAGER_H
#define WEBSOCKET_MANAGER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include "CommandHandler.h"

class WebSocketManager {
private:
    AsyncWebSocket ws;
    CommandHandler* commandHandler;
    
    // Statistiche
    uint32_t messagesReceived;
    uint32_t messagesSent;
    uint32_t clientsConnected;
    
    // Callback statico per eventi WebSocket
    static void onWebSocketEvent(AsyncWebSocket* server, 
                                  AsyncWebSocketClient* client,
                                  AwsEventType type, 
                                  void* arg, 
                                  uint8_t* data, 
                                  size_t len);
    
    // Istanza statica per callback
    static WebSocketManager* instance;
    
    // Gestione messaggi
    void handleMessage(AsyncWebSocketClient* client, uint8_t* data, size_t len);
    void handleConnect(AsyncWebSocketClient* client);
    void handleDisconnect(AsyncWebSocketClient* client);
    
public:
    WebSocketManager();
    
    // Inizializzazione
    void begin(AsyncWebServer* server, CommandHandler* cmdHandler);
    
    // Invia messaggio a tutti i client
    void broadcast(const String& message);
    
    // Invia messaggio a un client specifico
    void send(uint32_t clientId, const String& message);
    
    // Invia notifica di aggiornamento stato
    void notifyStatusChange();
    void notifyEffectChange(const char* effectName, int index);
    void notifyTimeChange(const String& time);
    
    // Statistiche
    uint32_t getClientsConnected() const { return clientsConnected; }
    uint32_t getMessagesReceived() const { return messagesReceived; }
    uint32_t getMessagesSent() const { return messagesSent; }
    
    // Cleanup
    void cleanupClients();
};

#endif
