#include "SnakeEffect.h"

SnakeEffect::SnakeEffect(DisplayManager* dm)
    : Effect(dm),
      direction(SnakeDirection::RIGHT),
      nextDirection(SnakeDirection::RIGHT),
      gameState(SnakeGameState::WAITING),
      score(0),
      highScore(0),
      level(1),
      foodEaten(0),
      lastMoveTime(0),
      moveInterval(BASE_MOVE_INTERVAL),
      foodSpawnTime(0),
      bonusFoodDuration(8000),
      animationTimer(0),
      animationFrame(0),
      headPulse(0),
      trailFade(0),
      showGrid(true),
      playerJoined(false),
      foodType(FoodType::NORMAL) {
}

void SnakeEffect::init() {
    DEBUG_PRINTLN("[Snake] Initializing");

    // Se nessun giocatore, mostra schermata attesa
    if (!playerJoined) {
        gameState = SnakeGameState::WAITING;
    }

    displayManager->fillScreen(0, 0, 0);
}

void SnakeEffect::resetGame() {
    snake.clear();

    // Posizione iniziale al centro
    int startX = GRID_WIDTH / 2;
    int startY = GRID_HEIGHT / 2;

    for (int i = 0; i < INITIAL_LENGTH; i++) {
        snake.push_back({startX - i, startY});
    }

    direction = SnakeDirection::RIGHT;
    nextDirection = SnakeDirection::RIGHT;
    score = 0;
    level = 1;
    foodEaten = 0;
    moveInterval = BASE_MOVE_INTERVAL;
    lastMoveTime = millis();
    animationFrame = 0;

    spawnFood();

    DEBUG_PRINTLN("[Snake] Game reset");
}

void SnakeEffect::spawnFood() {
    int attempts = 0;
    do {
        food.x = random(1, GRID_WIDTH - 1);  // Evita i bordi
        food.y = random(1, GRID_HEIGHT - 1);
        attempts++;
    } while (isPositionOnSnake(food.x, food.y) && attempts < 100);

    // Determina tipo di cibo
    int chance = random(100);
    if (chance < 5) {
        foodType = FoodType::SUPER;       // 5% super
    } else if (chance < 20) {
        foodType = FoodType::BONUS;       // 15% bonus
    } else {
        foodType = FoodType::NORMAL;      // 80% normale
    }

    foodSpawnTime = millis();

    DEBUG_PRINTF("[Snake] Food spawned at (%d, %d) type: %d\n", food.x, food.y, (int)foodType);
}

bool SnakeEffect::isPositionOnSnake(int x, int y) {
    for (const auto& seg : snake) {
        if (seg.x == x && seg.y == y) return true;
    }
    return false;
}

void SnakeEffect::update() {
    unsigned long now = millis();

    // Aggiorna animazioni
    if (now - animationTimer > 50) {
        animationTimer = now;
        animationFrame++;
        headPulse = sin(animationFrame * 0.2) * 0.3 + 0.7;
    }

    switch (gameState) {
        case SnakeGameState::WAITING:
            // Animazione attesa
            break;

        case SnakeGameState::PLAYING:
            // Aggiorna direzione
            direction = nextDirection;

            // Movimento del serpente
            if (now - lastMoveTime >= moveInterval) {
                lastMoveTime = now;
                moveSnake();

                // Controlla collisione con cibo
                if (snake[0].x == food.x && snake[0].y == food.y) {
                    growSnake();

                    // Assegna punti in base al tipo
                    switch (foodType) {
                        case FoodType::NORMAL:
                            score += 1;
                            break;
                        case FoodType::BONUS:
                            score += 3;
                            break;
                        case FoodType::SUPER:
                            score += 5;
                            break;
                    }

                    foodEaten++;
                    updateLevel();
                    spawnFood();

                    DEBUG_PRINTF("[Snake] Ate food! Score: %d, Length: %d\n", score, snake.size());
                }

                // Cibo bonus scade
                if (foodType != FoodType::NORMAL && (now - foodSpawnTime) > bonusFoodDuration) {
                    spawnFood();
                }

                // Controlla collisioni
                if (checkCollision()) {
                    gameState = SnakeGameState::GAME_OVER;
                    if (score > highScore) {
                        highScore = score;
                    }
                    DEBUG_PRINTF("[Snake] Game Over! Score: %d, High: %d\n", score, highScore);
                }
            }
            break;

        case SnakeGameState::PAUSED:
            // Nessun aggiornamento
            break;

        case SnakeGameState::GAME_OVER:
            // Animazione game over
            break;
    }
}

void SnakeEffect::moveSnake() {
    // Calcola nuova posizione testa
    SnakeSegment newHead = snake[0];

    switch (direction) {
        case SnakeDirection::UP:
            newHead.y--;
            break;
        case SnakeDirection::DOWN:
            newHead.y++;
            break;
        case SnakeDirection::LEFT:
            newHead.x--;
            break;
        case SnakeDirection::RIGHT:
            newHead.x++;
            break;
    }

    // Inserisci nuova testa
    snake.insert(snake.begin(), newHead);

    // Rimuovi coda
    snake.pop_back();
}

bool SnakeEffect::checkCollision() {
    const SnakeSegment& head = snake[0];

    // Collisione con bordi
    if (head.x < 0 || head.x >= GRID_WIDTH ||
        head.y < 0 || head.y >= GRID_HEIGHT) {
        return true;
    }

    // Collisione con se stesso
    for (size_t i = 1; i < snake.size(); i++) {
        if (head.x == snake[i].x && head.y == snake[i].y) {
            return true;
        }
    }

    return false;
}

void SnakeEffect::growSnake() {
    // Aggiungi segmento alla fine
    snake.push_back(snake.back());
}

void SnakeEffect::updateLevel() {
    // Aumenta livello ogni 5 cibi
    int newLevel = (foodEaten / 5) + 1;
    if (newLevel > level) {
        level = newLevel;
        // Velocizza il gioco
        moveInterval = max((int)MIN_MOVE_INTERVAL, BASE_MOVE_INTERVAL - (level - 1) * 15);
        DEBUG_PRINTF("[Snake] Level up! Level: %d, Speed: %d ms\n", level, moveInterval);
    }
}

void SnakeEffect::draw() {
    switch (gameState) {
        case SnakeGameState::WAITING:
            drawWaiting();
            break;
        case SnakeGameState::PLAYING:
        case SnakeGameState::PAUSED:
            drawGame();
            break;
        case SnakeGameState::GAME_OVER:
            drawGameOver();
            break;
    }
}

void SnakeEffect::drawGame() {
    displayManager->fillScreen(0, 0, 0);

    // Griglia di sfondo (sottile)
    if (showGrid) {
        drawGrid();
    }

    // Bordo luminoso
    drawBorder();

    // Disegna cibo
    drawFood();

    // Disegna serpente
    drawSnake();

    // Disegna punteggio
    drawScore();

    // Overlay pausa
    if (gameState == SnakeGameState::PAUSED) {
        // Sfondo semi-trasparente (pattern scacchiera)
        for (int y = 20; y < 44; y++) {
            for (int x = 10; x < 54; x++) {
                if ((x + y) % 2 == 0) {
                    displayManager->drawPixel(x, y, 0, 0, 0);
                }
            }
        }

        displayManager->setFont(nullptr);
        displayManager->setTextSize(1);
        displayManager->setTextColor(0xFFFF);
        displayManager->setCursor(14, 28);
        displayManager->print("PAUSED");
    }
}

void SnakeEffect::drawSnake() {
    // Disegna ogni segmento
    for (size_t i = 0; i < snake.size(); i++) {
        int px = snake[i].x * GRID_SIZE;
        int py = snake[i].y * GRID_SIZE;

        uint8_t r, g, b;

        if (i == 0) {
            // Testa con pulsazione
            getSnakeHeadColor(r, g, b);
            r = (uint8_t)(r * headPulse);
            g = (uint8_t)(g * headPulse);
            b = (uint8_t)(b * headPulse);

            // Disegna testa (4x4 con dettagli)
            for (int dy = 0; dy < GRID_SIZE; dy++) {
                for (int dx = 0; dx < GRID_SIZE; dx++) {
                    // Bordo piu' scuro
                    if (dx == 0 || dy == 0 || dx == GRID_SIZE-1 || dy == GRID_SIZE-1) {
                        displayManager->drawPixel(px + dx, py + dy, r/2, g/2, b/2);
                    } else {
                        displayManager->drawPixel(px + dx, py + dy, r, g, b);
                    }
                }
            }

            // Occhi sulla testa
            int eyeX1, eyeY1, eyeX2, eyeY2;
            switch (direction) {
                case SnakeDirection::UP:
                    eyeX1 = px + 1; eyeY1 = py + 1;
                    eyeX2 = px + 2; eyeY2 = py + 1;
                    break;
                case SnakeDirection::DOWN:
                    eyeX1 = px + 1; eyeY1 = py + 2;
                    eyeX2 = px + 2; eyeY2 = py + 2;
                    break;
                case SnakeDirection::LEFT:
                    eyeX1 = px + 1; eyeY1 = py + 1;
                    eyeX2 = px + 1; eyeY2 = py + 2;
                    break;
                case SnakeDirection::RIGHT:
                    eyeX1 = px + 2; eyeY1 = py + 1;
                    eyeX2 = px + 2; eyeY2 = py + 2;
                    break;
            }
            displayManager->drawPixel(eyeX1, eyeY1, 255, 255, 255);
            displayManager->drawPixel(eyeX2, eyeY2, 255, 255, 255);

        } else {
            // Corpo con gradiente
            getSnakeBodyColor(i, r, g, b);

            // Disegna segmento corpo (3x3 centrato per effetto arrotondato)
            for (int dy = 0; dy < GRID_SIZE - 1; dy++) {
                for (int dx = 0; dx < GRID_SIZE - 1; dx++) {
                    displayManager->drawPixel(px + dx + (GRID_SIZE > 3 ? 1 : 0),
                                             py + dy + (GRID_SIZE > 3 ? 1 : 0), r, g, b);
                }
            }
        }
    }
}

void SnakeEffect::drawFood() {
    int px = food.x * GRID_SIZE;
    int py = food.y * GRID_SIZE;

    uint8_t r, g, b;
    getFoodColor(r, g, b);

    // Animazione del cibo
    int pulseFrame = animationFrame % 20;
    float pulse = 1.0 + sin(pulseFrame * 0.314) * 0.2;

    if (foodType == FoodType::NORMAL) {
        // Cibo normale: mela rossa con foglia
        // Corpo mela
        displayManager->drawPixel(px + 1, py + 1, r, g, b);
        displayManager->drawPixel(px + 2, py + 1, r, g, b);
        displayManager->drawPixel(px + 1, py + 2, r, g, b);
        displayManager->drawPixel(px + 2, py + 2, r, g, b);
        // Foglia verde
        displayManager->drawPixel(px + 2, py, 0, 200, 0);

    } else if (foodType == FoodType::BONUS) {
        // Cibo bonus: stella gialla lampeggiante
        uint8_t brightness = (uint8_t)(255 * pulse);
        displayManager->drawPixel(px + 1, py, brightness, brightness, 0);
        displayManager->drawPixel(px + 2, py, brightness, brightness, 0);
        displayManager->drawPixel(px, py + 1, brightness, brightness, 0);
        displayManager->drawPixel(px + 1, py + 1, brightness, brightness, 0);
        displayManager->drawPixel(px + 2, py + 1, brightness, brightness, 0);
        displayManager->drawPixel(px + 3, py + 1, brightness, brightness, 0);
        displayManager->drawPixel(px + 1, py + 2, brightness, brightness, 0);
        displayManager->drawPixel(px + 2, py + 2, brightness, brightness, 0);
        displayManager->drawPixel(px + 1, py + 3, brightness, brightness, 0);
        displayManager->drawPixel(px + 2, py + 3, brightness, brightness, 0);

    } else {
        // Super cibo: diamante arcobaleno
        float hue = (animationFrame % 60) / 60.0 * 360.0;
        uint16_t color = hsvToRgb565(hue, 1.0, 1.0);

        // Estrai RGB da RGB565
        uint8_t rr = ((color >> 11) & 0x1F) << 3;
        uint8_t gg = ((color >> 5) & 0x3F) << 2;
        uint8_t bb = (color & 0x1F) << 3;

        // Forma diamante
        displayManager->drawPixel(px + 1, py, rr, gg, bb);
        displayManager->drawPixel(px + 2, py, rr, gg, bb);
        displayManager->drawPixel(px, py + 1, rr, gg, bb);
        displayManager->drawPixel(px + 1, py + 1, 255, 255, 255); // Centro bianco
        displayManager->drawPixel(px + 2, py + 1, 255, 255, 255);
        displayManager->drawPixel(px + 3, py + 1, rr, gg, bb);
        displayManager->drawPixel(px + 1, py + 2, rr, gg, bb);
        displayManager->drawPixel(px + 2, py + 2, rr, gg, bb);
    }
}

void SnakeEffect::drawScore() {
    displayManager->setFont(nullptr);
    displayManager->setTextSize(1);

    // Sfondo per score (rettangolo nero)
    for (int x = 0; x < 24; x++) {
        for (int y = 0; y < 8; y++) {
            displayManager->drawPixel(x, y, 0, 0, 20);
        }
    }

    // Score
    displayManager->setTextColor(0x07E0);  // Verde
    displayManager->setCursor(1, 1);
    displayManager->print(String(score));

    // Level (in alto a destra)
    for (int x = 50; x < 64; x++) {
        for (int y = 0; y < 8; y++) {
            displayManager->drawPixel(x, y, 0, 0, 20);
        }
    }
    displayManager->setTextColor(0xFFE0);  // Giallo
    displayManager->setCursor(51, 1);
    displayManager->print("L" + String(level));
}

void SnakeEffect::drawGrid() {
    // Griglia molto sottile
    for (int x = 0; x < 64; x += GRID_SIZE) {
        for (int y = 8; y < 64; y++) {  // Inizia sotto score
            if (y % GRID_SIZE == 0 || x == 0) {
                displayManager->drawPixel(x, y, 10, 10, 20);
            }
        }
    }
}

void SnakeEffect::drawBorder() {
    // Bordo colorato basato sul livello
    uint8_t r = min(50 + level * 10, 255);
    uint8_t g = 50;
    uint8_t b = max(150 - level * 10, 50);

    // Linee orizzontali (alto e basso)
    for (int x = 0; x < 64; x++) {
        displayManager->drawPixel(x, 8, r, g, b);   // Sotto score
        displayManager->drawPixel(x, 63, r, g, b);  // Fondo
    }

    // Linee verticali (sinistra e destra)
    for (int y = 8; y < 64; y++) {
        displayManager->drawPixel(0, y, r, g, b);
        displayManager->drawPixel(63, y, r, g, b);
    }
}

void SnakeEffect::drawWaiting() {
    displayManager->fillScreen(0, 0, 0);

    // Titolo "SNAKE" con animazione
    displayManager->setFont(nullptr);
    displayManager->setTextSize(1);

    // Animazione colore titolo
    float hue = (animationFrame % 60) / 60.0 * 360.0;
    uint16_t titleColor = hsvToRgb565(hue, 1.0, 1.0);

    displayManager->setTextColor(titleColor);
    displayManager->setCursor(16, 8);
    displayManager->print("SNAKE");

    // Serpente animato decorativo
    int snakeY = 28;
    int snakeStartX = 10 + (animationFrame % 40);
    for (int i = 0; i < 8; i++) {
        uint8_t r, g, b;
        float segHue = fmod(hue + i * 30, 360.0);
        uint16_t col = hsvToRgb565(segHue, 1.0, 0.8);
        r = ((col >> 11) & 0x1F) << 3;
        g = ((col >> 5) & 0x3F) << 2;
        b = (col & 0x1F) << 3;

        int x = snakeStartX - i * 3;
        if (x >= 0 && x < 60) {
            displayManager->drawPixel(x, snakeY, r, g, b);
            displayManager->drawPixel(x + 1, snakeY, r, g, b);
            displayManager->drawPixel(x, snakeY + 1, r, g, b);
            displayManager->drawPixel(x + 1, snakeY + 1, r, g, b);
        }
    }

    // Istruzioni
    displayManager->setTextColor(0x7BEF);  // Grigio chiaro
    displayManager->setCursor(4, 42);
    displayManager->print("Join from app");

    if (playerJoined) {
        displayManager->setTextColor(0x07E0);  // Verde
        displayManager->setCursor(6, 54);
        displayManager->print("Press START!");
    } else {
        // Puntino lampeggiante
        if (animationFrame % 30 < 15) {
            displayManager->setTextColor(0xF800);
            displayManager->setCursor(24, 54);
            displayManager->print("...");
        }
    }

    // High score se esiste
    if (highScore > 0) {
        displayManager->setTextColor(0xFFE0);
        displayManager->setCursor(4, 2);
        displayManager->print("HI:" + String(highScore));
    }
}

void SnakeEffect::drawGameOver() {
    displayManager->fillScreen(0, 0, 0);

    // "GAME OVER" lampeggiante
    displayManager->setFont(nullptr);
    displayManager->setTextSize(1);

    if (animationFrame % 30 < 20) {
        displayManager->setTextColor(0xF800);  // Rosso
        displayManager->setCursor(8, 10);
        displayManager->print("GAME OVER");
    }

    // Score finale
    displayManager->setTextColor(0xFFFF);
    displayManager->setCursor(10, 26);
    displayManager->print("Score:" + String(score));

    // High score
    if (score >= highScore && score > 0) {
        displayManager->setTextColor(0xFFE0);
        displayManager->setCursor(6, 38);
        displayManager->print("NEW HIGH!");
    } else {
        displayManager->setTextColor(0x7BEF);
        displayManager->setCursor(10, 38);
        displayManager->print("Best:" + String(highScore));
    }

    // Livello raggiunto
    displayManager->setTextColor(0x07FF);  // Cyan
    displayManager->setCursor(10, 50);
    displayManager->print("Level:" + String(level));

    // Istruzioni
    displayManager->setTextColor(0x7BEF);
    displayManager->setCursor(4, 58);
    if (animationFrame % 60 < 40) {
        displayManager->print("Press RESET");
    }
}

void SnakeEffect::getSnakeHeadColor(uint8_t& r, uint8_t& g, uint8_t& b) {
    // Testa verde brillante
    r = 0;
    g = 255;
    b = 50;
}

void SnakeEffect::getSnakeBodyColor(int segmentIndex, uint8_t& r, uint8_t& g, uint8_t& b) {
    // Gradiente dal verde chiaro al verde scuro
    float t = (float)segmentIndex / max((int)snake.size(), 1);

    // Effetto arcobaleno basato sul livello
    if (level >= 5) {
        float hue = fmod(t * 120 + animationFrame * 2, 360.0);
        uint16_t col = hsvToRgb565(hue, 0.8, 0.7 - t * 0.3);
        r = ((col >> 11) & 0x1F) << 3;
        g = ((col >> 5) & 0x3F) << 2;
        b = (col & 0x1F) << 3;
    } else {
        // Gradiente verde normale
        r = (uint8_t)(30 * (1.0 - t));
        g = (uint8_t)(200 - 100 * t);
        b = (uint8_t)(80 - 40 * t);
    }
}

void SnakeEffect::getFoodColor(uint8_t& r, uint8_t& g, uint8_t& b) {
    switch (foodType) {
        case FoodType::NORMAL:
            r = 255; g = 50; b = 50;   // Rosso (mela)
            break;
        case FoodType::BONUS:
            r = 255; g = 255; b = 0;   // Giallo (stella)
            break;
        case FoodType::SUPER:
            r = 255; g = 100; b = 255; // Magenta (diamante)
            break;
    }
}

uint16_t SnakeEffect::hsvToRgb565(float h, float s, float v) {
    float c = v * s;
    float x = c * (1 - fabs(fmod(h / 60.0, 2) - 1));
    float m = v - c;

    float r, g, b;
    if (h < 60) { r = c; g = x; b = 0; }
    else if (h < 120) { r = x; g = c; b = 0; }
    else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; }
    else if (h < 300) { r = x; g = 0; b = c; }
    else { r = c; g = 0; b = x; }

    uint8_t r8 = (uint8_t)((r + m) * 255);
    uint8_t g8 = (uint8_t)((g + m) * 255);
    uint8_t b8 = (uint8_t)((b + m) * 255);

    return ((r8 & 0xF8) << 8) | ((g8 & 0xFC) << 3) | (b8 >> 3);
}

// ═══════════════════════════════════════════
// Game API
// ═══════════════════════════════════════════

bool SnakeEffect::joinGame() {
    if (!playerJoined) {
        playerJoined = true;
        gameState = SnakeGameState::WAITING;
        DEBUG_PRINTLN("[Snake] Player joined!");
        return true;
    }
    return false;
}

bool SnakeEffect::leaveGame() {
    if (playerJoined) {
        playerJoined = false;
        gameState = SnakeGameState::WAITING;
        DEBUG_PRINTLN("[Snake] Player left");
        return true;
    }
    return false;
}

void SnakeEffect::setDirection(SnakeDirection dir) {
    // Previeni inversione (non puoi andare direttamente nella direzione opposta)
    if ((direction == SnakeDirection::UP && dir == SnakeDirection::DOWN) ||
        (direction == SnakeDirection::DOWN && dir == SnakeDirection::UP) ||
        (direction == SnakeDirection::LEFT && dir == SnakeDirection::RIGHT) ||
        (direction == SnakeDirection::RIGHT && dir == SnakeDirection::LEFT)) {
        return;
    }
    nextDirection = dir;
}

void SnakeEffect::setDirectionFromString(const String& dir) {
    String d = dir;
    d.toLowerCase();

    if (d == "u" || d == "up") setDirection(SnakeDirection::UP);
    else if (d == "d" || d == "down") setDirection(SnakeDirection::DOWN);
    else if (d == "l" || d == "left") setDirection(SnakeDirection::LEFT);
    else if (d == "r" || d == "right") setDirection(SnakeDirection::RIGHT);
}

void SnakeEffect::startGame() {
    if (gameState == SnakeGameState::WAITING || gameState == SnakeGameState::GAME_OVER) {
        resetGame();
        gameState = SnakeGameState::PLAYING;
        DEBUG_PRINTLN("[Snake] Game started!");
    }
}

void SnakeEffect::pauseGame() {
    if (gameState == SnakeGameState::PLAYING) {
        gameState = SnakeGameState::PAUSED;
        DEBUG_PRINTLN("[Snake] Game paused");
    }
}

void SnakeEffect::resumeGame() {
    if (gameState == SnakeGameState::PAUSED) {
        gameState = SnakeGameState::PLAYING;
        lastMoveTime = millis();  // Reset timer per evitare movimento immediato
        DEBUG_PRINTLN("[Snake] Game resumed");
    }
}

void SnakeEffect::resetToWaiting() {
    gameState = SnakeGameState::WAITING;
    DEBUG_PRINTLN("[Snake] Reset to waiting");
}

String SnakeEffect::getStateString() const {
    // SNAKE_STATE,gameState,score,highScore,level,length,foodX,foodY,foodType,direction,playerJoined
    String state = "SNAKE_STATE";
    state += ",";
    switch (gameState) {
        case SnakeGameState::WAITING: state += "waiting"; break;
        case SnakeGameState::PLAYING: state += "playing"; break;
        case SnakeGameState::PAUSED: state += "paused"; break;
        case SnakeGameState::GAME_OVER: state += "gameover"; break;
    }
    state += "," + String(score);
    state += "," + String(highScore);
    state += "," + String(level);
    state += "," + String(snake.size());
    state += "," + String(food.x);
    state += "," + String(food.y);
    state += "," + String((int)foodType);

    String dirStr;
    switch (direction) {
        case SnakeDirection::UP: dirStr = "up"; break;
        case SnakeDirection::DOWN: dirStr = "down"; break;
        case SnakeDirection::LEFT: dirStr = "left"; break;
        case SnakeDirection::RIGHT: dirStr = "right"; break;
    }
    state += "," + dirStr;
    state += "," + String(playerJoined ? "1" : "0");

    return state;
}
