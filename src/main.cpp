/*
 * LED Matrix Effects - Refactored Version
 * Architettura Object-Oriented con separazione delle responsabilitÃ 
 */

#include <Arduino.h>
#include "DisplayManager.h"
#include "EffectManager.h"
#include "TimeManager.h"
#include "effects/PongEffect.h"
#include "effects/MarioClockEffect.h"
#include "effects/PlasmaEffect.h"
#include "effects/ScrollTextEffect.h"
#include "effects/MatrixRainEffect.h"
#include "effects/FireEffect.h"
#include "effects/StarFieldEffect.h"
#include "effects/ImageEffect.h"
#include "andre.h"
#include "mario.h"
#include "paese.h"
#include "pokemon.h"
#include "luigi.h"
#include "cave.h"
#include "fox.h"

// Configurazione hardware
#define PANEL_WIDTH 64
#define PANEL_HEIGHT 64
#define PANELS_NUMBER 1
#define PIN_E 32

// Oggetti globali
DisplayManager *displayManager = nullptr;
EffectManager *effectManager = nullptr;
TimeManager *timeManager = nullptr;

// Timer per statistiche
unsigned long statsTimer = 0;
const unsigned long STATS_INTERVAL = 5000; // Stampa stats ogni 5 secondi

void setup()
{
    Serial.begin(115200);
    Serial.println(F("*****************************************************"));
    Serial.println(F("*    ESP32 LED Matrix - OOP Refactored Version      *"));
    Serial.println(F("*****************************************************"));

    timeManager = new TimeManager(true, 3000); // Tempo finto, 3 secondi per minuto
    timeManager->begin(12, 55, 0);             // Inizia a mezzogiorno
    // Inizializza Display Manager
    displayManager = new DisplayManager(PANEL_WIDTH, PANEL_HEIGHT,
                                        PANELS_NUMBER, PIN_E);

    if (!displayManager->begin())
    {
        Serial.println("FATAL: Display initialization failed!");
        while (1)
            delay(100);
    }

    displayManager->setBrightness(200);
    displayManager->fillScreen(0, 0, 0);

    Serial.println("Display initialized successfully");

    // Inizializza Effect Manager (10 secondi per effetto)
    effectManager = new EffectManager(displayManager, 10000);

    // Aggiungi gli effetti che vuoi usare
    // NOTA: Gli effetti vengono creati dinamicamente e gestiti dal manager

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

    effectManager->addEffect(new MarioClockEffect(displayManager,
                                                  timeManager));

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

    Serial.printf("Loaded %d effects\n", effectManager->getEffectCount());
    effectManager->switchToEffect("Mario Clock");
    effectManager->pause();
    statsTimer = millis();
}

void loop()
{
    // Aggiorna l'effect manager (gestisce automaticamente il cambio effetti)
    timeManager->update();
    effectManager->update();

    // Stampa statistiche periodicamente
    if (millis() - statsTimer >= STATS_INTERVAL)
    {
        effectManager->printStats();
        statsTimer = millis();
    }
    if (Serial.available())
    {
        char cmd = Serial.read();

        switch (cmd)
        {
        case 'p':
            effectManager->pause();
            break;
        case 'r':
            effectManager->resume();
            break;
        case 'n':
            effectManager->nextEffect();
            break;
        case '0' ... '9':
            effectManager->switchToEffect(cmd - '0');
            break;
        }
    }
    // Delay per controllo frame rate (circa 50 FPS)
    delay(20);
}

void setEffectDuration(unsigned long ms)
{
    if (effectManager)
    {
        effectManager->setDuration(ms);
    }
}