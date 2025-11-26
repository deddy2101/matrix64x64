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
bool walkingToJump = false;  // true se sta camminando per saltare, false se sta tornando al centro


// Dopo le variabili globali esistenti di Mario
enum MarioState {
  IDLE,
  WALKING,
  JUMPING
};

MarioState marioState = IDLE;
int marioTargetX = 23;  // Posizione X di destinazione
bool marioFacingRight = true;  // Direzione di Mario
const int WALK_SPEED = 2;  // Pixel per frame di camminata

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
int fakeMin = 49;
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

// Funzione per disegnare sprite specchiato orizzontalmente
void drawSpriteFlipped(const uint16_t *sprite, int x, int y, int width, int height, bool flipH)
{
  for (int dy = 0; dy < height; dy++)
  {
    for (int dx = 0; dx < width; dx++)
    {
      int srcX = flipH ? (width - 1 - dx) : dx;  // Specchia orizzontalmente se flipH è true
      uint16_t color = pgm_read_word(&sprite[dy * width + srcX]);

      if (color != _MASK)
      {
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
  marioSprite.x = 23;
  marioSprite.y = 40;
  marioSprite.firstY = 40;
  marioSprite.width = MARIO_IDLE_SIZE[0];
  marioSprite.height = MARIO_IDLE_SIZE[1];
  marioSprite.sprite = MARIO_IDLE;
  marioSprite.collisionDetected = false;
  marioSprite.direction = 1;
  marioSprite.lastY = 40;
  
  marioState = IDLE;
  marioTargetX = 23;
  marioFacingRight = true;

  // Reset variabili sequenza
  currentJumpTarget = NONE;
  waitingForNextJump = false;
  nextJumpDelay = 0;

  // Inizializza blocco ore - SPOSTATO PIÙ A SINISTRA
  hourBlock.x = 8;   // Era 13, ora 8
  hourBlock.y = 8;
  hourBlock.firstY = 8;
  hourBlock.width = 19;
  hourBlock.height = 19;
  hourBlock.isHit = false;
  hourBlock.direction = 1;
  hourBlock.moveAmount = 0;
  hourBlock.text = String(fakeHour);

  // Inizializza blocco minuti - SPOSTATO PIÙ A DESTRA
  minuteBlock.x = 37;  // Era 32, ora 37
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
  fakeMin = 55;
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
// Funzione per ridisegnare lo sfondo nella posizione specificata
void redrawBackground(int x, int y, int width, int height)
{
  for (int dy = 0; dy < height; dy++)
  {
    for (int dx = 0; dx < width; dx++)
    {
      int px = x + dx;
      int py = y + dy;
      
      if (px >= 0 && px < PANE_WIDTH && py >= 0 && py < PANE_HEIGHT)
      {
        // Colore di default: cielo o terreno
        uint8_t r = 0, g = 145, b = 206;
        
        // Controlla se siamo nel terreno
        if (py >= PANE_HEIGHT - 8)
        {
          int idx = ((py - (PANE_HEIGHT - 8)) * 8 + (px % 8));
          if (idx < 64)
          {
            uint16_t color = GROUND[idx];
            r = (color >> 11) << 3;
            g = ((color >> 5) & 0x3F) << 2;
            b = (color & 0x1F) << 3;
          }
        }
        // Controlla se siamo nella collina (x: 0-20, y: 34-56)
        else if (px >= 0 && px < 20 && py >= 34 && py < 56)
        {
          int hillX = px;
          int hillY = py - 34;
          if (hillY < 22 && hillX < 20)
          {
            uint16_t color = pgm_read_word(&HILL[hillY * 20 + hillX]);
            if (color != _MASK)
            {
              r = (color >> 11) << 3;
              g = ((color >> 5) & 0x3F) << 2;
              b = (color & 0x1F) << 3;
            }
          }
        }
        // Controlla se siamo nel cespuglio (x: 43-64, y: 47-56)
        else if (px >= 43 && px < 64 && py >= 47 && py < 56)
        {
          int bushX = px - 43;
          int bushY = py - 47;
          if (bushY < 9 && bushX < 21)
          {
            uint16_t color = pgm_read_word(&BUSH[bushY * 21 + bushX]);
            if (color != _MASK)
            {
              r = (color >> 11) << 3;
              g = ((color >> 5) & 0x3F) << 2;
              b = (color & 0x1F) << 3;
            }
          }
        }
        // Controlla se siamo nella nuvola 1 (x: 0-13, y: 21-33)
        else if (px >= 0 && px < 13 && py >= 21 && py < 33)
        {
          int cloudX = px;
          int cloudY = py - 21;
          if (cloudY < 12 && cloudX < 13)
          {
            uint16_t color = pgm_read_word(&CLOUD1[cloudY * 13 + cloudX]);
            if (color != _MASK)
            {
              r = (color >> 11) << 3;
              g = ((color >> 5) & 0x3F) << 2;
              b = (color & 0x1F) << 3;
            }
          }
        }
        // Controlla se siamo nella nuvola 2 (x: 51-64, y: 7-19)
        else if (px >= 51 && px < 64 && py >= 7 && py < 19)
        {
          int cloudX = px - 51;
          int cloudY = py - 7;
          if (cloudY < 12 && cloudX < 13)
          {
            uint16_t color = pgm_read_word(&CLOUD2[cloudY * 13 + cloudX]);
            if (color != _MASK)
            {
              r = (color >> 11) << 3;
              g = ((color >> 5) & 0x3F) << 2;
              b = (color & 0x1F) << 3;
            }
          }
        }
        
        dma_display->drawPixelRGB888(px, py, r, g, b);
      }
    }
  }
}
// Funzione helper per iniziare il salto
void startJump() {
  marioState = JUMPING;
  marioSprite.direction = 1; // UP
  marioSprite.lastY = marioSprite.y;
  marioSprite.width = MARIO_JUMP_SIZE[0];
  marioSprite.height = MARIO_JUMP_SIZE[1];
  marioSprite.sprite = MARIO_JUMP;
  marioSprite.collisionDetected = false;
}

// Funzione per fare saltare Mario
void marioJump(JumpTarget target) {
  if (marioState == IDLE && !waitingForNextJump)
  {
    currentJumpTarget = target;
    walkingToJump = true;
    
    // Determina la posizione target in base al blocco da colpire
    switch(target) {
      case HOUR_BLOCK:
        marioTargetX = 8;  // Era 11, ora 7 (4 pixel più a sinistra)
        Serial.println("Mario walking to HOUR block");
        break;
      case MINUTE_BLOCK:
        marioTargetX = 40;
        Serial.println("Mario walking to MINUTE block");
        break;
      case BOTH_BLOCKS:
        marioTargetX = 8;  // Era 11, ora 7
        Serial.println("Mario walking to HOUR block first");
        break;
      default:
        marioTargetX = 23;
        break;
    }
    
    // Determina la direzione
    marioFacingRight = (marioTargetX > marioSprite.x);
    
    // Se Mario è già nella posizione giusta, salta direttamente
    if (abs(marioSprite.x - marioTargetX) <= WALK_SPEED)
    {
      marioSprite.x = marioTargetX;
      startJump();
      walkingToJump = false;
    }
    else
    {
      // Inizia a camminare
      marioState = WALKING;
      marioSprite.width = MARIO_IDLE_SIZE[0];
      marioSprite.height = MARIO_IDLE_SIZE[1];
      marioSprite.sprite = MARIO_IDLE;
    }
  }
}



// Modifica updateMarioWalk():
void updateMarioWalk()
{
  static unsigned long lastWalkUpdate = 0;
  
  if (marioState == WALKING)
  {
    if (millis() - lastWalkUpdate >= 50)
    {
      // Cancella Mario nella posizione attuale
      redrawBackground(marioSprite.x - 1, marioSprite.y - 1, 
                       marioSprite.width + 2, marioSprite.height + 2);
      
      // Muovi Mario verso il target
      if (abs(marioSprite.x - marioTargetX) <= WALK_SPEED)
      {
        // Arrivato a destinazione
        marioSprite.x = marioTargetX;
        
        // CONTROLLA SE DEVE SALTARE O FERMARSI
        if (walkingToJump)
        {
            // Inizia il salto
            startJump();
            walkingToJump = false;  // Reset flag
        }
        else
        {
            // Si ferma semplicemente
            marioState = IDLE;
            needsRedraw = true;
        }
      }
      else
      {
        // Continua a camminare...
        if (marioSprite.x < marioTargetX)
        {
          marioSprite.x += WALK_SPEED;
        }
        else
        {
          marioSprite.x -= WALK_SPEED;
        }
        
        drawBlock(hourBlock);
        drawBlock(minuteBlock);
        
        drawSpriteFlipped(marioSprite.sprite, marioSprite.x, marioSprite.y, 
                          marioSprite.width, marioSprite.height, !marioFacingRight);
      }
      
      lastWalkUpdate = millis();
    }
  }
}
// Funzione per aggiornare Mario
void updateMario()
{
  // Gestisci il delay tra i salti
  if (waitingForNextJump && millis() >= nextJumpDelay)
  {
    waitingForNextJump = false;
    
    // AGGIORNA LA CIFRA DEL BLOCCO MINUTI QUI (prima di saltare)
    char minStr[3];
    sprintf(minStr, "%02d", fakeMin);
    minuteBlock.text = String(minStr);
    
    // Secondo salto per colpire il blocco minuti
    currentJumpTarget = MINUTE_BLOCK;
    marioTargetX = 40;
    marioFacingRight = (marioTargetX > marioSprite.x);
    walkingToJump = true;
    
    Serial.println("Mario walking to MINUTE block");
    
    if (abs(marioSprite.x - marioTargetX) <= WALK_SPEED)
    {
      marioSprite.x = marioTargetX;
      startJump();
      walkingToJump = false;
    }
    else
    {
      marioState = WALKING;
      marioSprite.width = MARIO_IDLE_SIZE[0];
      marioSprite.height = MARIO_IDLE_SIZE[1];
      marioSprite.sprite = MARIO_IDLE;
    }
  }
  
  if (marioState == WALKING)
  {
    updateMarioWalk();
    return;
  }
  
  if (marioState == JUMPING)
  {
    if (millis() - lastMarioUpdate >= 50)
    {
      // CANCELLA MARIO usando la funzione che ridisegna lo sfondo
      redrawBackground(marioSprite.x - 1, marioSprite.y - 1, 
                       marioSprite.width + 2, marioSprite.height + 2);

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
          
          // ← AGGIORNA LA CIFRA DELLE ORE SUBITO DOPO LA COLLISIONE
          hourBlock.text = String(fakeHour);
          needsRedraw = true;
        }
        else if (checkCollision(marioSprite, minuteBlock) && !marioSprite.collisionDetected)
        {
          Serial.println("Collision with minute block!");
          minuteBlock.isHit = true;
          marioSprite.direction = -1;
          marioSprite.collisionDetected = true;
          
          // ← AGGIORNA LA CIFRA DEI MINUTI SUBITO DOPO LA COLLISIONE
          char minStr[3];
          sprintf(minStr, "%02d", fakeMin);
          minuteBlock.text = String(minStr);
          needsRedraw = true;
        }
      }

      // ← QUESTA PARTE ERA STATA CANCELLATA! ←
      // Controlla se ha raggiunto l'altezza massima del salto
      if ((marioSprite.lastY - marioSprite.y) >= marioSprite.JUMP_HEIGHT && marioSprite.direction == 1)
      {
        marioSprite.direction = -1;
      }

      // Controlla se è tornato a terra
      if (marioSprite.y + marioSprite.height >= 56 && marioSprite.direction == -1)
      {
        // Cancella Mario usando la funzione che ridisegna lo sfondo
        redrawBackground(marioSprite.x - 1, marioSprite.y - 1, 
                         marioSprite.width + 2, marioSprite.height + 2);
        
        marioSprite.y = marioSprite.firstY;
        marioState = IDLE;
        marioSprite.width = MARIO_IDLE_SIZE[0];
        marioSprite.height = MARIO_IDLE_SIZE[1];
        marioSprite.sprite = MARIO_IDLE;
        
        // Controlla se deve fare un secondo salto
        if (currentJumpTarget == BOTH_BLOCKS)
        {
          Serial.println("Preparing for second jump...");
          waitingForNextJump = true;
          nextJumpDelay = millis() + 300;
        }
        else
        {
          currentJumpTarget = NONE;
          walkingToJump = false;  // NON deve saltare quando torna al centro
          
          // Torna alla posizione centrale dopo aver finito
          marioTargetX = 23;
          marioFacingRight = (marioTargetX > marioSprite.x);
          
          if (abs(marioSprite.x - marioTargetX) > WALK_SPEED)
          {
            marioState = WALKING;
          }
          else
          {
            marioSprite.x = 23;  // Posizionalo direttamente al centro
            needsRedraw = true;
          }
        }
      }
      // ← FINE DELLA PARTE CHE ERA STATA CANCELLATA ←

      // Ridisegna Mario nella nuova posizione (con orientamento corretto)
      drawSpriteFlipped(marioSprite.sprite, marioSprite.x, marioSprite.y, 
                        marioSprite.width, marioSprite.height, !marioFacingRight);

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
      //hourBlock.text = String(fakeHour);
      //char minStr[3];
      //sprintf(minStr, "%02d", fakeMin);
      //minuteBlock.text = String(minStr);

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
   if (needsRedraw && marioState == IDLE && !waitingForNextJump)
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

    // Disegna Mario con orientamento corretto
    drawSpriteFlipped(MARIO_IDLE, marioSprite.x, marioSprite.y, 
                      MARIO_IDLE_SIZE[0], MARIO_IDLE_SIZE[1], !marioFacingRight);

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