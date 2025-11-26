/*
 * Multi-Pattern LED Matrix Effects
 * Includes: Plasma, Pong, Matrix Rain, Fire, Starfield, Image Display, Mario Clock
 */

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <FastLED.h>
#include "bosco.h"
#include "casa.h"
#include "mario.h"
#include "paese.h"
#include "pokemon.h"
#include "andre.h"
#include <Fonts/FreeSansBold12pt7b.h>
#include "assets.h"
#include "Super_Mario_Bros__24pt7b.h"

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
unsigned long effect_timer;

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

// Scroll text variables
bool scrollCompleted = false;
String scrollText = "PROSSIMA FERMATA FIRENZE 6 GIARDINI ROSSI";
int scrollX = PANE_WIDTH;
int scrollSpeed = 1;
int textWidth = 0;

// Mario Clock variables - Block
struct MarioBlock {
  int x;
  int y;
  int firstY;
  int width;
  int height;
  String text;
  bool isHit;
  int direction;
  int moveAmount;
  static const int MAX_MOVE = 4;
  static const int MOVE_PACE = 2;
};

MarioBlock hourBlock;
MarioBlock minuteBlock;
// Mario Clock variables - Mario
struct MarioSprite {
  int x;
  int y;
  int firstY;
  int width;
  int height;
  bool isJumping;
  int direction;
  int lastY;
  const unsigned short* sprite;
  bool collisionDetected;
  static const int JUMP_HEIGHT = 14;
  static const int MARIO_PACE = 3;
};

MarioSprite marioSprite;

// AGGIUNGI QUESTE NUOVE VARIABILI:
enum JumpTarget {
  NONE,
  HOUR_BLOCK,
  MINUTE_BLOCK,
  BOTH_BLOCKS
};

JumpTarget currentJumpTarget = NONE;
bool waitingForNextJump = false;
unsigned long nextJumpDelay = 0;

// Time tracking
int fakeHour = 12;
int fakeMin = 30;
int fakeSec = 0;
unsigned long lastSecondUpdate = 0;
unsigned long lastMarioUpdate = 0;
bool needsRedraw = true;

// Matrix Rain variables
#define MAX_DROPS 20
struct Drop
{
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
struct Star
{
  float x, y, z;
};
#define MAX_STARS 50
Star stars[MAX_STARS];

// Effect selector
enum Effect
{
  SCROLL_TEXT,
  PLASMA,
  PONG,
  MARIO_CLOCK,
  MATRIX_RAIN,
  FIRE,
  STARFIELD,
  IMAGE1,
  IMAGE2,
  IMAGE3,
  IMAGE4,
  IMAGE5,
  IMAGE6,
  NUM_EFFECTS
};
Effect currentEffect = SCROLL_TEXT;

CRGB ColorFromCurrentPalette(uint8_t index = 0, uint8_t brightness = 255, TBlendType blendType = LINEARBLEND)
{
  return ColorFromPalette(currentPalette, index, brightness, blendType);
}

void initMatrixRain()
{
  for (int i = 0; i < MAX_DROPS; i++)
  {
    drops[i].x = random(PANE_WIDTH);
    drops[i].y = random(-50, 0);
    drops[i].speed = random(1, 4);
    drops[i].length = random(5, 15);
    drops[i].active = true;
  }
}

void initStarfield()
{
  for (int i = 0; i < MAX_STARS; i++)
  {
    stars[i].x = random(-PANE_WIDTH, PANE_WIDTH);
    stars[i].y = random(-PANE_HEIGHT, PANE_HEIGHT);
    stars[i].z = random(1, PANE_WIDTH);
  }
}

void initPong()
{
  ballX = PANE_WIDTH / 2;
  ballY = PANE_HEIGHT / 2;
  ballSpeedX = 0.8;
  ballSpeedY = 0.6;
  paddle1Y = PANE_HEIGHT / 2;
  paddle2Y = PANE_HEIGHT / 2;
  score1 = 0;
  score2 = 0;
}

void drawScrollText()
{
  if (!scrollCompleted)
  {
    dma_display->fillScreenRGB888(0, 0, 0);

    dma_display->setTextColor(dma_display->color565(255, 255, 0));
    dma_display->setTextSize(3);
    dma_display->setTextWrap(false);

    dma_display->setCursor(scrollX, PANE_HEIGHT / 2 - 12);
    dma_display->print(scrollText);

    scrollX -= scrollSpeed;

    int totalWidth = scrollText.length() * 19;

    if (scrollX <= -totalWidth)
    {
      scrollCompleted = true;
      Serial.println("=== SCROLL COMPLETATO! ===");
    }
  }
}

// Funzione per disegnare sprite RGB565 da assets.h
void drawSprite(const uint16_t *sprite, int x, int y, int width, int height)
{
  for (int dy = 0; dy < height; dy++)
  {
    for (int dx = 0; dx < width; dx++)
    {
      uint16_t color = pgm_read_word(&sprite[dy * width + dx]);

      // Salta i pixel trasparenti (SKY_COLOR / _MASK)
      if (color != _MASK)
      {
        // Converti da RGB565 a RGB888
        uint8_t r = (color >> 11) << 3;
        uint8_t g = ((color >> 5) & 0x3F) << 2;
        uint8_t b = (color & 0x1F) << 3;

        dma_display->drawPixelRGB888(x + dx, y + dy, r, g, b);
      }
    }
  }
}

// Funzione per disegnare il terreno
void drawGround()
{
  for (int x = 0; x < PANE_WIDTH; x++)
  {
    for (int y = 0; y < 8; y++)
    {
      int idx = (y * 8 + (x % 8));
      if (idx < 64)
      {
        uint16_t color = GROUND[idx];
        uint8_t r = (color >> 11) << 3;
        uint8_t g = ((color >> 5) & 0x3F) << 2;
        uint8_t b = (color & 0x1F) << 3;
        dma_display->drawPixelRGB888(x, PANE_HEIGHT - 8 + y, r, g, b);
      }
    }
  }
}
void initMarioClock() {
  // Inizializza Mario
  marioSprite.x = 23;  // Posizione centrale iniziale
  marioSprite.y = 40;
  marioSprite.firstY = 40;
  marioSprite.isJumping = false;
  marioSprite.width = MARIO_IDLE_SIZE[0];
  marioSprite.height = MARIO_IDLE_SIZE[1];
  marioSprite.sprite = MARIO_IDLE;
  marioSprite.collisionDetected = false;
  marioSprite.direction = 1;
  marioSprite.lastY = 40;

  // Reset variabili sequenza
  currentJumpTarget = NONE;
  waitingForNextJump = false;
  nextJumpDelay = 0;

  // Inizializza blocco ore
  hourBlock.x = 13;
  hourBlock.y = 8;
  hourBlock.firstY = 8;
  hourBlock.width = 19;
  hourBlock.height = 19;
  hourBlock.isHit = false;
  hourBlock.direction = 1;
  hourBlock.moveAmount = 0;
  hourBlock.text = String(fakeHour);

  // Inizializza blocco minuti
  minuteBlock.x = 32;
  minuteBlock.y = 8;
  minuteBlock.firstY = 8;
  minuteBlock.width = 19;
  minuteBlock.height = 19;
  minuteBlock.isHit = false;
  minuteBlock.direction = 1;
  minuteBlock.moveAmount = 0;
  char minStr[3];
  sprintf(minStr, "%02d", fakeMin);
  minuteBlock.text = String(minStr);

  // Reset time
  fakeHour = 12;
  fakeMin = 30;
  fakeSec = 0;
  lastSecondUpdate = millis();
  lastMarioUpdate = millis();
  needsRedraw = true;
}


void drawBlock(MarioBlock &block)
{
  // Disegna il blocco
  drawSprite(BLOCK, block.x, block.y, BLOCK_SIZE[0], BLOCK_SIZE[1]);

  // Disegna il testo sul blocco come nel progetto originale
  dma_display->setFont(&Super_Mario_Bros__24pt7b);
  dma_display->setTextColor(0x0000); // Nero

  if (block.text.length() == 1)
  {
    dma_display->setCursor(block.x + 6, block.y + 12);  // y+15 invece di y+12
  }
  else
  {
    dma_display->setCursor(block.x + 2, block.y + 12);  // y+15 invece di y+12
  }

  dma_display->print(block.text);
}

// Funzione per aggiornare blocco (animazione hit)
void updateBlock(MarioBlock &block)
{
  static unsigned long lastHourUpdate = 0;
  static unsigned long lastMinuteUpdate = 0;
  
  unsigned long* blockTimer = (&block == &hourBlock) ? &lastHourUpdate : &lastMinuteUpdate;
  
  if (block.isHit)
  {
    if (millis() - *blockTimer >= 50)  // Aggiorna ogni 50ms come Mario
    {
      // Cancella posizione precedente CON MARGINE PIÙ AMPIO
      for (int dy = 0; dy < block.height + 6; dy++)
      {
        for (int dx = 0; dx < block.width + 2; dx++)
        {
          int px = block.x + dx - 1;
          int py = block.y + dy - 3;
          if (px >= 0 && px < PANE_WIDTH && py >= 0 && py < PANE_HEIGHT)
          {
            dma_display->drawPixelRGB888(px, py, 0, 145, 206);
          }
        }
      }

      // Muovi il blocco
      block.y += block.MOVE_PACE * (block.direction == 1 ? -1 : 1);
      block.moveAmount += block.MOVE_PACE;

      // Controlla se ha raggiunto il massimo movimento
      if (block.moveAmount >= block.MAX_MOVE && block.direction == 1)
      {
        block.direction = -1;
      }

      // Controlla se è tornato alla posizione iniziale
      if (block.y >= block.firstY && block.direction == -1)
      {
        block.y = block.firstY;
        block.isHit = false;
        block.direction = 1;
        block.moveAmount = 0;
      }

      // Ridisegna il blocco
      drawBlock(block);
      
      *blockTimer = millis();
    }
  }
}
// Controllo collisione Mario con blocchi
bool checkCollision(MarioSprite &mario, MarioBlock &block)
{
  return (mario.x < block.x + block.width &&
          mario.x + mario.width > block.x &&
          mario.y < block.y + block.height &&
          mario.y + mario.height > block.y);
}

// Funzione per fare saltare Mario
void marioJump(JumpTarget target) {
  if (!marioSprite.isJumping && !waitingForNextJump)
  {
    currentJumpTarget = target;
    
    // CANCELLA MARIO NELLA POSIZIONE ATTUALE PRIMA DI SPOSTARLO
    for (int dy = 0; dy < MARIO_IDLE_SIZE[1] + 2; dy++)
    {
      for (int dx = 0; dx < MARIO_IDLE_SIZE[0] + 2; dx++)
      {
        int px = marioSprite.x + dx - 1;
        int py = marioSprite.y + dy - 1;
        if (px >= 0 && px < PANE_WIDTH && py >= 0 && py < PANE_HEIGHT)
        {
          if (py >= PANE_HEIGHT - 8)
          {
            // Ridisegna il terreno
            int idx = ((py - (PANE_HEIGHT - 8)) * 8 + (px % 8));
            if (idx < 64)
            {
              uint16_t color = GROUND[idx];
              uint8_t r = (color >> 11) << 3;
              uint8_t g = ((color >> 5) & 0x3F) << 2;
              uint8_t b = (color & 0x1F) << 3;
              dma_display->drawPixelRGB888(px, py, r, g, b);
            }
          }
          else
          {
            dma_display->drawPixelRGB888(px, py, 0, 145, 206);
          }
        }
      }
    }
    
    // Posiziona Mario in base al target
    switch(target) {
      case HOUR_BLOCK:
        marioSprite.x = 18;  // Posizione per colpire blocco ore
        Serial.println("Mario targeting HOUR block");
        break;
      case MINUTE_BLOCK:
        marioSprite.x = 28;  // Posizione per colpire blocco minuti
        Serial.println("Mario targeting MINUTE block");
        break;
      case BOTH_BLOCKS:
        marioSprite.x = 18;  // Inizia dal blocco ore
        Serial.println("Mario targeting BOTH blocks - starting with HOUR");
        break;
      default:
        marioSprite.x = 23;  // Posizione centrale
        break;
    }
    
    marioSprite.isJumping = true;
    marioSprite.direction = 1; // UP
    marioSprite.lastY = marioSprite.y;
    marioSprite.width = MARIO_JUMP_SIZE[0];
    marioSprite.height = MARIO_JUMP_SIZE[1];
    marioSprite.sprite = MARIO_JUMP;
    marioSprite.collisionDetected = false;
    
    // Ridisegna Mario nella nuova posizione prima di saltare
    drawSprite(marioSprite.sprite, marioSprite.x, marioSprite.y, marioSprite.width, marioSprite.height);
  }
}

// Funzione per aggiornare Mario
void updateMario()
{
  // Gestisci il delay tra i salti
  if (waitingForNextJump && millis() >= nextJumpDelay)
  {
    waitingForNextJump = false;
    
    // CANCELLA MARIO PRIMA DI SPOSTARLO per il secondo salto
    for (int dy = 0; dy < MARIO_IDLE_SIZE[1] + 2; dy++)
    {
      for (int dx = 0; dx < MARIO_IDLE_SIZE[0] + 2; dx++)
      {
        int px = marioSprite.x + dx - 1;
        int py = marioSprite.y + dy - 1;
        if (px >= 0 && px < PANE_WIDTH && py >= 0 && py < PANE_HEIGHT)
        {
          if (py >= PANE_HEIGHT - 8)
          {
            int idx = ((py - (PANE_HEIGHT - 8)) * 8 + (px % 8));
            if (idx < 64)
            {
              uint16_t color = GROUND[idx];
              uint8_t r = (color >> 11) << 3;
              uint8_t g = ((color >> 5) & 0x3F) << 2;
              uint8_t b = (color & 0x1F) << 3;
              dma_display->drawPixelRGB888(px, py, r, g, b);
            }
          }
          else
          {
            dma_display->drawPixelRGB888(px, py, 0, 145, 206);
          }
        }
      }
    }
    
    // Secondo salto per colpire il blocco minuti
    marioSprite.x = 28;  // Sposta verso blocco minuti
    Serial.println("Mario jumping to MINUTE block");
    
    marioSprite.isJumping = true;
    marioSprite.direction = 1;
    marioSprite.lastY = marioSprite.y;
    marioSprite.width = MARIO_JUMP_SIZE[0];
    marioSprite.height = MARIO_JUMP_SIZE[1];
    marioSprite.sprite = MARIO_JUMP;
    marioSprite.collisionDetected = false;
    currentJumpTarget = MINUTE_BLOCK;  // Ora è solo il blocco minuti
    
    // Ridisegna Mario nella nuova posizione
    drawSprite(marioSprite.sprite, marioSprite.x, marioSprite.y, marioSprite.width, marioSprite.height);
  }
  
  if (marioSprite.isJumping)
  {
    if (millis() - lastMarioUpdate >= 50)
    {
      // CANCELLA SOLO MARIO nella posizione precedente
      for (int dy = 0; dy < marioSprite.height + 2; dy++)
      {
        for (int dx = 0; dx < marioSprite.width + 2; dx++)
        {
          int px = marioSprite.x + dx - 1;
          int py = marioSprite.y + dy - 1;
          if (px >= 0 && px < PANE_WIDTH && py >= 0 && py < PANE_HEIGHT)
          {
            // Colore cielo o terreno
            if (py >= PANE_HEIGHT - 8)
            {
              // Ridisegna il terreno
              int idx = ((py - (PANE_HEIGHT - 8)) * 8 + (px % 8));
              if (idx < 64)
              {
                uint16_t color = GROUND[idx];
                uint8_t r = (color >> 11) << 3;
                uint8_t g = ((color >> 5) & 0x3F) << 2;
                uint8_t b = (color & 0x1F) << 3;
                dma_display->drawPixelRGB888(px, py, r, g, b);
              }
            }
            else
            {
              // Colore cielo
              dma_display->drawPixelRGB888(px, py, 0, 145, 206);
            }
          }
        }
      }

      // Muovi Mario
      marioSprite.y += marioSprite.MARIO_PACE * (marioSprite.direction == 1 ? -1 : 1);

      // Controlla collisione con blocchi durante il salto verso l'alto
      if (marioSprite.direction == 1)
      {
        if (checkCollision(marioSprite, hourBlock) && !marioSprite.collisionDetected)
        {
          Serial.println("Collision with hour block!");
          hourBlock.isHit = true;
          marioSprite.direction = -1;
          marioSprite.collisionDetected = true;
        }
        else if (checkCollision(marioSprite, minuteBlock) && !marioSprite.collisionDetected)
        {
          Serial.println("Collision with minute block!");
          minuteBlock.isHit = true;
          marioSprite.direction = -1;
          marioSprite.collisionDetected = true;
        }
      }

      // Controlla se ha raggiunto l'altezza massima del salto
      if ((marioSprite.lastY - marioSprite.y) >= marioSprite.JUMP_HEIGHT && marioSprite.direction == 1)
      {
        marioSprite.direction = -1;
      }

      // Controlla se è tornato a terra
      if (marioSprite.y + marioSprite.height >= 56 && marioSprite.direction == -1)
      {
        // Cancella Mario nella posizione corrente prima di cambiare sprite
        for (int dy = 0; dy < marioSprite.height + 2; dy++)
        {
          for (int dx = 0; dx < marioSprite.width + 2; dx++)
          {
            int px = marioSprite.x + dx - 1;
            int py = marioSprite.y + dy - 1;
            if (px >= 0 && px < PANE_WIDTH && py >= 0 && py < PANE_HEIGHT)
            {
              if (py >= PANE_HEIGHT - 8)
              {
                int idx = ((py - (PANE_HEIGHT - 8)) * 8 + (px % 8));
                if (idx < 64)
                {
                  uint16_t color = GROUND[idx];
                  uint8_t r = (color >> 11) << 3;
                  uint8_t g = ((color >> 5) & 0x3F) << 2;
                  uint8_t b = (color & 0x1F) << 3;
                  dma_display->drawPixelRGB888(px, py, r, g, b);
                }
              }
              else
              {
                dma_display->drawPixelRGB888(px, py, 0, 145, 206);
              }
            }
          }
        }
        
        marioSprite.y = marioSprite.firstY;
        marioSprite.isJumping = false;
        marioSprite.width = MARIO_IDLE_SIZE[0];
        marioSprite.height = MARIO_IDLE_SIZE[1];
        marioSprite.sprite = MARIO_IDLE;
        
        // Controlla se deve fare un secondo salto
        if (currentJumpTarget == BOTH_BLOCKS)
        {
          Serial.println("Preparing second jump...");
          waitingForNextJump = true;
          nextJumpDelay = millis() + 300;  // Attendi 300ms prima del secondo salto
        }
        else
        {
          currentJumpTarget = NONE;
          needsRedraw = true;
        }
      }

      // Ridisegna Mario nella nuova posizione
      drawSprite(marioSprite.sprite, marioSprite.x, marioSprite.y, marioSprite.width, marioSprite.height);

      lastMarioUpdate = millis();
    }
  }
}
void drawMarioClock()
{
  // Aggiorna l'orario ogni secondo
  if (millis() - lastSecondUpdate >= 1000)
  {
    lastSecondUpdate = millis();
    fakeSec++;

    // Cambia minuti ogni 5 secondi per vedere l'animazione
    if (fakeSec >= 5)
    {
      fakeSec = 0;
      
      int oldHour = fakeHour;
      int oldMin = fakeMin;
      
      fakeMin++;

      if (fakeMin >= 60)
      {
        fakeMin = 0;
        fakeHour++;

        if (fakeHour >= 24)
        {
          fakeHour = 0;
        }
      }

      // IMPORTANTE: Aggiorna il testo sui blocchi PRIMA di decidere quale colpire
      hourBlock.text = String(fakeHour);
      char minStr[3];
      sprintf(minStr, "%02d", fakeMin);
      minuteBlock.text = String(minStr);

      // Decidi quale blocco colpire in base a cosa è cambiato
      if (oldHour != fakeHour && oldMin != fakeMin)
      {
        // Cambiano sia ore che minuti (es: 23:59 -> 00:00)
        Serial.println("Hour AND minute changed - jump to BOTH");
        marioJump(BOTH_BLOCKS);
      }
      else if (oldHour != fakeHour)
      {
        // Cambia solo l'ora
        Serial.println("Hour changed - jump to HOUR block");
        marioJump(HOUR_BLOCK);
      }
      else
      {
        // Cambia solo il minuto
        Serial.println("Minute changed - jump to MINUTE block");
        marioJump(MINUTE_BLOCK);
      }
    }
  }

  // Ridisegna la scena completa quando necessario
  if (needsRedraw && !marioSprite.isJumping && !waitingForNextJump)
  {
    // Sfondo cielo
    dma_display->fillScreenRGB888(0, 145, 206);

    // Disegna elementi decorativi
    drawSprite(HILL, 0, 34, 20, 22);
    drawSprite(BUSH, 43, 47, 21, 9);
    drawSprite(CLOUD1, 0, 21, 13, 12);
    drawSprite(CLOUD2, 51, 7, 13, 12);

    // Disegna il terreno
    drawGround();

    // Disegna i blocchi con le cifre
    drawBlock(hourBlock);
    drawBlock(minuteBlock);

    // Disegna Mario
    drawSprite(MARIO_IDLE, marioSprite.x, marioSprite.y, MARIO_IDLE_SIZE[0], MARIO_IDLE_SIZE[1]);

    needsRedraw = false;
  }

  // Aggiorna l'animazione di Mario se sta saltando
  updateMario();
  
  // IMPORTANTE: Aggiorna SEMPRE i blocchi, anche quando Mario non salta
  updateBlock(hourBlock);
  updateBlock(minuteBlock);
}
void switchEffect()
{
  //currentEffect = (Effect)((currentEffect + 1) % NUM_EFFECTS);
  currentEffect = MARIO_CLOCK; // Per testare sempre Mario Clock
  time_counter = 0;
  cycles = 0;
  scrollCompleted = false;
  return;

  switch (currentEffect)
  {
  case SCROLL_TEXT:
    Serial.println("Switching to SCROLL TEXT...");
    textWidth = scrollText.length() * 19;
    scrollX = PANE_WIDTH;
    scrollCompleted = false;
    dma_display->fillScreenRGB888(0, 0, 0);
    break;

  case PLASMA:
    Serial.println("Switching to PLASMA effect...");
    currentPalette = palettes[random(0, sizeof(palettes) / sizeof(palettes[0]))];
    dma_display->fillScreenRGB888(0, 0, 0);
    break;

  case PONG:
    Serial.println("Switching to PONG game...");
    initPong();
    dma_display->fillScreenRGB888(0, 0, 0);
    break;

  case MARIO_CLOCK:
  Serial.println("Switching to MARIO CLOCK...");
  initMarioClock();  // <-- Usa la funzione invece di duplicare il codice
  drawMarioClock();
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
    Serial.println("Switching to IMAGE1 display...");
    dma_display->fillScreenRGB888(0, 0, 0);
    draw_bosco(dma_display, 0, 0);
    break;

  case IMAGE2:
    Serial.println("Switching to IMAGE2 display...");
    dma_display->fillScreenRGB888(0, 0, 0);
    draw_casa(dma_display, 0, 0);
    break;

  case IMAGE3:
    Serial.println("Switching to IMAGE3 display...");
    dma_display->fillScreenRGB888(0, 0, 0);
    draw_mario(dma_display, 0, 0);
    break;

  case IMAGE4:
    Serial.println("Switching to IMAGE4 display...");
    dma_display->fillScreenRGB888(0, 0, 0);
    draw_paese(dma_display, 0, 0);
    break;

  case IMAGE5:
    Serial.println("Switching to IMAGE5 display...");
    dma_display->fillScreenRGB888(0, 0, 0);
    draw_pokemon(dma_display, 0, 0);
    break;

  case IMAGE6:
    Serial.println("Switching to IMAGE6 display...");
    dma_display->fillScreenRGB888(0, 0, 0);
    draw_andre(dma_display, 0, 0);
    break;

  default:
    break;
  }

  effect_timer = millis();
}


void setup()
{
  Serial.begin(115200);

  Serial.println(F("*****************************************************"));
  Serial.println(F("*        ESP32 LED Matrix - Multiple Effects        *"));
  Serial.println(F("*****************************************************"));

  HUB75_I2S_CFG mxconfig;
  mxconfig.mx_height = PANEL_HEIGHT;
  mxconfig.chain_length = PANELS_NUMBER;
  mxconfig.gpio.e = PIN_E;

  mxconfig.clkphase = false;
  mxconfig.latch_blanking = 4;

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->setBrightness8(200);

  if (!dma_display->begin())
    Serial.println("****** !KABOOM! I2S memory allocation failed ***********");

  dma_display->fillScreenRGB888(0, 0, 0);
  // AGGIUNGI QUESTA RIGA:
  initMarioClock();  // <-- Inizializza le strutture di Mario Clock
  currentEffect = PONG;
  Serial.println("Starting with SCROLL TEXT effect...");
  currentPalette = RainbowColors_p;

  fps_timer = millis();
  effect_timer = millis();
}

void drawPlasma()
{
  for (int x = 0; x < PANE_WIDTH; x++)
  {
    for (int y = 0; y < PANE_HEIGHT; y++)
    {
      int16_t v = 128;
      uint8_t wibble = sin8(time_counter);
      v += sin16(x * wibble * 3 + time_counter);
      v += cos16(y * (128 - wibble) + time_counter);
      v += sin16(y * x * cos8(-time_counter) / 8);

      currentColor = ColorFromPalette(currentPalette, (v >> 8));
      dma_display->drawPixelRGB888(x, y, currentColor.r, currentColor.g, currentColor.b);
    }
  }

  if (cycles >= 1024)
  {
    currentPalette = palettes[random(0, sizeof(palettes) / sizeof(palettes[0]))];
  }
}

void drawPong()
{
  dma_display->fillScreenRGB888(0, 0, 0);

  ballX += ballSpeedX;
  ballY += ballSpeedY;

  if (ballY <= 0 || ballY >= PANE_HEIGHT - 1)
  {
    ballSpeedY = -ballSpeedY;
  }

  if (ballX <= paddleWidth && ballY >= paddle1Y && ballY <= paddle1Y + paddleHeight)
  {
    ballSpeedX = abs(ballSpeedX);
  }
  if (ballX >= PANE_WIDTH - paddleWidth - 1 && ballY >= paddle2Y && ballY <= paddle2Y + paddleHeight)
  {
    ballSpeedX = -abs(ballSpeedX);
  }

  if (ballX < 0)
  {
    score2++;
    ballX = PANE_WIDTH / 2;
    ballY = PANE_HEIGHT / 2;
  }
  if (ballX >= PANE_WIDTH)
  {
    score1++;
    ballX = PANE_WIDTH / 2;
    ballY = PANE_HEIGHT / 2;
  }

  if (ballY > paddle1Y + paddleHeight / 2)
    paddle1Y++;
  if (ballY < paddle1Y + paddleHeight / 2)
    paddle1Y--;
  if (ballY > paddle2Y + paddleHeight / 2)
    paddle2Y++;
  if (ballY < paddle2Y + paddleHeight / 2)
    paddle2Y--;

  paddle1Y = constrain(paddle1Y, 0, PANE_HEIGHT - paddleHeight);
  paddle2Y = constrain(paddle2Y, 0, PANE_HEIGHT - paddleHeight);

  for (int i = 0; i < paddleHeight; i++)
  {
    for (int w = 0; w < paddleWidth; w++)
    {
      dma_display->drawPixelRGB888(w, paddle1Y + i, 0, 255, 0);
      dma_display->drawPixelRGB888(PANE_WIDTH - paddleWidth + w, paddle2Y + i, 255, 0, 0);
    }
  }

  dma_display->drawPixelRGB888((int)ballX, (int)ballY, 255, 255, 255);
  dma_display->drawPixelRGB888((int)ballX + 1, (int)ballY, 255, 255, 255);
  dma_display->drawPixelRGB888((int)ballX, (int)ballY + 1, 255, 255, 255);
  dma_display->drawPixelRGB888((int)ballX + 1, (int)ballY + 1, 255, 255, 255);

  for (int y = 0; y < PANE_HEIGHT; y += 4)
  {
    dma_display->drawPixelRGB888(PANE_WIDTH / 2, y, 50, 50, 50);
  }
}

void drawMatrixRain()
{
  for (int x = 0; x < PANE_WIDTH; x++)
  {
    for (int y = 0; y < PANE_HEIGHT; y++)
    {
      dma_display->drawPixelRGB888(x, y, 0, 2, 0);
    }
  }

  for (int i = 0; i < MAX_DROPS; i++)
  {
    if (drops[i].active)
    {
      for (int j = 0; j < drops[i].length; j++)
      {
        int y = drops[i].y - j;
        if (y >= 0 && y < PANE_HEIGHT)
        {
          int brightness = 255 - (j * 20);
          if (brightness < 0)
            brightness = 0;
          dma_display->drawPixelRGB888(drops[i].x, y, 0, brightness, 0);
        }
      }

      drops[i].y += drops[i].speed;

      if (drops[i].y > PANE_HEIGHT + drops[i].length)
      {
        drops[i].y = random(-20, 0);
        drops[i].x = random(PANE_WIDTH);
        drops[i].speed = random(1, 4);
        drops[i].length = random(5, 15);
      }
    }
  }
}

void drawFire()
{
  for (int x = 0; x < PANE_WIDTH; x++)
  {
    for (int y = 0; y < PANE_HEIGHT; y++)
    {
      heat[x][y] = qsub8(heat[x][y], random8(0, ((55 * 10) / PANE_HEIGHT) + 2));
    }
  }

  for (int x = 0; x < PANE_WIDTH; x++)
  {
    for (int y = 0; y < PANE_HEIGHT - 1; y++)
    {
      heat[x][y] = (heat[x][y + 1] + heat[x][y + 1] + heat[x][y + 1]) / 3;
    }
  }

  if (random8() < 120)
  {
    int x = random(PANE_WIDTH);
    heat[x][PANE_HEIGHT - 1] = qadd8(heat[x][PANE_HEIGHT - 1], random8(160, 255));
  }

  for (int x = 0; x < PANE_WIDTH; x++)
  {
    for (int y = 0; y < PANE_HEIGHT; y++)
    {
      CRGB color = HeatColor(heat[x][y]);
      dma_display->drawPixelRGB888(x, y, color.r, color.g, color.b);
    }
  }
}

void drawStarfield()
{
  dma_display->fillScreenRGB888(0, 0, 5);

  for (int i = 0; i < MAX_STARS; i++)
  {
    stars[i].z -= 0.5;
    if (stars[i].z <= 0)
    {
      stars[i].x = random(-PANE_WIDTH, PANE_WIDTH);
      stars[i].y = random(-PANE_HEIGHT, PANE_HEIGHT);
      stars[i].z = PANE_WIDTH;
    }

    int sx = (stars[i].x / stars[i].z) * PANE_WIDTH + PANE_WIDTH / 2;
    int sy = (stars[i].y / stars[i].z) * PANE_HEIGHT + PANE_HEIGHT / 2;

    if (sx >= 0 && sx < PANE_WIDTH && sy >= 0 && sy < PANE_HEIGHT)
    {
      int brightness = map(stars[i].z, 0, PANE_WIDTH, 255, 50);
      dma_display->drawPixelRGB888(sx, sy, brightness, brightness, brightness);
    }
  }
}

void loop()
{
  bool shouldSwitch = false;

  if (currentEffect == SCROLL_TEXT)
  {
    shouldSwitch = scrollCompleted;
  }
  else
  {
    shouldSwitch = (millis() - effect_timer >= 10000);
  }

  if (shouldSwitch)
  {
    switchEffect();
  }

  switch (currentEffect)
  {
  case SCROLL_TEXT:
    drawScrollText();
    break;
  case PLASMA:
    drawPlasma();
    break;
  case PONG:
    drawPong();
    break;
  case MARIO_CLOCK:
    drawMarioClock();
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

  if (cycles >= 1024)
  {
    time_counter = 0;
    cycles = 0;
  }

  if (fps_timer + 5000 < millis())
  {
    Serial.printf_P(PSTR("Effect fps: %d\n"), fps / 5);
    fps_timer = millis();
    fps = 0;
  }

  delay(20);
}