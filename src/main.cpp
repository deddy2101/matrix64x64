/*
 * LED Matrix Effects - Refactored Version
 * Architettura Object-Oriented con separazione delle responsabilità
 */

#include <Arduino.h>
#include "DisplayManager.h"
#include "EffectManager.h"
#include "effects/PongEffect.h"
#include "effects/MarioClockEffect.h"
// Aggiungi altri include per gli effetti quando li implementi
// #include "effects/PlasmaEffect.h"
// #include "effects/ScrollTextEffect.h"
// etc...

// Configurazione hardware
#define PANEL_WIDTH 64
#define PANEL_HEIGHT 64
#define PANELS_NUMBER 1
#define PIN_E 32

// Oggetti globali
DisplayManager* displayManager = nullptr;
EffectManager* effectManager = nullptr;

// Timer per statistiche
unsigned long statsTimer = 0;
const unsigned long STATS_INTERVAL = 5000;  // Stampa stats ogni 5 secondi

void setup() {
    Serial.begin(115200);
    Serial.println(F("*****************************************************"));
    Serial.println(F("*    ESP32 LED Matrix - OOP Refactored Version      *"));
    Serial.println(F("*****************************************************"));
    
    // Inizializza Display Manager
    displayManager = new DisplayManager(PANEL_WIDTH, PANEL_HEIGHT, 
                                       PANELS_NUMBER, PIN_E);
    
    if (!displayManager->begin()) {
        Serial.println("FATAL: Display initialization failed!");
        while(1) delay(100);
    }
    
    displayManager->setBrightness(200);
    displayManager->fillScreen(0, 0, 0);
    
    Serial.println("Display initialized successfully");
    
    // Inizializza Effect Manager (10 secondi per effetto)
    effectManager = new EffectManager(displayManager, 10000);
    
    // Aggiungi gli effetti che vuoi usare
    // NOTA: Gli effetti vengono creati dinamicamente e gestiti dal manager
    
    // effectManager->addEffect(new ScrollTextEffect(displayManager, 
    //     "PROSSIMA FERMATA FIRENZE 6 GIARDINI ROSSI"));
    
    // effectManager->addEffect(new PlasmaEffect(displayManager));
    
    effectManager->addEffect(new PongEffect(displayManager));
    
    effectManager->addEffect(new MarioClockEffect(displayManager));
    
    // effectManager->addEffect(new MatrixRainEffect(displayManager));
    // effectManager->addEffect(new FireEffect(displayManager));
    // effectManager->addEffect(new StarfieldEffect(displayManager));
    // effectManager->addEffect(new ImageEffect(displayManager, IMAGE_BOSCO));
    
    Serial.printf("Loaded %d effects\n", effectManager->getEffectCount());
    
    statsTimer = millis();
}

void loop() {
    // Aggiorna l'effect manager (gestisce automaticamente il cambio effetti)
    effectManager->update();
    
    // Stampa statistiche periodicamente
    if (millis() - statsTimer >= STATS_INTERVAL) {
        effectManager->printStats();
        statsTimer = millis();
    }
    
    // Delay per controllo frame rate (circa 50 FPS)
    delay(20);
}

// Funzioni opzionali per controllo manuale (es. tramite MQTT o Web)
void switchToEffect(const char* effectName) {
    // Implementa la logica per cambiare effetto by name
    // Utile per controllo remoto
}

void setEffectDuration(unsigned long ms) {
    if (effectManager) {
        effectManager->setDuration(ms);
    }
}
