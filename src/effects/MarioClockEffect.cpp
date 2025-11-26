#include "MarioClockEffect.h"

MarioClockEffect::MarioClockEffect(DisplayManager* dm) 
    : Effect(dm), 
      spriteRenderer(new SpriteRenderer(dm)),
      hourBlock(new MarioBlock()),
      minuteBlock(new MarioBlock()),
      marioSprite(new MarioSprite()),
      marioState(IDLE),
      currentJumpTarget(NONE),
      marioTargetX(23),
      marioFacingRight(true),
      walkingToJump(false),
      waitingForNextJump(false),
      nextJumpDelay(0),
      needsRedraw(true),
      fakeHour(12),
      fakeMin(55),
      fakeSec(0),
      lastSecondUpdate(0),
      lastMarioUpdate(0) {
}

MarioClockEffect::~MarioClockEffect() {
    delete spriteRenderer;
    delete hourBlock;
    delete minuteBlock;
    delete marioSprite;
}

void MarioClockEffect::init() {
    Serial.println("Initializing Mario Clock Effect");
    
    initMario();
    initBlocks();
    
    lastSecondUpdate = millis();
    lastMarioUpdate = millis();
    needsRedraw = true;
}

void MarioClockEffect::initMario() {
    marioSprite->x = 23;
    marioSprite->y = 40;
    marioSprite->firstY = 40;
    marioSprite->width = MARIO_IDLE_SIZE[0];
    marioSprite->height = MARIO_IDLE_SIZE[1];
    marioSprite->sprite = MARIO_IDLE;
    marioSprite->collisionDetected = false;
    marioSprite->direction = 1;
    marioSprite->lastY = 40;
    
    marioState = IDLE;
    marioTargetX = 23;
    marioFacingRight = true;
}

void MarioClockEffect::initBlocks() {
    // Blocco ore
    hourBlock->x = 8;
    hourBlock->y = 8;
    hourBlock->firstY = 8;
    hourBlock->width = 19;
    hourBlock->height = 19;
    hourBlock->isHit = false;
    hourBlock->direction = 1;
    hourBlock->moveAmount = 0;
    hourBlock->text = String(fakeHour);
    
    // Blocco minuti
    minuteBlock->x = 37;
    minuteBlock->y = 8;
    minuteBlock->firstY = 8;
    minuteBlock->width = 19;
    minuteBlock->height = 19;
    minuteBlock->isHit = false;
    minuteBlock->direction = 1;
    minuteBlock->moveAmount = 0;
    char minStr[3];
    sprintf(minStr, "%02d", fakeMin);
    minuteBlock->text = String(minStr);
    
    // Reset time
    currentJumpTarget = NONE;
    waitingForNextJump = false;
    nextJumpDelay = 0;
}

void MarioClockEffect::reset() {
    Effect::reset();
    fakeHour = 12;
    fakeMin = 55;
    fakeSec = 0;
}

void MarioClockEffect::update() {
    updateTime();
    updateMario();
    
    static unsigned long lastHourUpdate = 0;
    static unsigned long lastMinuteUpdate = 0;
    
    updateBlock(*hourBlock, lastHourUpdate);
    updateBlock(*minuteBlock, lastMinuteUpdate);
}

void MarioClockEffect::updateTime() {
    if (millis() - lastSecondUpdate >= 1000) {
        lastSecondUpdate = millis();
        fakeSec++;
        
        // Cambia minuti ogni 5 secondi per vedere l'animazione
        if (fakeSec >= 5) {
            fakeSec = 0;
            
            int oldHour = fakeHour;
            int oldMin = fakeMin;
            
            fakeMin++;
            
            if (fakeMin >= 60) {
                fakeMin = 0;
                fakeHour++;
                
                if (fakeHour >= 24) {
                    fakeHour = 0;
                }
            }
            
            // Decidi quale blocco colpire
            if (oldHour != fakeHour && oldMin != fakeMin) {
                Serial.println("Hour AND minute changed - jump to BOTH");
                marioJump(BOTH_BLOCKS);
            } else if (oldHour != fakeHour) {
                Serial.println("Hour changed - jump to HOUR block");
                marioJump(HOUR_BLOCK);
            } else {
                Serial.println("Minute changed - jump to MINUTE block");
                marioJump(MINUTE_BLOCK);
            }
        }
    }
}

void MarioClockEffect::draw() {
    // Ridisegna scena completa quando necessario
    if (needsRedraw && marioState == IDLE && !waitingForNextJump && 
        !hourBlock->isHit && !minuteBlock->isHit) {
        drawScene();
        needsRedraw = false;
    }
}

void MarioClockEffect::drawScene() {
    // Sfondo cielo
    displayManager->fillScreen(0, 145, 206);
    
    // Elementi decorativi
    spriteRenderer->drawSprite(HILL, 0, 34, 20, 22);
    spriteRenderer->drawSprite(BUSH, 43, 47, 21, 9);
    spriteRenderer->drawSprite(CLOUD1, 0, 21, 13, 12);
    spriteRenderer->drawSprite(CLOUD2, 51, 7, 13, 12);
    
    // Terreno
    drawGround();
    
    // Blocchi
    drawBlock(*hourBlock);
    drawBlock(*minuteBlock);
    
    // Mario
    spriteRenderer->drawSpriteFlipped(MARIO_IDLE, marioSprite->x, marioSprite->y,
                                     MARIO_IDLE_SIZE[0], MARIO_IDLE_SIZE[1], 
                                     !marioFacingRight);
}

void MarioClockEffect::drawGround() {
    int width = displayManager->getWidth();
    int height = displayManager->getHeight();
    
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < 8; y++) {
            int idx = (y * 8 + (x % 8));
            if (idx < 64) {
                uint16_t color = GROUND[idx];
                uint8_t r = (color >> 11) << 3;
                uint8_t g = ((color >> 5) & 0x3F) << 2;
                uint8_t b = (color & 0x1F) << 3;
                displayManager->drawPixel(x, height - 8 + y, r, g, b);
            }
        }
    }
}

void MarioClockEffect::drawBlock(MarioBlock& block) {
    spriteRenderer->drawSprite(BLOCK, block.x, block.y, BLOCK_SIZE[0], BLOCK_SIZE[1]);
    
    displayManager->setFont(&Super_Mario_Bros__24pt7b);
    displayManager->setTextColor(0x0000);
    
    if (block.text.length() == 1) {
        displayManager->setCursor(block.x + 6, block.y + 12);
    } else {
        displayManager->setCursor(block.x + 2, block.y + 12);
    }
    
    displayManager->print(block.text);
}

bool MarioClockEffect::checkCollision(MarioSprite& mario, MarioBlock& block) {
    return (mario.x < block.x + block.width &&
            mario.x + mario.width > block.x &&
            mario.y < block.y + block.height &&
            mario.y + mario.height > block.y);
}

void MarioClockEffect::updateBlock(MarioBlock& block, unsigned long& lastUpdate) {
    if (block.isHit) {
        if (millis() - lastUpdate >= 50) {
            // Cancella posizione precedente con margine
            for (int dy = 0; dy < block.height + 6; dy++) {
                for (int dx = 0; dx < block.width + 2; dx++) {
                    int px = block.x + dx - 1;
                    int py = block.y + dy - 3;
                    if (px >= 0 && px < displayManager->getWidth() && 
                        py >= 0 && py < displayManager->getHeight()) {
                        displayManager->drawPixel(px, py, 0, 145, 206);
                    }
                }
            }
            
            // Muovi il blocco
            block.y += BLOCK_MOVE_PACE * (block.direction == 1 ? -1 : 1);
            block.moveAmount += BLOCK_MOVE_PACE;
            
            // Controlla se ha raggiunto il massimo movimento
            if (block.moveAmount >= BLOCK_MAX_MOVE && block.direction == 1) {
                block.direction = -1;
            }
            
            // Controlla se è tornato alla posizione iniziale
            if (block.y >= block.firstY && block.direction == -1) {
                block.y = block.firstY;
                block.isHit = false;
                block.direction = 1;
                block.moveAmount = 0;
            }
            
            // Ridisegna il blocco
            drawBlock(block);
            lastUpdate = millis();
        }
    }
}

void MarioClockEffect::redrawBackground(int x, int y, int width, int height) {
    for (int dy = 0; dy < height; dy++) {
        for (int dx = 0; dx < width; dx++) {
            int px = x + dx;
            int py = y + dy;
            
            if (px >= 0 && px < displayManager->getWidth() && 
                py >= 0 && py < displayManager->getHeight()) {
                
                // Colore di default: cielo
                uint8_t r = 0, g = 145, b = 206;
                
                // Controlla se siamo nel terreno
                if (py >= displayManager->getHeight() - 8) {
                    int idx = ((py - (displayManager->getHeight() - 8)) * 8 + (px % 8));
                    if (idx < 64) {
                        uint16_t color = GROUND[idx];
                        r = (color >> 11) << 3;
                        g = ((color >> 5) & 0x3F) << 2;
                        b = (color & 0x1F) << 3;
                    }
                }
                // Collina (x: 0-20, y: 34-56)
                else if (px >= 0 && px < 20 && py >= 34 && py < 56) {
                    int hillX = px;
                    int hillY = py - 34;
                    if (hillY < 22 && hillX < 20) {
                        uint16_t color = pgm_read_word(&HILL[hillY * 20 + hillX]);
                        if (color != _MASK) {
                            r = (color >> 11) << 3;
                            g = ((color >> 5) & 0x3F) << 2;
                            b = (color & 0x1F) << 3;
                        }
                    }
                }
                // Cespuglio (x: 43-64, y: 47-56)
                else if (px >= 43 && px < 64 && py >= 47 && py < 56) {
                    int bushX = px - 43;
                    int bushY = py - 47;
                    if (bushY < 9 && bushX < 21) {
                        uint16_t color = pgm_read_word(&BUSH[bushY * 21 + bushX]);
                        if (color != _MASK) {
                            r = (color >> 11) << 3;
                            g = ((color >> 5) & 0x3F) << 2;
                            b = (color & 0x1F) << 3;
                        }
                    }
                }
                // Nuvola 1 (x: 0-13, y: 21-33)
                else if (px >= 0 && px < 13 && py >= 21 && py < 33) {
                    int cloudX = px;
                    int cloudY = py - 21;
                    if (cloudY < 12 && cloudX < 13) {
                        uint16_t color = pgm_read_word(&CLOUD1[cloudY * 13 + cloudX]);
                        if (color != _MASK) {
                            r = (color >> 11) << 3;
                            g = ((color >> 5) & 0x3F) << 2;
                            b = (color & 0x1F) << 3;
                        }
                    }
                }
                // Nuvola 2 (x: 51-64, y: 7-19)
                else if (px >= 51 && px < 64 && py >= 7 && py < 19) {
                    int cloudX = px - 51;
                    int cloudY = py - 7;
                    if (cloudY < 12 && cloudX < 13) {
                        uint16_t color = pgm_read_word(&CLOUD2[cloudY * 13 + cloudX]);
                        if (color != _MASK) {
                            r = (color >> 11) << 3;
                            g = ((color >> 5) & 0x3F) << 2;
                            b = (color & 0x1F) << 3;
                        }
                    }
                }
                
                displayManager->drawPixel(px, py, r, g, b);
            }
        }
    }
}

void MarioClockEffect::startJump() {
    marioState = JUMPING;
    marioSprite->direction = 1; // UP
    marioSprite->lastY = marioSprite->y;
    marioSprite->width = MARIO_JUMP_SIZE[0];
    marioSprite->height = MARIO_JUMP_SIZE[1];
    marioSprite->sprite = MARIO_JUMP;
    marioSprite->collisionDetected = false;
}

void MarioClockEffect::marioJump(JumpTarget target) {
    if (marioState == IDLE && !waitingForNextJump) {
        currentJumpTarget = target;
        walkingToJump = true;
        
        // Determina posizione target
        switch(target) {
            case HOUR_BLOCK:
                marioTargetX = 8;
                Serial.println("Mario walking to HOUR block");
                break;
            case MINUTE_BLOCK:
                marioTargetX = 40;
                Serial.println("Mario walking to MINUTE block");
                break;
            case BOTH_BLOCKS:
                marioTargetX = 8;
                Serial.println("Mario walking to HOUR block first");
                break;
            default:
                marioTargetX = 23;
                break;
        }
        
        // Determina direzione
        marioFacingRight = (marioTargetX > marioSprite->x);
        
        // Se è già nella posizione giusta, salta direttamente
        if (abs(marioSprite->x - marioTargetX) <= WALK_SPEED) {
            marioSprite->x = marioTargetX;
            startJump();
            walkingToJump = false;
        } else {
            // Inizia a camminare
            marioState = WALKING;
            marioSprite->width = MARIO_IDLE_SIZE[0];
            marioSprite->height = MARIO_IDLE_SIZE[1];
            marioSprite->sprite = MARIO_IDLE;
        }
    }
}

void MarioClockEffect::updateMarioWalk() {
    static unsigned long lastWalkUpdate = 0;
    
    if (marioState == WALKING) {
        if (millis() - lastWalkUpdate >= 50) {
            int oldX = marioSprite->x;
            
            // Calcola movimento
            if (abs(marioSprite->x - marioTargetX) <= WALK_SPEED) {
                marioSprite->x = marioTargetX;
                
                if (walkingToJump) {
                    startJump();
                    walkingToJump = false;
                } else {
                    marioState = IDLE;
                }
            } else {
                if (marioSprite->x < marioTargetX)
                    marioSprite->x += WALK_SPEED;
                else
                    marioSprite->x -= WALK_SPEED;
            }
            
            // Cancella solo la parte scoperta
            if (oldX != marioSprite->x) {
                if (oldX < marioSprite->x) {
                    // Mario si muove a destra -> cancella sinistra
                    redrawBackground(oldX, marioSprite->y, WALK_SPEED + 1, marioSprite->height);
                } else {
                    // Mario si muove a sinistra -> cancella destra
                    redrawBackground(marioSprite->x + marioSprite->width - 1, marioSprite->y, 
                                   WALK_SPEED + 1, marioSprite->height);
                }
                
                // Disegna Mario nella nuova posizione
                spriteRenderer->drawSpriteFlipped(marioSprite->sprite, marioSprite->x, marioSprite->y,
                                                 marioSprite->width, marioSprite->height, !marioFacingRight);
            }
            
            lastWalkUpdate = millis();
        }
    }
}

void MarioClockEffect::updateMario() {
    // Gestisci delay tra salti
    if (waitingForNextJump && millis() >= nextJumpDelay) {
        waitingForNextJump = false;
        
        // Aggiorna cifra minuti prima del secondo salto
        char minStr[3];
        sprintf(minStr, "%02d", fakeMin);
        minuteBlock->text = String(minStr);
        
        // Secondo salto per blocco minuti
        currentJumpTarget = MINUTE_BLOCK;
        marioTargetX = 40;
        marioFacingRight = (marioTargetX > marioSprite->x);
        walkingToJump = true;
        
        Serial.println("Mario walking to MINUTE block");
        
        if (abs(marioSprite->x - marioTargetX) <= WALK_SPEED) {
            marioSprite->x = marioTargetX;
            startJump();
            walkingToJump = false;
        } else {
            marioState = WALKING;
            marioSprite->width = MARIO_IDLE_SIZE[0];
            marioSprite->height = MARIO_IDLE_SIZE[1];
            marioSprite->sprite = MARIO_IDLE;
        }
    }
    
    if (marioState == WALKING) {
        updateMarioWalk();
        return;
    }
    
    if (marioState == JUMPING) {
        if (millis() - lastMarioUpdate >= 50) {
            // Cancella Mario
            redrawBackground(marioSprite->x - 1, marioSprite->y - 1,
                           marioSprite->width + 2, marioSprite->height + 2);
            
            // Muovi Mario
            marioSprite->y += MARIO_PACE * (marioSprite->direction == 1 ? -1 : 1);
            
            // Controlla collisione durante salto verso l'alto
            if (marioSprite->direction == 1) {
                if (checkCollision(*marioSprite, *hourBlock) && !marioSprite->collisionDetected) {
                    Serial.println("Collision with hour block!");
                    hourBlock->isHit = true;
                    marioSprite->direction = -1;
                    marioSprite->collisionDetected = true;
                    
                    // Aggiorna cifra ore
                    hourBlock->text = String(fakeHour);
                    needsRedraw = true;
                } else if (checkCollision(*marioSprite, *minuteBlock) && !marioSprite->collisionDetected) {
                    Serial.println("Collision with minute block!");
                    minuteBlock->isHit = true;
                    marioSprite->direction = -1;
                    marioSprite->collisionDetected = true;
                    
                    // Aggiorna cifra minuti
                    char minStr[3];
                    sprintf(minStr, "%02d", fakeMin);
                    minuteBlock->text = String(minStr);
                    needsRedraw = true;
                }
            }
            
            // Controlla altezza massima salto
            if ((marioSprite->lastY - marioSprite->y) >= JUMP_HEIGHT && marioSprite->direction == 1) {
                marioSprite->direction = -1;
            }
            
            // Controlla se è tornato a terra
            if (marioSprite->y + marioSprite->height >= 56 && marioSprite->direction == -1) {
                // Cancella Mario
                redrawBackground(marioSprite->x - 1, marioSprite->y - 1,
                               marioSprite->width + 2, marioSprite->height + 2);
                
                marioSprite->y = marioSprite->firstY;
                marioState = IDLE;
                marioSprite->width = MARIO_IDLE_SIZE[0];
                marioSprite->height = MARIO_IDLE_SIZE[1];
                marioSprite->sprite = MARIO_IDLE;
                
                // Controlla se deve fare secondo salto
                if (currentJumpTarget == BOTH_BLOCKS) {
                    Serial.println("Preparing for second jump...");
                    waitingForNextJump = true;
                    nextJumpDelay = millis() + 300;
                } else {
                    currentJumpTarget = NONE;
                    walkingToJump = false;
                    
                    // Torna al centro
                    marioTargetX = 23;
                    marioFacingRight = (marioTargetX > marioSprite->x);
                    
                    if (abs(marioSprite->x - marioTargetX) > WALK_SPEED) {
                        marioState = WALKING;
                    } else {
                        marioSprite->x = 23;
                        needsRedraw = true;
                    }
                }
            }
            
            // Ridisegna Mario
            spriteRenderer->drawSpriteFlipped(marioSprite->sprite, marioSprite->x, marioSprite->y,
                                             marioSprite->width, marioSprite->height, !marioFacingRight);
            
            lastMarioUpdate = millis();
        }
    }
}
