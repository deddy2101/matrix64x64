/*
 * LED Matrix Effects - Full Featured Version
 * With WiFi, WebSocket, mDNS, and persistent settings
 * CSV Protocol
 */

#include <Arduino.h>
#include "DisplayManager.h"
#include "EffectManager.h"
#include "TimeManager.h"
#include "Settings.h"
#include "WiFiManager.h"
#include "WebServerManager.h"
#include "WebSocketManager.h"
#include "CommandHandler.h"

// Effects
#include "effects/PongEffect.h"
#include "effects/MarioClockEffect.h"
#include "effects/PlasmaEffect.h"
#include "effects/ScrollTextEffect.h"
#include "effects/MatrixRainEffect.h"
#include "effects/FireEffect.h"
#include "effects/StarFieldEffect.h"
#include "effects/ImageEffect.h"

// Images
#include "andre.h"
#include "mario.h"
#include "paese.h"
#include "pokemon.h"
#include "luigi.h"
#include "cave.h"
#include "fox.h"

// ═══════════════════════════════════════════
// Hardware Configuration
// ═══════════════════════════════════════════
#define PANEL_WIDTH 64
#define PANEL_HEIGHT 64
#define PANELS_NUMBER 1
#define PIN_E 32

// ═══════════════════════════════════════════
// Global Objects
// ═══════════════════════════════════════════
Settings settings;
DisplayManager* displayManager = nullptr;
EffectManager* effectManager = nullptr;
TimeManager* timeManager = nullptr;
WiFiManager* wifiManager = nullptr;
WebServerManager* webServer = nullptr;
WebSocketManager* wsManager = nullptr;
CommandHandler commandHandler;

// ═══════════════════════════════════════════
// Timers
// ═══════════════════════════════════════════
unsigned long statsTimer = 0;
unsigned long brightnessTimer = 0;
unsigned long wsCleanupTimer = 0;

const unsigned long STATS_INTERVAL = 30000;      // Stats ogni 30 secondi
const unsigned long BRIGHTNESS_INTERVAL = 60000; // Check brightness ogni minuto
const unsigned long WS_CLEANUP_INTERVAL = 1000;  // Cleanup WS ogni secondo

// ═══════════════════════════════════════════
// Serial Buffer
// ═══════════════════════════════════════════
String serialBuffer = "";

// ═══════════════════════════════════════════
// Setup
// ═══════════════════════════════════════════
void setup() {
    Serial.begin(115200);
    delay(500);
    
    Serial.println(F(""));
    Serial.println(F("╔══════════════════════════════════════════════════════╗"));
    Serial.println(F("║     ESP32 LED Matrix - CSV Protocol Version         ║"));
    Serial.println(F("║     WiFi + WebSocket + mDNS + Persistent Settings    ║"));
    Serial.println(F("╚══════════════════════════════════════════════════════╝"));
    Serial.println(F(""));
    
    // ─────────────────────────────────────────
    // 1. Settings
    // ─────────────────────────────────────────
    Serial.println(F("[Setup] Loading settings..."));
    settings.begin();
    
    // ─────────────────────────────────────────
    // 2. Display
    // ─────────────────────────────────────────
    Serial.println(F("[Setup] Initializing display..."));
    displayManager = new DisplayManager(PANEL_WIDTH, PANEL_HEIGHT, PANELS_NUMBER, PIN_E);
    
    if (!displayManager->begin()) {
        Serial.println(F("FATAL: Display initialization failed!"));
        while (1) delay(100);
    }
    
    displayManager->setBrightness(settings.getBrightnessDay());
    displayManager->fillScreen(0, 0, 0);
    Serial.println(F("[Setup] ✓ Display OK"));
    
    // ─────────────────────────────────────────
    // 3. Time Manager
    // ─────────────────────────────────────────
    Serial.println(F("[Setup] Initializing TimeManager..."));
    timeManager = new TimeManager(false);  // RTC mode
    timeManager->begin();
    Serial.println(F("[Setup] ✓ TimeManager OK"));
    
    // ─────────────────────────────────────────
    // 4. Effect Manager
    // ─────────────────────────────────────────
    Serial.println(F("[Setup] Initializing EffectManager..."));
    effectManager = new EffectManager(displayManager, settings.getEffectDuration());
    
    // Aggiungi effetti
    effectManager->addEffect(new ScrollTextEffect(displayManager,
        "PROSSIMA FERMATA FIRENZE 6 GIARDINI ROSSI"));
    effectManager->addEffect(new PlasmaEffect(displayManager));
    effectManager->addEffect(new PongEffect(displayManager));
    effectManager->addEffect(new MatrixRainEffect(displayManager));
    effectManager->addEffect(new FireEffect(displayManager));
    effectManager->addEffect(new StarfieldEffect(displayManager));
    effectManager->addEffect(new ImageEffect(displayManager, 
        (DrawImageFunction)draw_paese, "Paese"));
    effectManager->addEffect(new ImageEffect(displayManager, 
        (DrawImageFunction)draw_pokemon, "Pokemon"));
    effectManager->addEffect(new MarioClockEffect(displayManager, timeManager));
    effectManager->addEffect(new ImageEffect(displayManager, 
        (DrawImageFunction)draw_andre, "Andre"));
    effectManager->addEffect(new ImageEffect(displayManager, 
        (DrawImageFunction)draw_mario, "Mario"));
    effectManager->addEffect(new ImageEffect(displayManager, 
        (DrawImageFunction)draw_luigi, "Luigi"));
    effectManager->addEffect(new ImageEffect(displayManager, 
        (DrawImageFunction)draw_cave, "Cave"));
    effectManager->addEffect(new ImageEffect(displayManager, 
        (DrawImageFunction)draw_fox, "Fox"));
    
    Serial.printf("[Setup] ✓ Loaded %d effects\n", effectManager->getEffectCount());
    
    // Applica impostazioni salvate
    if (settings.isAutoSwitch()) {
        effectManager->resume();
    } else {
        effectManager->pause();
        if (settings.getCurrentEffect() >= 0) {
            effectManager->switchToEffect(settings.getCurrentEffect());
        }
    }
    
    // ─────────────────────────────────────────
    // 5. WiFi
    // ─────────────────────────────────────────
    Serial.println(F("[Setup] Initializing WiFi..."));
    wifiManager = new WiFiManager(&settings);
    wifiManager->begin();
    Serial.println(F("[Setup] ✓ WiFi OK"));
    
    // ─────────────────────────────────────────
    // 6. Command Handler
    // ─────────────────────────────────────────
    commandHandler.init(timeManager, effectManager, displayManager, &settings, wifiManager);
    
    // ─────────────────────────────────────────
    // 7. Web Server
    // ─────────────────────────────────────────
    Serial.println(F("[Setup] Initializing WebServer..."));
    webServer = new WebServerManager(80);
    webServer->init(&commandHandler);
    Serial.println(F("[Setup] ✓ WebServer OK"));
    
    // ─────────────────────────────────────────
    // 8. WebSocket
    // ─────────────────────────────────────────
    Serial.println(F("[Setup] Initializing WebSocket..."));
    wsManager = new WebSocketManager();
    wsManager->init(webServer->getServer(), &commandHandler);
    commandHandler.setWebSocketManager(wsManager);
    Serial.println(F("[Setup] ✓ WebSocket OK"));
    
    // ─────────────────────────────────────────
    // 9. Callbacks per notifiche
    // ─────────────────────────────────────────
    timeManager->setOnMinuteChange([](int h, int m, int s) {
        // Aggiorna luminosità ogni minuto se necessario
        commandHandler.updateBrightness();
    });
    
    // ─────────────────────────────────────────
    // Setup complete
    // ─────────────────────────────────────────
    Serial.println(F(""));
    Serial.println(F("╔══════════════════════════════════════════════════════╗"));
    Serial.println(F("║                   Setup Complete!                    ║"));
    Serial.println(F("╠══════════════════════════════════════════════════════╣"));
    Serial.printf("║  IP Address: %-39s ║\n", wifiManager->getIP().c_str());
    Serial.printf("║  mDNS: %-45s ║\n", 
                 (String(settings.getDeviceName()) + ".local").c_str());
    Serial.printf("║  WebSocket: ws://%s/ws                        ║\n", 
                 wifiManager->getIP().c_str());
    Serial.println(F("╠══════════════════════════════════════════════════════╣"));
    Serial.println(F("║  Serial Commands: T, D, E, M, S, ?, p, r, n, 0-9     ║"));
    Serial.println(F("║  CSV Commands: getStatus, effect,next, etc.          ║"));
    Serial.println(F("╚══════════════════════════════════════════════════════╝"));
    Serial.println(F(""));
    
    // Inizializza timer
    statsTimer = millis();
    brightnessTimer = millis();
    wsCleanupTimer = millis();
    
    // Imposta luminosità iniziale
    commandHandler.updateBrightness();
    
    // Start effects
    effectManager->start();
}

// ═══════════════════════════════════════════
// Loop
// ═══════════════════════════════════════════
void loop() {
    unsigned long now = millis();
    
    // ─────────────────────────────────────────
    // Time Manager update
    // ─────────────────────────────────────────
    timeManager->update();
    
    // ─────────────────────────────────────────
    // Effect Manager update
    // ─────────────────────────────────────────
    effectManager->update();
    
    // ─────────────────────────────────────────
    // WiFi check
    // ─────────────────────────────────────────
    wifiManager->update();
    
    // ─────────────────────────────────────────
    // WebSocket cleanup
    // ─────────────────────────────────────────
    if (now - wsCleanupTimer >= WS_CLEANUP_INTERVAL) {
        wsManager->cleanupClients();
        wsCleanupTimer = now;
    }
    
    // ─────────────────────────────────────────
    // Brightness check (ogni minuto)
    // ─────────────────────────────────────────
    if (now - brightnessTimer >= BRIGHTNESS_INTERVAL) {
        commandHandler.updateBrightness();
        brightnessTimer = now;
    }
    
    // ─────────────────────────────────────────
    // Stats (ogni 30 secondi)
    // ─────────────────────────────────────────
    if (now - statsTimer >= STATS_INTERVAL) {
        effectManager->printStats();
        Serial.printf("[Stats] Heap: %u bytes | WS Clients: %u\n", 
                     ESP.getFreeHeap(), 
                     wsManager->getClientsConnected());
        statsTimer = now;
    }
    
    // ─────────────────────────────────────────
    // Serial command processing
    // ─────────────────────────────────────────
    while (Serial.available()) {
        char c = Serial.read();
        
        if (c == '\n' || c == '\r') {
            if (serialBuffer.length() > 0) {
                commandHandler.processSerial(serialBuffer);
                serialBuffer = "";
            }
        } else {
            serialBuffer += c;
        }
    }
    
    // ─────────────────────────────────────────
    // Small delay
    // ─────────────────────────────────────────
    delay(10);
}