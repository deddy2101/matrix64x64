#include "SpaceInvadersClockEffect.h"
#include <math.h>

// Font 5x7 per cifre
const uint8_t SpaceInvadersClockEffect::FONT_5X7[][7] = {
    // 0
    {0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110},
    // 1
    {0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110},
    // 2
    {0b01110, 0b10001, 0b00001, 0b00110, 0b01000, 0b10000, 0b11111},
    // 3
    {0b01110, 0b10001, 0b00001, 0b00110, 0b00001, 0b10001, 0b01110},
    // 4
    {0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010},
    // 5
    {0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110},
    // 6
    {0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110},
    // 7
    {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000},
    // 8
    {0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110},
    // 9
    {0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b01100},
};

SpaceInvadersClockEffect::SpaceInvadersClockEffect(DisplayManager* dm, TimeManager* tm)
    : Effect(dm),
      timeManager(tm),
      shipX(27),
      shipY(54),
      shipState(SHIP_PLAYING),
      shipTargetX(27),
      shipMovingRight(true),
      shipDirection(1),
      lastBulletUpdate(0),
      aliensAlive(0),
      alienDirection(1),
      alienBaseY(18),
      lastAlienUpdate(0),
      lastAlienShootTime(0),
      lastAlienExplosionUpdate(0),
      explodingDigit(-1),
      explosionFrame(0),
      lastExplosionUpdate(0),
      lastHour(-1),
      lastMinute(-1),
      currentTargetDigit(-1),
      lastShipUpdate(0),
      lastUpdate(0),
      needsRedraw(true),
      minuteCallbackId(-1) {

    bullet.active = false;
    bullet.x = 0;
    bullet.y = 0;
    bullet.targetDigit = -1;

    for (int i = 0; i < 4; i++) {
        displayedDigits[i] = 0;
        digitNeedsUpdate[i] = false;
    }

    for (int i = 0; i < MAX_ALIENS; i++) {
        aliens[i].alive = false;
    }

    for (int i = 0; i < MAX_ALIEN_EXPLOSIONS; i++) {
        alienExplosions[i].active = false;
    }

    // Posizioni delle cifre (HH:MM)
    digitX[0] = 4;      // H1
    digitX[1] = 16;     // H2
    // Colon at 28-30
    digitX[2] = 34;     // M1
    digitX[3] = 46;     // M2
    digitY = 4;
}

SpaceInvadersClockEffect::~SpaceInvadersClockEffect() {
}

void SpaceInvadersClockEffect::init() {
    DEBUG_PRINTLN("[SpaceInvadersClockEffect] Initializing");

    lastHour = timeManager->getHour();
    lastMinute = timeManager->getMinute();

    // Initialize displayed digits
    displayedDigits[0] = lastHour / 10;
    displayedDigits[1] = lastHour % 10;
    displayedDigits[2] = lastMinute / 10;
    displayedDigits[3] = lastMinute % 10;

    // Center ship
    shipX = 27;
    shipState = SHIP_PLAYING;
    shipDirection = 1;

    // Initialize aliens
    initAliens();

    minuteCallbackId = timeManager->addOnMinuteChange([this](int h, int m, int s) {
        onTimeChange(h, m);
    });

    needsRedraw = true;
    lastUpdate = millis();
    lastAlienShootTime = millis();
}

void SpaceInvadersClockEffect::cleanup() {
    if (minuteCallbackId >= 0) {
        timeManager->removeCallback(minuteCallbackId);
    }
    DEBUG_PRINTLN("[SpaceInvadersClockEffect] Cleanup");
}

void SpaceInvadersClockEffect::reset() {
    Effect::reset();
    lastHour = -1;
    lastMinute = -1;
    shipState = SHIP_PLAYING;
    bullet.active = false;
    explodingDigit = -1;
    initAliens();
    needsRedraw = true;
}

void SpaceInvadersClockEffect::initAliens() {
    // Create 2 rows of 6 aliens each
    aliensAlive = 0;
    int startX = 4;
    int startY = alienBaseY;

    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 6; col++) {
            int idx = row * 6 + col;
            aliens[idx].x = startX + col * 10;
            aliens[idx].y = startY + row * 10;  // More vertical spacing
            aliens[idx].alive = true;
            aliens[idx].type = row;  // Different type per row
            aliensAlive++;
        }
    }
    alienDirection = 1;
}

void SpaceInvadersClockEffect::respawnAliens() {
    // Respawn all aliens when they're all dead
    initAliens();
    DEBUG_PRINTLN("[SpaceInvadersClockEffect] Aliens respawned!");
}

void SpaceInvadersClockEffect::onTimeChange(int newHour, int newMinute) {
    DEBUG_PRINTF("[SpaceInvadersClockEffect] Time changed to %02d:%02d\n", newHour, newMinute);

    int newDigits[4] = {
        newHour / 10,
        newHour % 10,
        newMinute / 10,
        newMinute % 10
    };

    // Mark which digits need updating
    for (int i = 0; i < 4; i++) {
        if (displayedDigits[i] != newDigits[i]) {
            digitNeedsUpdate[i] = true;
        }
    }

    lastHour = newHour;
    lastMinute = newMinute;

    // Interrupt game to shoot digits
    if (shipState == SHIP_PLAYING || shipState == SHIP_IDLE) {
        int nextDigit = findNextDigitToUpdate();
        if (nextDigit >= 0) {
            // Wait for current bullet to finish
            if (!bullet.active) {
                startShooting(nextDigit);
            }
        }
    }
}

int SpaceInvadersClockEffect::findNextDigitToUpdate() {
    for (int i = 0; i < 4; i++) {
        if (digitNeedsUpdate[i]) {
            return i;
        }
    }
    return -1;
}

int SpaceInvadersClockEffect::getDigitCenterX(int digitIndex) {
    return digitX[digitIndex] + 5;  // Center of 10px wide digit
}

void SpaceInvadersClockEffect::startShooting(int digitIndex) {
    currentTargetDigit = digitIndex;
    shipTargetX = getDigitCenterX(digitIndex) - SHIP_WIDTH / 2;

    // Clamp to screen
    if (shipTargetX < 0) shipTargetX = 0;
    if (shipTargetX > 64 - SHIP_WIDTH) shipTargetX = 64 - SHIP_WIDTH;

    shipMovingRight = (shipTargetX > shipX);
    shipState = SHIP_MOVING;

    DEBUG_PRINTF("[SpaceInvadersClockEffect] Moving to digit %d\n", digitIndex);
}

void SpaceInvadersClockEffect::update() {
    updateShip();
    updateBullet();
    updateExplosion();
    updateAliens();
    updateAlienExplosions();
    updateGame();
}

void SpaceInvadersClockEffect::updateGame() {
    // Game logic when playing
    if (shipState != SHIP_PLAYING) return;

    unsigned long now = millis();

    // Check if we need to shoot a digit instead
    int nextDigit = findNextDigitToUpdate();
    if (nextDigit >= 0 && !bullet.active) {
        startShooting(nextDigit);
        return;
    }

    // Respawn aliens if all dead
    if (aliensAlive == 0) {
        respawnAliens();
    }

    // Auto-fire at aliens periodically
    if (!bullet.active && (now - lastAlienShootTime > 800)) {
        fireAtAlien();
        lastAlienShootTime = now;
    }

    // Move ship towards nearest alien
    int nearestAlien = findNearestAlien();
    if (nearestAlien >= 0 && !bullet.active) {
        int targetX = aliens[nearestAlien].x + ALIEN_WIDTH / 2 - SHIP_WIDTH / 2;
        if (targetX < 0) targetX = 0;
        if (targetX > 64 - SHIP_WIDTH) targetX = 64 - SHIP_WIDTH;

        if (abs(shipX - targetX) > 2) {
            shipDirection = (targetX > shipX) ? 1 : -1;
        } else {
            shipDirection = 0;
        }
    }
}

int SpaceInvadersClockEffect::findNearestAlien() {
    int nearest = -1;
    int minDist = 9999;

    for (int i = 0; i < MAX_ALIENS; i++) {
        if (aliens[i].alive) {
            int dist = abs((aliens[i].x + ALIEN_WIDTH / 2) - (shipX + SHIP_WIDTH / 2));
            if (dist < minDist) {
                minDist = dist;
                nearest = i;
            }
        }
    }
    return nearest;
}

void SpaceInvadersClockEffect::fireAtAlien() {
    if (bullet.active) return;

    bullet.active = true;
    bullet.x = shipX + SHIP_WIDTH / 2;
    bullet.y = shipY - 2;
    bullet.targetDigit = -1;  // Shooting at alien, not digit
}

void SpaceInvadersClockEffect::addAlienExplosion(int x, int y) {
    for (int i = 0; i < MAX_ALIEN_EXPLOSIONS; i++) {
        if (!alienExplosions[i].active) {
            alienExplosions[i].x = x;
            alienExplosions[i].y = y;
            alienExplosions[i].frame = 0;
            alienExplosions[i].active = true;
            return;
        }
    }
}

void SpaceInvadersClockEffect::updateShip() {
    unsigned long now = millis();

    if (now - lastShipUpdate < 30) return;
    lastShipUpdate = now;

    switch (shipState) {
        case SHIP_PLAYING:
            // Move based on shipDirection
            if (shipDirection != 0) {
                shipX += shipDirection * 2;
                if (shipX < 0) shipX = 0;
                if (shipX > 64 - SHIP_WIDTH) shipX = 64 - SHIP_WIDTH;
                needsRedraw = true;
            }
            break;

        case SHIP_IDLE:
            // Check if there are digits to update
            {
                int nextDigit = findNextDigitToUpdate();
                if (nextDigit >= 0 && !bullet.active && explodingDigit < 0) {
                    startShooting(nextDigit);
                } else if (!bullet.active && explodingDigit < 0) {
                    // Return to playing
                    shipState = SHIP_PLAYING;
                }
            }
            break;

        case SHIP_MOVING:
            // Move towards target
            if (abs(shipX - shipTargetX) <= 2) {
                shipX = shipTargetX;
                shipState = SHIP_SHOOTING;
            } else {
                shipX += shipMovingRight ? 2 : -2;
                needsRedraw = true;
            }
            break;

        case SHIP_SHOOTING:
            // Fire bullet at digit
            if (!bullet.active) {
                bullet.active = true;
                bullet.x = shipX + SHIP_WIDTH / 2;
                bullet.y = shipY - 2;
                bullet.targetDigit = currentTargetDigit;
                shipState = SHIP_WAITING;
            }
            break;

        case SHIP_WAITING:
            // Wait for bullet/explosion to complete
            if (!bullet.active && explodingDigit < 0) {
                int nextDigit = findNextDigitToUpdate();
                if (nextDigit >= 0) {
                    startShooting(nextDigit);
                } else {
                    // Return to playing mode
                    shipState = SHIP_PLAYING;
                }
            }
            break;
    }
}

void SpaceInvadersClockEffect::updateBullet() {
    if (!bullet.active) return;

    unsigned long now = millis();
    if (now - lastBulletUpdate < 20) return;
    lastBulletUpdate = now;

    // Move bullet up
    bullet.y -= 4;
    needsRedraw = true;

    // Check collision with digit (if targeting digit)
    if (bullet.targetDigit >= 0 && bullet.y <= digitY + 14) {
        explodingDigit = bullet.targetDigit;
        explosionFrame = 0;
        lastExplosionUpdate = millis();
        bullet.active = false;
        return;
    }

    // Check collision with aliens
    if (bullet.targetDigit < 0) {
        for (int i = 0; i < MAX_ALIENS; i++) {
            if (aliens[i].alive) {
                if (bullet.x >= aliens[i].x && bullet.x < aliens[i].x + ALIEN_WIDTH &&
                    bullet.y >= aliens[i].y && bullet.y < aliens[i].y + ALIEN_HEIGHT) {
                    // Hit!
                    aliens[i].alive = false;
                    aliensAlive--;
                    addAlienExplosion(aliens[i].x + ALIEN_WIDTH / 2, aliens[i].y + ALIEN_HEIGHT / 2);
                    bullet.active = false;
                    return;
                }
            }
        }
    }

    // Off screen
    if (bullet.y < 0) {
        bullet.active = false;
    }
}

void SpaceInvadersClockEffect::updateExplosion() {
    if (explodingDigit < 0) return;

    unsigned long now = millis();
    if (now - lastExplosionUpdate < 80) return;
    lastExplosionUpdate = now;

    explosionFrame++;
    needsRedraw = true;

    if (explosionFrame >= 6) {
        // Update the digit
        int newDigits[4] = {
            lastHour / 10,
            lastHour % 10,
            lastMinute / 10,
            lastMinute % 10
        };

        displayedDigits[explodingDigit] = newDigits[explodingDigit];
        digitNeedsUpdate[explodingDigit] = false;

        explodingDigit = -1;
        explosionFrame = 0;
    }
}

void SpaceInvadersClockEffect::updateAliens() {
    if (aliensAlive == 0) return;

    unsigned long now = millis();
    if (now - lastAlienUpdate < 800) return;  // Move every 800ms (slower)
    lastAlienUpdate = now;

    // Find edges
    int leftMost = 64, rightMost = 0;
    for (int i = 0; i < MAX_ALIENS; i++) {
        if (aliens[i].alive) {
            if (aliens[i].x < leftMost) leftMost = aliens[i].x;
            if (aliens[i].x + ALIEN_WIDTH > rightMost) rightMost = aliens[i].x + ALIEN_WIDTH;
        }
    }

    // Check if need to change direction
    bool changeDir = false;
    if (alienDirection > 0 && rightMost >= 60) {
        changeDir = true;
    } else if (alienDirection < 0 && leftMost <= 4) {
        changeDir = true;
    }

    // Move aliens
    for (int i = 0; i < MAX_ALIENS; i++) {
        if (aliens[i].alive) {
            if (changeDir) {
                aliens[i].y += 2;  // Move down
            } else {
                aliens[i].x += alienDirection * 2;
            }
        }
    }

    if (changeDir) {
        alienDirection = -alienDirection;
    }

    // Check if any alien reached or passed the ship (game over condition)
    for (int i = 0; i < MAX_ALIENS; i++) {
        if (aliens[i].alive) {
            // Check if alien touches or passes ship Y position
            if (aliens[i].y + ALIEN_HEIGHT >= shipY) {
                DEBUG_PRINTLN("[SpaceInvadersClockEffect] Alien reached ship! Restarting game...");
                respawnAliens();
                return;
            }
        }
    }

    needsRedraw = true;
}

void SpaceInvadersClockEffect::updateAlienExplosions() {
    unsigned long now = millis();
    if (now - lastAlienExplosionUpdate < 60) return;
    lastAlienExplosionUpdate = now;

    for (int i = 0; i < MAX_ALIEN_EXPLOSIONS; i++) {
        if (alienExplosions[i].active) {
            alienExplosions[i].frame++;
            if (alienExplosions[i].frame >= 5) {
                alienExplosions[i].active = false;
            }
            needsRedraw = true;
        }
    }
}

void SpaceInvadersClockEffect::draw() {
    // Always redraw during animations
    drawScene();
}

void SpaceInvadersClockEffect::drawScene() {
    // Black background
    displayManager->fillScreen(0, 0, 0);

    // Draw digits (time) at top
    drawDigits();
    drawColon();

    // Draw explosion on digit if active
    if (explodingDigit >= 0) {
        drawExplosion(explodingDigit);
    }

    // Draw aliens
    drawAliens();

    // Draw alien explosions
    drawAlienExplosions();

    // Draw bullet
    drawBullet();

    // Draw ship
    drawShip();
}

void SpaceInvadersClockEffect::drawShip() {
    // Classic space invaders player ship (green)
    // Simple triangular shape
    for (int row = 0; row < 8; row++) {
        uint16_t rowData = 0;
        switch(row) {
            case 0: rowData = 0b00000100000; break;
            case 1: rowData = 0b00001110000; break;
            case 2: rowData = 0b00001110000; break;
            case 3: rowData = 0b01111111110; break;
            case 4: rowData = 0b11111111111; break;
            case 5: rowData = 0b11111111111; break;
            case 6: rowData = 0b11111111111; break;
            case 7: rowData = 0b11111111111; break;
        }

        for (int col = 0; col < 11; col++) {
            if (rowData & (1 << (10 - col))) {
                int px = shipX + col;
                int py = shipY + row;
                if (px >= 0 && px < 64 && py >= 0 && py < 64) {
                    displayManager->drawPixel(px, py, SHIP_COLOR_R, SHIP_COLOR_G, SHIP_COLOR_B);
                }
            }
        }
    }
}

void SpaceInvadersClockEffect::drawBullet() {
    if (!bullet.active) return;

    // Draw bullet as 2x4 rectangle
    for (int dy = 0; dy < 4; dy++) {
        for (int dx = 0; dx < 2; dx++) {
            int px = bullet.x + dx - 1;
            int py = bullet.y + dy;
            if (px >= 0 && px < 64 && py >= 0 && py < 64) {
                displayManager->drawPixel(px, py, BULLET_COLOR_R, BULLET_COLOR_G, BULLET_COLOR_B);
            }
        }
    }
}

void SpaceInvadersClockEffect::drawAliens() {
    bool animFrame = (millis() / 300) % 2;  // Alternate every 300ms

    for (int i = 0; i < MAX_ALIENS; i++) {
        if (aliens[i].alive) {
            drawAlien(aliens[i].x, aliens[i].y, aliens[i].type, animFrame);
        }
    }
}

void SpaceInvadersClockEffect::drawAlien(int x, int y, int type, bool frame) {
    // Simple alien sprites (8x6)
    // Type 0: squid-like (top row)
    // Type 1: crab-like (bottom row)

    uint8_t sprite[6];

    if (type == 0) {
        // Squid alien
        if (frame) {
            sprite[0] = 0b00111100;
            sprite[1] = 0b01111110;
            sprite[2] = 0b11011011;
            sprite[3] = 0b11111111;
            sprite[4] = 0b01100110;
            sprite[5] = 0b11000011;
        } else {
            sprite[0] = 0b00111100;
            sprite[1] = 0b01111110;
            sprite[2] = 0b11011011;
            sprite[3] = 0b11111111;
            sprite[4] = 0b00100100;
            sprite[5] = 0b01000010;
        }
    } else {
        // Crab alien
        if (frame) {
            sprite[0] = 0b00100100;
            sprite[1] = 0b00111100;
            sprite[2] = 0b01111110;
            sprite[3] = 0b11011011;
            sprite[4] = 0b11111111;
            sprite[5] = 0b01000010;
        } else {
            sprite[0] = 0b00100100;
            sprite[1] = 0b10111101;
            sprite[2] = 0b11111111;
            sprite[3] = 0b11011011;
            sprite[4] = 0b01111110;
            sprite[5] = 0b10000001;
        }
    }

    // Color based on type
    uint8_t r, g, b;
    if (type == 0) {
        r = 255; g = 0; b = 255;  // Magenta
    } else {
        r = 0; g = 255; b = 255;  // Cyan
    }

    for (int row = 0; row < 6; row++) {
        for (int col = 0; col < 8; col++) {
            if (sprite[row] & (1 << (7 - col))) {
                int px = x + col;
                int py = y + row;
                if (px >= 0 && px < 64 && py >= 0 && py < 64) {
                    displayManager->drawPixel(px, py, r, g, b);
                }
            }
        }
    }
}

void SpaceInvadersClockEffect::drawAlienExplosions() {
    for (int i = 0; i < MAX_ALIEN_EXPLOSIONS; i++) {
        if (alienExplosions[i].active) {
            int cx = alienExplosions[i].x;
            int cy = alienExplosions[i].y;
            int frame = alienExplosions[i].frame;

            // Expanding explosion
            int radius = frame * 2 + 1;
            uint8_t brightness = 255 - frame * 50;

            // Draw particles
            for (int angle = 0; angle < 8; angle++) {
                float a = angle * 0.785f;
                int px = cx + (int)(cos(a) * radius);
                int py = cy + (int)(sin(a) * radius);

                if (px >= 0 && px < 64 && py >= 0 && py < 64) {
                    displayManager->drawPixel(px, py, brightness, brightness / 2, 0);
                }
            }
        }
    }
}

void SpaceInvadersClockEffect::drawDigits() {
    for (int i = 0; i < 4; i++) {
        if (i == explodingDigit && explosionFrame < 4) continue;

        drawDigit(displayedDigits[i], digitX[i], digitY,
                 DIGIT_COLOR_R, DIGIT_COLOR_G, DIGIT_COLOR_B);
    }
}

void SpaceInvadersClockEffect::drawDigit(int digit, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (digit < 0 || digit > 9) return;

    const uint8_t* glyph = FONT_5X7[digit];
    int scale = 2;

    for (int row = 0; row < 7; row++) {
        uint8_t rowData = glyph[row];
        for (int col = 0; col < 5; col++) {
            if (rowData & (0b10000 >> col)) {
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        int px = x + col * scale + sx;
                        int py = y + row * scale + sy;
                        if (px >= 0 && px < 64 && py >= 0 && py < 64) {
                            displayManager->drawPixel(px, py, r, g, b);
                        }
                    }
                }
            }
        }
    }
}

void SpaceInvadersClockEffect::drawColon() {
    int colonX = 28;
    int colonY = digitY;

    bool visible = (millis() / 500) % 2 == 0;

    if (visible) {
        // Upper dot
        for (int dy = 0; dy < 3; dy++) {
            for (int dx = 0; dx < 3; dx++) {
                displayManager->drawPixel(colonX + dx, colonY + 4 + dy,
                                         DIGIT_COLOR_R, DIGIT_COLOR_G, DIGIT_COLOR_B);
            }
        }
        // Lower dot
        for (int dy = 0; dy < 3; dy++) {
            for (int dx = 0; dx < 3; dx++) {
                displayManager->drawPixel(colonX + dx, colonY + 10 + dy,
                                         DIGIT_COLOR_R, DIGIT_COLOR_G, DIGIT_COLOR_B);
            }
        }
    }
}

void SpaceInvadersClockEffect::drawExplosion(int digitIndex) {
    int cx = digitX[digitIndex] + 5;
    int cy = digitY + 7;

    int radius = explosionFrame * 3 + 2;

    for (int angle = 0; angle < 8; angle++) {
        float a = angle * 0.785f;
        int px = cx + (int)(cos(a) * radius);
        int py = cy + (int)(sin(a) * radius);

        uint8_t r = EXPLOSION_COLOR_R;
        uint8_t g = EXPLOSION_COLOR_G * (6 - explosionFrame) / 6;
        uint8_t b = 0;

        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int ppx = px + dx;
                int ppy = py + dy;
                if (ppx >= 0 && ppx < 64 && ppy >= 0 && ppy < 64) {
                    displayManager->drawPixel(ppx, ppy, r, g, b);
                }
            }
        }
    }
}
