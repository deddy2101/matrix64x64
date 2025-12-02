#include "MarioClockEffect.h"

#if ENABLE_PIPE_ANIMATION
#include "pipeSprite.h"
#endif

MarioClockEffect::MarioClockEffect(DisplayManager* dm, TimeManager* tm) 
    : Effect(dm), 
      spriteRenderer(new SpriteRenderer(dm)),
      timeManager(tm),
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
      lastHour(-1),
      lastMinute(-1),
      lastMarioUpdate(0)
#if ENABLE_PIPE_ANIMATION
      ,pipe(new MarioPipe()),
      lastPipeUpdate(0),
      pipeVisibleUntil(0)
#endif
{
}

MarioClockEffect::~MarioClockEffect() {
    delete spriteRenderer;
    delete hourBlock;
    delete minuteBlock;
    delete marioSprite;
#if ENABLE_PIPE_ANIMATION
    delete pipe;
#endif
}

void MarioClockEffect::init() {
    DEBUG_PRINTLN("[MarioClockEffect] Initializing");
    
    initMario();
    initBlocks();
#if ENABLE_PIPE_ANIMATION
    initPipe();
#endif
    
    displayManager->setTextSize(1);
    displayManager->setFont(&Super_Mario_Bros__24pt7b);
    
    lastHour = timeManager->getHour();
    lastMinute = timeManager->getMinute();
    
    hourBlock->text = String(lastHour);
    char minStr[3];
    sprintf(minStr, "%02d", lastMinute);
    minuteBlock->text = String(minStr);
    
    timeManager->addOnMinuteChange([this](int h, int m, int s) {
        DEBUG_PRINTF("[MarioClockEffect] Minute changed callback: %02d:%02d\n", h, m);
        
        bool hourChanged = (h != lastHour);
        bool minuteChanged = (m != lastMinute);
        
        lastHour = h;
        lastMinute = m;
        
#if ENABLE_PIPE_ANIMATION
        // Triggera il tubo ad ogni cambio minuto!
        //triggerPipe();
#endif
        
        if (hourChanged && minuteChanged) {
            DEBUG_PRINTLN("[MarioClockEffect] Hour AND minute changed - jump to BOTH");
            marioJump(BOTH_BLOCKS);
        } else if (hourChanged) {
            DEBUG_PRINTLN("[MarioClockEffect] Hour changed - jump to HOUR block");
            marioJump(HOUR_BLOCK);
        } else if (minuteChanged) {
            DEBUG_PRINTLN("[MarioClockEffect] Minute changed - jump to MINUTE block");
            marioJump(MINUTE_BLOCK);
        }
    });
    
    DEBUG_PRINTF("[MarioClockEffect] Synced with time: %02d:%02d\n", lastHour, lastMinute);
    
    lastMarioUpdate = millis();
    needsRedraw = true;
}

void MarioClockEffect::cleanup() {
    DEBUG_PRINTLN("[MarioClockEffect] Cleanup - removing TimeManager callback");
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
    hourBlock->x = 8;
    hourBlock->y = 8;
    hourBlock->firstY = 8;
    hourBlock->width = 19;
    hourBlock->height = 19;
    hourBlock->isHit = false;
    hourBlock->direction = 1;
    hourBlock->moveAmount = 0;
    hourBlock->text = String(timeManager->getHour());
    
    minuteBlock->x = 37;
    minuteBlock->y = 8;
    minuteBlock->firstY = 8;
    minuteBlock->width = 19;
    minuteBlock->height = 19;
    minuteBlock->isHit = false;
    minuteBlock->direction = 1;
    minuteBlock->moveAmount = 0;
    char minStr[3];
    sprintf(minStr, "%02d", timeManager->getMinute());
    minuteBlock->text = String(minStr);
    
    currentJumpTarget = NONE;
    waitingForNextJump = false;
    nextJumpDelay = 0;
}

// ========== PIPE ANIMATION FUNCTIONS ==========
#if ENABLE_PIPE_ANIMATION

void MarioClockEffect::initPipe() {
    // Posiziona il tubo a destra dello schermo
    pipe->x = 40;  // Posizione X (a destra)
    pipe->width = PIPE_SPRITE_SIZE[0];   // 12
    pipe->height = PIPE_SPRITE_SIZE[1];  // 16
    
    // Il tubo parte nascosto sotto il pavimento
    int groundY = displayManager->getHeight() - 8;  // 56 per display 64px
    pipe->hiddenY = groundY + 2;  // Completamente sotto terra
    pipe->targetY = groundY - pipe->height + 4;  // Emerge di 12px sopra il terreno
    pipe->y = pipe->hiddenY;
    pipe->state = PIPE_HIDDEN;
    
    lastPipeUpdate = 0;
    pipeVisibleUntil = 0;
    
    DEBUG_PRINTF("[MarioClockEffect] Pipe initialized at x=%d, hiddenY=%d, targetY=%d\n", 
                  pipe->x, pipe->hiddenY, pipe->targetY);
}

void MarioClockEffect::triggerPipe() {
    if (pipe->state == PIPE_HIDDEN) {
        pipe->state = PIPE_RISING;
        DEBUG_PRINTLN("[MarioClockEffect] Pipe triggered - starting to rise");
    }
}

void MarioClockEffect::updatePipe() {
    if (pipe->state == PIPE_HIDDEN) {
        return;  // Niente da fare
    }
    
    unsigned long now = millis();
    
    if (now - lastPipeUpdate >= 40) {  // ~25 FPS per animazione fluida
        
        switch (pipe->state) {
            case PIPE_RISING:
                // Cancella area precedente (solo la parte che verrà scoperta)
                redrawBackground(pipe->x, pipe->y, pipe->width, PIPE_RISE_SPEED + 1);
                
                pipe->y -= PIPE_RISE_SPEED;
                
                if (pipe->y <= pipe->targetY) {
                    pipe->y = pipe->targetY;
                    pipe->state = PIPE_VISIBLE;
                    pipeVisibleUntil = now + PIPE_VISIBLE_TIME;
                    DEBUG_PRINTLN("[MarioClockEffect] Pipe fully visible");
                }
                
                drawPipe();
                break;
                
            case PIPE_VISIBLE:
                // Aspetta che scada il timer
                if (now >= pipeVisibleUntil) {
                    pipe->state = PIPE_LOWERING;
                    DEBUG_PRINTLN("[MarioClockEffect] Pipe starting to lower");
                }
                break;
                
            case PIPE_LOWERING:
                // Cancella area precedente
                redrawBackground(pipe->x, pipe->y, pipe->width, pipe->height + PIPE_RISE_SPEED);
                
                pipe->y += PIPE_RISE_SPEED;
                
                if (pipe->y >= pipe->hiddenY) {
                    pipe->y = pipe->hiddenY;
                    pipe->state = PIPE_HIDDEN;
                    // Ridisegna il terreno dove era il tubo
                    redrawBackground(pipe->x, displayManager->getHeight() - 8, pipe->width, 8);
                    DEBUG_PRINTLN("[MarioClockEffect] Pipe hidden");
                } else {
                    drawPipe();
                }
                break;
                
            default:
                break;
        }
        
        lastPipeUpdate = now;
    }
}

void MarioClockEffect::drawPipe() {
    if (pipe->state == PIPE_HIDDEN) return;
    
    int screenHeight = displayManager->getHeight();
    int screenWidth = displayManager->getWidth();
    int groundY = screenHeight - 8;
    
    // Disegna solo la parte visibile del tubo (sopra il terreno)
    for (int dy = 0; dy < pipe->height; dy++) {
        int screenY = pipe->y + dy;
        
        // Salta se sotto il terreno
        if (screenY >= groundY) continue;
        // Salta se sopra lo schermo
        if (screenY < 0) continue;
        
        for (int dx = 0; dx < pipe->width; dx++) {
            int screenX = pipe->x + dx;
            
            if (screenX < 0 || screenX >= screenWidth) continue;
            
            int idx = dy * pipe->width + dx;
            uint16_t color = pgm_read_word(&PIPE_SPRITE[idx]);
            
            uint8_t r, g, b;
            
            // Se pixel trasparente, ridisegna lo sfondo
            if (color == _MASK) {
                // Default: colore cielo
                r = 0; g = 145; b = 206;
                
                // Controlla se è nella collina
                if (screenX < 20 && screenY >= 34 && screenY < 56) {
                    int hillX = screenX;
                    int hillY = screenY - 34;
                    if (hillY < 22 && hillX < 20) {
                        uint16_t hillColor = pgm_read_word(&HILL[hillY * 20 + hillX]);
                        if (hillColor != _MASK) {
                            r = (hillColor >> 11) << 3;
                            g = ((hillColor >> 5) & 0x3F) << 2;
                            b = (hillColor & 0x1F) << 3;
                        }
                    }
                }
                // Controlla cespuglio
                else if (screenX >= 43 && screenX < 64 && screenY >= 47 && screenY < 56) {
                    int bushX = screenX - 43;
                    int bushY = screenY - 47;
                    if (bushY < 9 && bushX < 21) {
                        uint16_t bushColor = pgm_read_word(&BUSH[bushY * 21 + bushX]);
                        if (bushColor != _MASK) {
                            r = (bushColor >> 11) << 3;
                            g = ((bushColor >> 5) & 0x3F) << 2;
                            b = (bushColor & 0x1F) << 3;
                        }
                    }
                }
            } else {
                // Pixel del tubo
                r = (color >> 11) << 3;
                g = ((color >> 5) & 0x3F) << 2;
                b = (color & 0x1F) << 3;
            }
            
            displayManager->drawPixel(screenX, screenY, r, g, b);
        }
    }
}

#endif  // ENABLE_PIPE_ANIMATION
// ============================================

void MarioClockEffect::reset() {
    Effect::reset();
    lastHour = -1;
    lastMinute = -1;
}

void MarioClockEffect::update() {
    updateMario();
    
#if ENABLE_PIPE_ANIMATION
    updatePipe();
#endif
    
    static unsigned long lastHourUpdate = 0;
    static unsigned long lastMinuteUpdate = 0;
    
    updateBlock(*hourBlock, lastHourUpdate);
    updateBlock(*minuteBlock, lastMinuteUpdate);
}

void MarioClockEffect::draw() {
    if (needsRedraw && marioState == IDLE && !waitingForNextJump && 
        !hourBlock->isHit && !minuteBlock->isHit) {
        drawScene();
        needsRedraw = false;
    }
}

void MarioClockEffect::drawScene() {
    displayManager->fillScreen(0, 145, 206);
    
    displayManager->setTextSize(1);
    displayManager->setFont(&Super_Mario_Bros__24pt7b);
    
    spriteRenderer->drawSprite(HILL, 0, 34, 20, 22);
    spriteRenderer->drawSprite(BUSH, 43, 47, 21, 9);
    spriteRenderer->drawSprite(CLOUD1, 0, 21, 13, 12);
    spriteRenderer->drawSprite(CLOUD2, 51, 7, 13, 12);
    
    drawGround();
    
    drawBlock(*hourBlock);
    drawBlock(*minuteBlock);
    
#if ENABLE_PIPE_ANIMATION
    // Disegna tubo se visibile
    if (pipe->state != PIPE_HIDDEN) {
        drawPipe();
    }
#endif
    
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
    
    displayManager->setTextSize(1);
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
            
            block.y += BLOCK_MOVE_PACE * (block.direction == 1 ? -1 : 1);
            block.moveAmount += BLOCK_MOVE_PACE;
            
            if (block.moveAmount >= BLOCK_MAX_MOVE && block.direction == 1) {
                block.direction = -1;
            }
            
            if (block.y >= block.firstY && block.direction == -1) {
                block.y = block.firstY;
                block.isHit = false;
                block.direction = 1;
                block.moveAmount = 0;
            }
            
            drawBlock(block);
            lastUpdate = millis();
        }
    }
}

void MarioClockEffect::redrawBackground(int x, int y, int width, int height) {
    int screenWidth = displayManager->getWidth();
    int screenHeight = displayManager->getHeight();
    
    for (int dy = 0; dy < height; dy++) {
        for (int dx = 0; dx < width; dx++) {
            int px = x + dx;
            int py = y + dy;
            
            if (px < 0 || px >= screenWidth || py < 0 || py >= screenHeight) continue;
            
            uint8_t r = 0, g = 145, b = 206;
            
            if (py >= screenHeight - 8) {
                int groundY = py - (screenHeight - 8);
                int idx = (groundY * 8 + (px % 8));
                if (idx < 64) {
                    uint16_t color = GROUND[idx];
                    r = (color >> 11) << 3;
                    g = ((color >> 5) & 0x3F) << 2;
                    b = (color & 0x1F) << 3;
                }
            }
            else if (px < 20 && py >= 34 && py < 56) {
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
            else if (px < 13 && py >= 21 && py < 33) {
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

void MarioClockEffect::startJump() {
    marioState = JUMPING;
    marioSprite->direction = 1;
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
        
        switch(target) {
            case HOUR_BLOCK:
                marioTargetX = 8;
                DEBUG_PRINTLN("[MarioClockEffect] Walking to HOUR block");
                break;
            case MINUTE_BLOCK:
                marioTargetX = 40;
                DEBUG_PRINTLN("[MarioClockEffect] Walking to MINUTE block");
                break;
            case BOTH_BLOCKS:
                marioTargetX = 8;
                DEBUG_PRINTLN("[MarioClockEffect] Walking to HOUR block first");
                break;
            default:
                marioTargetX = 23;
                break;
        }
        
        marioFacingRight = (marioTargetX > marioSprite->x);
        
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
}

void MarioClockEffect::updateMarioWalk() {
    static unsigned long lastWalkUpdate = 0;
    
    if (marioState == WALKING) {
        if (millis() - lastWalkUpdate >= 50) {
            int oldX = marioSprite->x;
            
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
            
            if (oldX != marioSprite->x) {
                if (oldX < marioSprite->x) {
                    redrawBackground(oldX, marioSprite->y, WALK_SPEED + 1, marioSprite->height);
                } else {
                    redrawBackground(marioSprite->x + marioSprite->width - 1, marioSprite->y, 
                                   WALK_SPEED + 1, marioSprite->height);
                }
                
                spriteRenderer->drawSpriteFlipped(marioSprite->sprite, marioSprite->x, marioSprite->y,
                                                 marioSprite->width, marioSprite->height, !marioFacingRight);
            }
            
            lastWalkUpdate = millis();
        }
    }
}

void MarioClockEffect::updateMario() {
    if (waitingForNextJump && millis() >= nextJumpDelay) {
        waitingForNextJump = false;
        
        char minStr[3];
        sprintf(minStr, "%02d", timeManager->getMinute());
        minuteBlock->text = String(minStr);
        
        currentJumpTarget = MINUTE_BLOCK;
        marioTargetX = 40;
        marioFacingRight = (marioTargetX > marioSprite->x);
        walkingToJump = true;
        
        DEBUG_PRINTLN("[MarioClockEffect] Walking to MINUTE block");
        
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
            redrawBackground(marioSprite->x - 1, marioSprite->y - 1,
                           marioSprite->width + 2, marioSprite->height + 2);
            
            marioSprite->y += MARIO_PACE * (marioSprite->direction == 1 ? -1 : 1);
            
            if (marioSprite->direction == 1) {
                if (checkCollision(*marioSprite, *hourBlock) && !marioSprite->collisionDetected) {
                    DEBUG_PRINTLN("[MarioClockEffect] Collision with hour block!");
                    hourBlock->isHit = true;
                    marioSprite->direction = -1;
                    marioSprite->collisionDetected = true;
                    
                    hourBlock->text = String(timeManager->getHour());
                    needsRedraw = true;
                } else if (checkCollision(*marioSprite, *minuteBlock) && !marioSprite->collisionDetected) {
                    DEBUG_PRINTLN("[MarioClockEffect] Collision with minute block!");
                    minuteBlock->isHit = true;
                    marioSprite->direction = -1;
                    marioSprite->collisionDetected = true;
                    
                    char minStr[3];
                    sprintf(minStr, "%02d", timeManager->getMinute());
                    minuteBlock->text = String(minStr);
                    needsRedraw = true;
                }
            }
            
            if ((marioSprite->lastY - marioSprite->y) >= JUMP_HEIGHT && marioSprite->direction == 1) {
                marioSprite->direction = -1;
            }
            
            if (marioSprite->y + marioSprite->height >= 56 && marioSprite->direction == -1) {
                redrawBackground(marioSprite->x - 1, marioSprite->y - 1,
                               marioSprite->width + 2, marioSprite->height + 2);
                
                marioSprite->y = marioSprite->firstY;
                marioState = IDLE;
                marioSprite->width = MARIO_IDLE_SIZE[0];
                marioSprite->height = MARIO_IDLE_SIZE[1];
                marioSprite->sprite = MARIO_IDLE;
                
                if (currentJumpTarget == BOTH_BLOCKS) {
                    DEBUG_PRINTLN("[MarioClockEffect] Preparing for second jump...");
                    waitingForNextJump = true;
                    nextJumpDelay = millis() + 300;
                } else {
                    currentJumpTarget = NONE;
                    walkingToJump = false;
                    
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
            
            spriteRenderer->drawSpriteFlipped(marioSprite->sprite, marioSprite->x, marioSprite->y,
                                             marioSprite->width, marioSprite->height, !marioFacingRight);
            
            lastMarioUpdate = millis();
        }
    }
}