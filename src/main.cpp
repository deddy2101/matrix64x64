/*
 * Multi-Pattern LED Matrix Effects
 * Includes: Plasma, Pong, Matrix Rain, Fire, Starfield, Image Display
 */

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <FastLED.h>
#include "bosco.h"  // Include your generated image file
#include "casa.h" 
#include "mario.h" 
#include "paese.h" 
#include "pokemon.h" 
#include "andre.h"
// Configure for your panel(s) as appropriate!
#define PANEL_WIDTH 64
#define PANEL_HEIGHT 64
#define PANELS_NUMBER 1
#define PIN_E 32

#define PANE_WIDTH PANEL_WIDTH * PANELS_NUMBER
#define PANE_HEIGHT PANEL_HEIGHT

// placeholder for the matrix object
MatrixPanel_I2S_DMA *dma_display = nullptr;

uint16_t time_counter = 0, cycles = 0, fps = 0;
unsigned long fps_timer;
unsigned long effect_timer; // Timer per cambio effetto

CRGB currentColor;
CRGBPalette16 palettes[] = {HeatColors_p, LavaColors_p, RainbowColors_p, RainbowStripeColors_p, CloudColors_p};
CRGBPalette16 currentPalette = palettes[0];

// Pong variables
float ballX = PANE_WIDTH / 2;
float ballY = PANE_HEIGHT / 2;
float ballSpeedX = 0.8;
float ballSpeedY = 0.6;
int paddle1Y = PANE_HEIGHT / 2;
int paddle2Y = PANE_HEIGHT / 2;
int paddleHeight = 12;
int paddleWidth = 2;
int score1 = 0;
int score2 = 0;

// Matrix Rain variables
#define MAX_DROPS 20
struct Drop {
  int x;
  int y;
  int speed;
  int length;
  bool active;
};
Drop drops[MAX_DROPS];

// Fire variables
byte heat[PANE_WIDTH][PANE_HEIGHT];

// Starfield variables
struct Star {
  float x, y, z;
};
#define MAX_STARS 50
Star stars[MAX_STARS];

// Effect selector - IMAGE added
enum Effect { PLASMA, PONG, MATRIX_RAIN, FIRE, STARFIELD, IMAGE1, IMAGE2, IMAGE3, IMAGE4, IMAGE5,IMAGE6, NUM_EFFECTS };
Effect currentEffect = PLASMA;

CRGB ColorFromCurrentPalette(uint8_t index = 0, uint8_t brightness = 255, TBlendType blendType = LINEARBLEND) {
  return ColorFromPalette(currentPalette, index, brightness, blendType);
}

void initMatrixRain() {
  for (int i = 0; i < MAX_DROPS; i++) {
    drops[i].x = random(PANE_WIDTH);
    drops[i].y = random(-50, 0);
    drops[i].speed = random(1, 4);
    drops[i].length = random(5, 15);
    drops[i].active = true;
  }
}

void initStarfield() {
  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].x = random(-PANE_WIDTH, PANE_WIDTH);
    stars[i].y = random(-PANE_HEIGHT, PANE_HEIGHT);
    stars[i].z = random(1, PANE_WIDTH);
  }
}

void initPong() {
  ballX = PANE_WIDTH / 2;
  ballY = PANE_HEIGHT / 2;
  ballSpeedX = 0.8;
  ballSpeedY = 0.6;
  paddle1Y = PANE_HEIGHT / 2;
  paddle2Y = PANE_HEIGHT / 2;
  score1 = 0;
  score2 = 0;
}

void switchEffect() {
  // Passa all'effetto successivo
  currentEffect = (Effect)((currentEffect + 1) % NUM_EFFECTS);
  
  // Reset variabili e inizializzazione
  time_counter = 0;
  cycles = 0;
  
  switch(currentEffect) {
    case PLASMA:
      Serial.println("Switching to PLASMA effect...");
      currentPalette = palettes[random(0, sizeof(palettes)/sizeof(palettes[0]))];
      dma_display->fillScreenRGB888(0, 0, 0);
      break;
    case PONG:
      Serial.println("Switching to PONG game...");
      initPong();
      dma_display->fillScreenRGB888(0, 0, 0);
      break;
    case MATRIX_RAIN:
      Serial.println("Switching to MATRIX RAIN effect...");
      initMatrixRain();
      dma_display->fillScreenRGB888(0, 0, 0);
      break;
    case FIRE:
      Serial.println("Switching to FIRE effect...");
      memset(heat, 0, sizeof(heat));
      dma_display->fillScreenRGB888(0, 0, 0);
      break;
    case STARFIELD:
      Serial.println("Switching to STARFIELD effect...");
      initStarfield();
      dma_display->fillScreenRGB888(0, 0, 0);
      break;
    case IMAGE1:
      Serial.println("Switching to IMAGE display...");
      dma_display->fillScreenRGB888(0, 0, 0);
      draw_bosco(dma_display, 0, 0);  // Draw the image
      break;
    case IMAGE2:
      Serial.println("Switching to IMAGE display...");
      dma_display->fillScreenRGB888(0, 0, 0);
      draw_casa(dma_display, 0, 0);  // Draw the image
      break;
    case IMAGE3:
      Serial.println("Switching to IMAGE display...");
      dma_display->fillScreenRGB888(0, 0, 0);
      draw_mario(dma_display, 0, 0);  // Draw the image
      break;
    case IMAGE4:
      Serial.println("Switching to IMAGE display...");
      dma_display->fillScreenRGB888(0, 0, 0);
      draw_paese(dma_display, 0, 0);  // Draw the image
      break;
    case IMAGE5:
      Serial.println("Switching to IMAGE display...");
      dma_display->fillScreenRGB888(0, 0, 0);
      draw_pokemon(dma_display, 0, 0);  // Draw the image
      break;
    case IMAGE6:
      Serial.println("Switching to IMAGE display...");
      dma_display->fillScreenRGB888(0, 0, 0);
      draw_andre(dma_display, 0, 0);  // Draw the image
      break;
    default:
      break;
  }
  
  effect_timer = millis(); // Reset timer
}

void setup() {
  Serial.begin(115200);
  
  Serial.println(F("*****************************************************"));
  Serial.println(F("*        ESP32 LED Matrix - Multiple Effects        *"));
  Serial.println(F("*****************************************************"));

  HUB75_I2S_CFG mxconfig;
  mxconfig.mx_height = PANEL_HEIGHT;
  mxconfig.chain_length = PANELS_NUMBER;
  mxconfig.gpio.e = PIN_E;

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->setBrightness8(200);

  if(!dma_display->begin())
      Serial.println("****** !KABOOM! I2S memory allocation failed ***********");
 
  dma_display->fillScreenRGB888(0, 0, 0);
  
  // Inizia con PLASMA
  currentEffect = PLASMA;
  Serial.println("Starting with PLASMA effect...");
  currentPalette = RainbowColors_p;
  
  fps_timer = millis();
  effect_timer = millis(); // Inizializza timer effetto
}

void drawPlasma() {
  for (int x = 0; x < PANE_WIDTH; x++) {
    for (int y = 0; y < PANE_HEIGHT; y++) {
      int16_t v = 128;
      uint8_t wibble = sin8(time_counter);
      v += sin16(x * wibble * 3 + time_counter);
      v += cos16(y * (128 - wibble) + time_counter);
      v += sin16(y * x * cos8(-time_counter) / 8);

      currentColor = ColorFromPalette(currentPalette, (v >> 8));
      dma_display->drawPixelRGB888(x, y, currentColor.r, currentColor.g, currentColor.b);
    }
  }
  
  // Cambia palette ogni 1024 cicli all'interno del plasma
  if (cycles >= 1024) {
    currentPalette = palettes[random(0, sizeof(palettes)/sizeof(palettes[0]))];
  }
}

void drawPong() {
  dma_display->fillScreenRGB888(0, 0, 0);
  
  // Update ball position
  ballX += ballSpeedX;
  ballY += ballSpeedY;
  
  // Ball collision with top/bottom
  if (ballY <= 0 || ballY >= PANE_HEIGHT - 1) {
    ballSpeedY = -ballSpeedY;
  }
  
  // Ball collision with paddles
  if (ballX <= paddleWidth && ballY >= paddle1Y && ballY <= paddle1Y + paddleHeight) {
    ballSpeedX = abs(ballSpeedX);
  }
  if (ballX >= PANE_WIDTH - paddleWidth - 1 && ballY >= paddle2Y && ballY <= paddle2Y + paddleHeight) {
    ballSpeedX = -abs(ballSpeedX);
  }
  
  // Score
  if (ballX < 0) {
    score2++;
    ballX = PANE_WIDTH / 2;
    ballY = PANE_HEIGHT / 2;
  }
  if (ballX >= PANE_WIDTH) {
    score1++;
    ballX = PANE_WIDTH / 2;
    ballY = PANE_HEIGHT / 2;
  }
  
  // AI for paddles
  if (ballY > paddle1Y + paddleHeight/2) paddle1Y++;
  if (ballY < paddle1Y + paddleHeight/2) paddle1Y--;
  if (ballY > paddle2Y + paddleHeight/2) paddle2Y++;
  if (ballY < paddle2Y + paddleHeight/2) paddle2Y--;
  
  paddle1Y = constrain(paddle1Y, 0, PANE_HEIGHT - paddleHeight);
  paddle2Y = constrain(paddle2Y, 0, PANE_HEIGHT - paddleHeight);
  
  // Draw paddles
  for (int i = 0; i < paddleHeight; i++) {
    for (int w = 0; w < paddleWidth; w++) {
      dma_display->drawPixelRGB888(w, paddle1Y + i, 0, 255, 0);
      dma_display->drawPixelRGB888(PANE_WIDTH - paddleWidth + w, paddle2Y + i, 255, 0, 0);
    }
  }
  
  // Draw ball
  dma_display->drawPixelRGB888((int)ballX, (int)ballY, 255, 255, 255);
  dma_display->drawPixelRGB888((int)ballX + 1, (int)ballY, 255, 255, 255);
  dma_display->drawPixelRGB888((int)ballX, (int)ballY + 1, 255, 255, 255);
  dma_display->drawPixelRGB888((int)ballX + 1, (int)ballY + 1, 255, 255, 255);
  
  // Draw center line
  for (int y = 0; y < PANE_HEIGHT; y += 4) {
    dma_display->drawPixelRGB888(PANE_WIDTH/2, y, 50, 50, 50);
  }
}

void drawMatrixRain() {
  // Clear screen with slight fade effect
  for (int x = 0; x < PANE_WIDTH; x++) {
    for (int y = 0; y < PANE_HEIGHT; y++) {
      dma_display->drawPixelRGB888(x, y, 0, 2, 0); // Very dark green background
    }
  }
  
  for (int i = 0; i < MAX_DROPS; i++) {
    if (drops[i].active) {
      // Draw drop trail
      for (int j = 0; j < drops[i].length; j++) {
        int y = drops[i].y - j;
        if (y >= 0 && y < PANE_HEIGHT) {
          int brightness = 255 - (j * 20);
          if (brightness < 0) brightness = 0;
          dma_display->drawPixelRGB888(drops[i].x, y, 0, brightness, 0);
        }
      }
      
      drops[i].y += drops[i].speed;
      
      if (drops[i].y > PANE_HEIGHT + drops[i].length) {
        drops[i].y = random(-20, 0);
        drops[i].x = random(PANE_WIDTH);
        drops[i].speed = random(1, 4);
        drops[i].length = random(5, 15);
      }
    }
  }
}

void drawFire() {
  // Cool down every cell
  for (int x = 0; x < PANE_WIDTH; x++) {
    for (int y = 0; y < PANE_HEIGHT; y++) {
      heat[x][y] = qsub8(heat[x][y], random8(0, ((55 * 10) / PANE_HEIGHT) + 2));
    }
  }

  // Heat from each cell drifts up
  for (int x = 0; x < PANE_WIDTH; x++) {
    for (int y = 0; y < PANE_HEIGHT - 1; y++) {
      heat[x][y] = (heat[x][y + 1] + heat[x][y + 1] + heat[x][y + 1]) / 3;
    }
  }
  
  // Randomly ignite new sparks near bottom
  if (random8() < 120) {
    int x = random(PANE_WIDTH);
    heat[x][PANE_HEIGHT - 1] = qadd8(heat[x][PANE_HEIGHT - 1], random8(160, 255));
  }

  // Convert heat to colors
  for (int x = 0; x < PANE_WIDTH; x++) {
    for (int y = 0; y < PANE_HEIGHT; y++) {
      CRGB color = HeatColor(heat[x][y]);
      dma_display->drawPixelRGB888(x, y, color.r, color.g, color.b);
    }
  }
}

void drawStarfield() {
  dma_display->fillScreenRGB888(0, 0, 5);
  
  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].z -= 0.5;
    if (stars[i].z <= 0) {
      stars[i].x = random(-PANE_WIDTH, PANE_WIDTH);
      stars[i].y = random(-PANE_HEIGHT, PANE_HEIGHT);
      stars[i].z = PANE_WIDTH;
    }
    
    int sx = (stars[i].x / stars[i].z) * PANE_WIDTH + PANE_WIDTH / 2;
    int sy = (stars[i].y / stars[i].z) * PANE_HEIGHT + PANE_HEIGHT / 2;
    
    if (sx >= 0 && sx < PANE_WIDTH && sy >= 0 && sy < PANE_HEIGHT) {
      int brightness = map(stars[i].z, 0, PANE_WIDTH, 255, 50);
      dma_display->drawPixelRGB888(sx, sy, brightness, brightness, brightness);
    }
  }
}

void loop() {
  // Controlla se sono passati 10 secondi
  if (millis() - effect_timer >= 10000) {
    switchEffect();
  }
  
  switch(currentEffect) {
    case PLASMA:
      drawPlasma();
      break;
    case PONG:
      drawPong();
      break;
    case MATRIX_RAIN:
      drawMatrixRain();
      break;
    case FIRE:
      drawFire();
      break;
    case STARFIELD:
      drawStarfield();
      break;
    case IMAGE1:
    case IMAGE2:
    case IMAGE3:
    case IMAGE4:
    case IMAGE5:
    case IMAGE6:
      // Image is already drawn in switchEffect, no need to redraw
      break;
  }
  
  ++time_counter;
  ++cycles;
  ++fps;

  if (cycles >= 1024) {
    time_counter = 0;
    cycles = 0;
  }

  if (fps_timer + 5000 < millis()) {
    Serial.printf_P(PSTR("Effect fps: %d\n"), fps/5);
    fps_timer = millis();
    fps = 0;
  }
  
  delay(20); // Small delay for smoother animation
}