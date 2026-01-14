/*
 * LED Matrix Effects - Full Featured Version
 * With WiFi, WebSocket, mDNS, and persistent settings
 * CSV Protocol
 */

#include "Debug.h"
#include <Arduino.h>
#include "DisplayManager.h"
#include "EffectManager.h"
#include "TimeManager.h"
#include "Settings.h"
#include "WiFiManager.h"
#include "WebServerManager.h"
#include "WebSocketManager.h"
#include "CommandHandler.h"
#include "Discovery.h"
#include "ImageManager.h"
#include "TextScheduleManager.h"

// Effects
#include "effects/PongEffect.h"
#include "effects/SnakeEffect.h"
#include "effects/MarioClockEffect.h"
#include "effects/PacManClockEffect.h"
//#include "effects/PlasmaEffect.h"
#include "effects/ScrollTextEffect.h"
#include "effects/MatrixRainEffect.h"
//#include "effects/FireEffect.h"
//#include "effects/StarFieldEffect.h"
#include "effects/ImageEffect.h"
#include "effects/DynamicImageEffect.h"


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
DiscoveryService* discoveryService = nullptr;
ImageManager* imageManager = nullptr;
TextScheduleManager* scheduleManager = nullptr;
DynamicImageEffect* dynamicImageEffect = nullptr;
ScrollTextEffect* scrollTextEffect = nullptr;
PongEffect* pongEffect = nullptr;
SnakeEffect* snakeEffect = nullptr;

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
// Scheduled Text State
// ═══════════════════════════════════════════
int previousEffectIndex = -1;  // Indice dell'effetto prima della scritta programmata

// ═══════════════════════════════════════════
// Serial Buffer
// ═══════════════════════════════════════════
String serialBuffer = "";

// ═══════════════════════════════════════════
// Setup
// ═══════════════════════════════════════════
void setup() {
    DEBUG_INIT(115200);
    delay(500);

    DEBUG_PRINTLN(F(""));
    DEBUG_PRINTLN(F("╔══════════════════════════════════════════════════════╗"));
    DEBUG_PRINTLN(F("║     ESP32 LED Matrix - CSV Protocol Version         ║"));
    DEBUG_PRINTLN(F("║     WiFi + WebSocket + mDNS + Persistent Settings    ║"));
    DEBUG_PRINTLN(F("╚══════════════════════════════════════════════════════╝"));
    DEBUG_PRINTLN(F(""));

    // ─────────────────────────────────────────
    // 0. OTA Boot Validation (FIRST!)
    // ─────────────────────────────────────────
    DEBUG_PRINTLN(F("[Setup] Checking OTA boot status..."));
    CommandHandler::checkOTABootStatus();

    // ─────────────────────────────────────────
    // 1. Settings
    // ─────────────────────────────────────────
    DEBUG_PRINTLN(F("[Setup] Loading settings..."));
    settings.begin();
    
    // ─────────────────────────────────────────
    // 2. Display
    // ─────────────────────────────────────────
    DEBUG_PRINTLN(F("[Setup] Initializing display..."));
    displayManager = new DisplayManager(PANEL_WIDTH, PANEL_HEIGHT, PANELS_NUMBER, PIN_E);
    
    if (!displayManager->begin()) {
        DEBUG_PRINTLN(F("FATAL: Display initialization failed!"));
        while (1) delay(100);
    }
    
    displayManager->setBrightness(settings.getBrightnessDay());
    displayManager->fillScreen(0, 0, 0);
    DEBUG_PRINTLN(F("[Setup] ✓ Display OK"));
    
    // ─────────────────────────────────────────
    // 3. Time Manager
    // ─────────────────────────────────────────
    DEBUG_PRINTLN(F("[Setup] Initializing TimeManager..."));
    timeManager = new TimeManager;  // RTC mode
    timeManager->enableNTP(settings.isNTPEnabled());
    timeManager->begin();
    timeManager->setTimezone(settings.getTimezone());
    DEBUG_PRINTLN(F("[Setup] ✓ TimeManager OK"));
    
    // ─────────────────────────────────────────
    // 4. Effect Manager
    // ─────────────────────────────────────────
    DEBUG_PRINTLN(F("[Setup] Initializing EffectManager..."));
    effectManager = new EffectManager(displayManager, settings.getEffectDuration());
    
    // Aggiungi effetti
    effectManager->addEffect(new MarioClockEffect(displayManager, timeManager));
    effectManager->addEffect(new PacManClockEffect(displayManager, timeManager));
    scrollTextEffect = new ScrollTextEffect(displayManager, settings.getScrollText(), 3, settings.getScrollTextColor());
    effectManager->addEffect(scrollTextEffect);
    //effectManager->addEffect(new PlasmaEffect(displayManager));
    pongEffect = new PongEffect(displayManager);
    effectManager->addEffect(pongEffect);
    snakeEffect = new SnakeEffect(displayManager);
    effectManager->addEffect(snakeEffect);
    effectManager->addEffect(new MatrixRainEffect(displayManager));
    
    DEBUG_PRINTF("[Setup] ✓ Loaded %d effects\n", effectManager->getEffectCount());
    
    // Applica impostazioni salvate
    effectManager->setAutoSwitch(settings.isAutoSwitch());

    if (!settings.isAutoSwitch()) {
        DEBUG_PRINTLN(F("[Setup] Setting current effect from settings..."));
        if (settings.getCurrentEffect() >= 0) {
            effectManager->switchToEffect(settings.getCurrentEffect());
            DEBUG_PRINTF("[Setup] ✓ Effect set to index %d\n", settings.getCurrentEffect());
        }
    }
    
    // ─────────────────────────────────────────
    // 5. WiFi
    // ─────────────────────────────────────────
    DEBUG_PRINTLN(F("[Setup] Initializing WiFi..."));
    wifiManager = new WiFiManager(&settings);
    wifiManager->begin();
    DEBUG_PRINTLN(F("[Setup] ✓ WiFi OK"));

    // ─────────────────────────────────────────
    // 6. Image Manager
    // ─────────────────────────────────────────
    DEBUG_PRINTLN(F("[Setup] Initializing ImageManager..."));
    imageManager = new ImageManager();
    if (!imageManager->begin()) {
        DEBUG_PRINTLN(F("[Setup] ⚠ ImageManager failed, image commands disabled"));
    } else {
        DEBUG_PRINTLN(F("[Setup] ✓ ImageManager OK"));

        // Aggiungi DynamicImageEffect se ImageManager è disponibile
        DEBUG_PRINTLN(F("[Setup] Adding DynamicImageEffect..."));
        dynamicImageEffect = new DynamicImageEffect(displayManager, imageManager, 5000); // 5 sec per immagine
        effectManager->addEffect(dynamicImageEffect);
        DEBUG_PRINTLN(F("[Setup] ✓ DynamicImageEffect added"));
    }

    // ─────────────────────────────────────────
    // 7. Text Schedule Manager
    // ─────────────────────────────────────────
    DEBUG_PRINTLN(F("[Setup] Initializing TextScheduleManager..."));
    scheduleManager = new TextScheduleManager();
    scheduleManager->begin();
    DEBUG_PRINTLN(F("[Setup] ✓ TextScheduleManager OK"));

    // ─────────────────────────────────────────
    // 8. Command Handler
    // ─────────────────────────────────────────
    commandHandler.init(timeManager, effectManager, displayManager, &settings, wifiManager, imageManager, scheduleManager);
    commandHandler.setScrollTextEffect(scrollTextEffect);
    commandHandler.setPongEffect(pongEffect);
    commandHandler.setSnakeEffect(snakeEffect);
    
    // ─────────────────────────────────────────
    // 8. Web Server
    // ─────────────────────────────────────────
    DEBUG_PRINTLN(F("[Setup] Initializing WebServer..."));
    webServer = new WebServerManager(80);
    webServer->init(&commandHandler);
    DEBUG_PRINTLN(F("[Setup] ✓ WebServer OK"));

    // ─────────────────────────────────────────
    // 9. WebSocket
    // ─────────────────────────────────────────
    DEBUG_PRINTLN(F("[Setup] Initializing WebSocket..."));
    wsManager = new WebSocketManager();
    wsManager->init(webServer->getServer(), &commandHandler);
    commandHandler.setWebSocketManager(wsManager);
    DEBUG_PRINTLN(F("[Setup] ✓ WebSocket OK"));

    // ─────────────────────────────────────────
    // 10. Callbacks per notifiche
    // ─────────────────────────────────────────
    timeManager->addOnMinuteChange([](int h, int m, int s) {
        // Aggiorna luminosità ogni minuto se necessario
        commandHandler.updateBrightness();
    });

    // Callback per scritte programmate
    timeManager->addOnMinuteChange([](int h, int m, int s) {
        if (scheduleManager && scrollTextEffect && effectManager) {
            // Ottieni data e giorno della settimana correnti
            int year = timeManager->getYear();
            int month = timeManager->getMonth();
            int day = timeManager->getDay();
            int weekDay = timeManager->getWeekday();

            // Controlla se c'è una scritta programmata per questo momento
            ScheduledText* scheduled = scheduleManager->getActiveScheduledText(h, m, year, month, day, weekDay);
            if (scheduled) {
                DEBUG_PRINTF("[Schedule] Activating scheduled text ID %d: %s (loops: %d)\n",
                            scheduled->id, scheduled->text, scheduled->loopCount);

                // Salva l'indice dell'effetto corrente per poterci tornare dopo
                previousEffectIndex = effectManager->getCurrentEffectIndex();
                DEBUG_PRINTF("[Schedule] Saving previous effect index: %d\n", previousEffectIndex);

                // Configura il testo, colore e numero di loop
                scrollTextEffect->setText(scheduled->text);
                scrollTextEffect->setColor(scheduled->color);
                scrollTextEffect->setLoopCount(scheduled->loopCount);

                // Attiva l'effetto scroll text
                effectManager->switchToEffect("Scroll Text");
            }
        }
    });

    // ─────────────────────────────────────────
    // 11. Discovery Service
    // ─────────────────────────────────────────
    DEBUG_PRINTLN(F("[Setup] Initializing Discovery Service..."));
    discoveryService = new DiscoveryService(&settings, 80);
    discoveryService->begin();
    DEBUG_PRINTLN(F("[Setup] ✓ Discovery Service OK"));


    // ─────────────────────────────────────────
    // Setup complete
    // ─────────────────────────────────────────
    DEBUG_PRINTLN(F(""));
    DEBUG_PRINTLN(F("╔══════════════════════════════════════════════════════╗"));
    DEBUG_PRINTLN(F("║                   Setup Complete!                    ║"));
    DEBUG_PRINTLN(F("╠══════════════════════════════════════════════════════╣"));
    DEBUG_PRINTF("║  IP Address: %-39s ║\n", wifiManager->getIP().c_str());
    DEBUG_PRINTF("║  mDNS: %-45s ║\n", 
                 (String(settings.getDeviceName()) + ".local").c_str());
    DEBUG_PRINTF("║  WebSocket: ws://%s/ws                        ║\n", 
                 wifiManager->getIP().c_str());
    DEBUG_PRINTLN(F("╠══════════════════════════════════════════════════════╣"));
    DEBUG_PRINTLN(F("║  Serial Commands: T, D, E, M, S, ?, p, r, n, 0-9     ║"));
    DEBUG_PRINTLN(F("║  CSV Commands: getStatus, effect,next, etc.          ║"));
    DEBUG_PRINTLN(F("╚══════════════════════════════════════════════════════╝"));
    DEBUG_PRINTLN(F(""));
    
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
    // OTA Watchdog (controlla timeout durante OTA)
    // ─────────────────────────────────────────
    commandHandler.checkOTAWatchdog();

    // ─────────────────────────────────────────
    // Time Manager update
    // ─────────────────────────────────────────
    timeManager->update();

    // ─────────────────────────────────────────
    // Effect Manager update
    // ─────────────────────────────────────────
    effectManager->update();

    // ─────────────────────────────────────────
    // Scheduled Text: ritorna all'effetto precedente
    // ─────────────────────────────────────────
    if (previousEffectIndex >= 0 && scrollTextEffect && scrollTextEffect->isComplete()) {
        DEBUG_PRINTF("[Schedule] Scroll text completed, returning to effect %d\n", previousEffectIndex);
        effectManager->switchToEffect(previousEffectIndex);
        scrollTextEffect->setLoopCount(0);  // Reset a infinito per uso manuale
        previousEffectIndex = -1;  // Reset
    }

    // ─────────────────────────────────────────
    // WiFi check
    // ─────────────────────────────────────────
    wifiManager->update();

    // ─────────────────────────────────────────
    // Discovery Service update
    // ─────────────────────────────────────────
    discoveryService->update();
    
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
        DEBUG_PRINTF("[Stats] Heap: %u bytes | WS Clients: %u\n", 
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