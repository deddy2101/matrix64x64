#include "PongEffect.h"

PongEffect::PongEffect(DisplayManager* dm)
    : Effect(dm),
      ballX(0), ballY(0),
      ballSpeedX(0), ballSpeedY(0),
      baseBallSpeed(1.0),
      paddle1Y(0), paddle2Y(0),
      score1(0), score2(0),
      player1Mode(PlayerMode::AI),
      player2Mode(PlayerMode::AI),
      gameState(PongGameState::PLAYING),
      winScore(MAX_SCORE),
      player1Input(0), player2Input(0),
      paddleSpeed(2),
      countdownValue(0),
      countdownStart(0) {
}

void PongEffect::init() {
    DEBUG_PRINTLN("[Pong] Initializing");

    int width = displayManager->getWidth();
    int height = displayManager->getHeight();

    ballX = width / 2;
    ballY = height / 2;
    paddle1Y = (height - PADDLE_HEIGHT) / 2;
    paddle2Y = (height - PADDLE_HEIGHT) / 2;
    score1 = 0;
    score2 = 0;

    // Reset players to AI if no one joined
    if (player1Mode == PlayerMode::HUMAN || player2Mode == PlayerMode::HUMAN) {
        gameState = PongGameState::WAITING;
    } else {
        gameState = PongGameState::PLAYING;
        resetBall();
    }

    player1Input = 0;
    player2Input = 0;

    displayManager->fillScreen(0, 0, 0);
}

void PongEffect::update() {
    switch (gameState) {
        case PongGameState::WAITING:
            // Non fare nulla, aspetta giocatori
            break;

        case PongGameState::PLAYING:
            updateBall();
            updatePaddles();

            // Controlla vittoria
            if (score1 >= winScore || score2 >= winScore) {
                gameState = PongGameState::GAME_OVER;
                DEBUG_PRINTF("[Pong] Game Over! P1: %d, P2: %d\n", score1, score2);
            }
            break;

        case PongGameState::PAUSED:
            // Non aggiornare nulla
            break;

        case PongGameState::GAME_OVER:
            // Aspetta reset
            break;
    }
}

void PongEffect::updateBall() {
    int width = displayManager->getWidth();
    int height = displayManager->getHeight();

    ballX += ballSpeedX;
    ballY += ballSpeedY;

    // Rimbalzo top/bottom
    if (ballY <= 0) {
        ballY = 0;
        ballSpeedY = abs(ballSpeedY);
    }
    if (ballY >= height - 2) {
        ballY = height - 2;
        ballSpeedY = -abs(ballSpeedY);
    }

    // Collisione paddle sinistro
    if (ballX <= PADDLE_WIDTH + 1 && ballX >= 0) {
        if (ballY + 2 >= paddle1Y && ballY <= paddle1Y + PADDLE_HEIGHT) {
            ballSpeedX = abs(ballSpeedX);
            // Aggiungi effetto spin basato su dove colpisce il paddle
            float hitPos = (ballY - paddle1Y) / (float)PADDLE_HEIGHT;
            ballSpeedY = (hitPos - 0.5f) * 2.0f * baseBallSpeed;
        }
    }

    // Collisione paddle destro
    if (ballX >= width - PADDLE_WIDTH - 2 && ballX <= width) {
        if (ballY + 2 >= paddle2Y && ballY <= paddle2Y + PADDLE_HEIGHT) {
            ballSpeedX = -abs(ballSpeedX);
            float hitPos = (ballY - paddle2Y) / (float)PADDLE_HEIGHT;
            ballSpeedY = (hitPos - 0.5f) * 2.0f * baseBallSpeed;
        }
    }

    // Punto segnato
    if (ballX < -5) {
        score2++;
        DEBUG_PRINTF("[Pong] Point P2! Score: %d-%d\n", score1, score2);
        resetBall();
    }
    if (ballX >= width + 5) {
        score1++;
        DEBUG_PRINTF("[Pong] Point P1! Score: %d-%d\n", score1, score2);
        resetBall();
    }
}

void PongEffect::updatePaddles() {
    int height = displayManager->getHeight();

    // Player 1
    if (player1Mode == PlayerMode::HUMAN) {
        paddle1Y += player1Input * paddleSpeed;
    } else {
        // AI semplice
        int targetY = (int)ballY - PADDLE_HEIGHT / 2;
        if (paddle1Y < targetY) paddle1Y++;
        if (paddle1Y > targetY) paddle1Y--;
    }

    // Player 2
    if (player2Mode == PlayerMode::HUMAN) {
        paddle2Y += player2Input * paddleSpeed;
    } else {
        // AI semplice
        int targetY = (int)ballY - PADDLE_HEIGHT / 2;
        if (paddle2Y < targetY) paddle2Y++;
        if (paddle2Y > targetY) paddle2Y--;
    }

    // Limita i paddle
    paddle1Y = constrain(paddle1Y, 0, height - PADDLE_HEIGHT);
    paddle2Y = constrain(paddle2Y, 0, height - PADDLE_HEIGHT);
}

void PongEffect::resetBall() {
    int width = displayManager->getWidth();
    int height = displayManager->getHeight();

    ballX = width / 2;
    ballY = height / 2;

    // Direzione casuale
    ballSpeedX = (random(2) == 0 ? 1 : -1) * baseBallSpeed;
    ballSpeedY = (random(100) - 50) / 100.0f * baseBallSpeed;
}

void PongEffect::draw() {
    switch (gameState) {
        case PongGameState::WAITING:
            drawWaiting();
            break;
        case PongGameState::PLAYING:
            drawGame();
            break;
        case PongGameState::PAUSED:
            drawGame();
            // Aggiungi "PAUSED" overlay
            displayManager->setTextSize(1);
            displayManager->setTextColor(0xFFFF);
            displayManager->setCursor(18, 28);
            displayManager->print("PAUSED");
            break;
        case PongGameState::GAME_OVER:
            drawGameOver();
            break;
    }
}

void PongEffect::drawGame() {
    int width = displayManager->getWidth();
    int height = displayManager->getHeight();

    displayManager->fillScreen(0, 0, 0);

    // Disegna linea centrale tratteggiata
    for (int y = 0; y < height; y += 4) {
        displayManager->drawPixel(width / 2, y, 40, 40, 40);
        displayManager->drawPixel(width / 2, y + 1, 40, 40, 40);
    }

    // Disegna punteggio
    drawScore();

    // Disegna paddle sinistro (verde se umano, grigio se AI)
    uint8_t p1r = player1Mode == PlayerMode::HUMAN ? 0 : 100;
    uint8_t p1g = player1Mode == PlayerMode::HUMAN ? 255 : 100;
    uint8_t p1b = player1Mode == PlayerMode::HUMAN ? 0 : 100;
    for (int i = 0; i < PADDLE_HEIGHT; i++) {
        for (int w = 0; w < PADDLE_WIDTH; w++) {
            displayManager->drawPixel(w, paddle1Y + i, p1r, p1g, p1b);
        }
    }

    // Disegna paddle destro (rosso se umano, grigio se AI)
    uint8_t p2r = player2Mode == PlayerMode::HUMAN ? 255 : 100;
    uint8_t p2g = player2Mode == PlayerMode::HUMAN ? 0 : 100;
    uint8_t p2b = player2Mode == PlayerMode::HUMAN ? 0 : 100;
    for (int i = 0; i < PADDLE_HEIGHT; i++) {
        for (int w = 0; w < PADDLE_WIDTH; w++) {
            displayManager->drawPixel(width - PADDLE_WIDTH + w, paddle2Y + i, p2r, p2g, p2b);
        }
    }

    // Disegna pallina (2x2)
    int bx = (int)ballX;
    int by = (int)ballY;
    displayManager->drawPixel(bx, by, 255, 255, 255);
    displayManager->drawPixel(bx + 1, by, 255, 255, 255);
    displayManager->drawPixel(bx, by + 1, 255, 255, 255);
    displayManager->drawPixel(bx + 1, by + 1, 255, 255, 255);
}

void PongEffect::drawScore() {
    displayManager->setFont(nullptr);
    displayManager->setTextSize(1);

    // Score P1 (verde)
    displayManager->setTextColor(player1Mode == PlayerMode::HUMAN ? 0x07E0 : 0x39E7);
    displayManager->setCursor(20, 2);
    displayManager->print(String(score1));

    // Score P2 (rosso)
    displayManager->setTextColor(player2Mode == PlayerMode::HUMAN ? 0xF800 : 0x39E7);
    displayManager->setCursor(40, 2);
    displayManager->print(String(score2));
}

void PongEffect::drawWaiting() {
    displayManager->fillScreen(0, 0, 0);

    displayManager->setFont(nullptr);
    displayManager->setTextSize(1);
    displayManager->setTextColor(0xFFFF);

    displayManager->setCursor(16, 10);
    displayManager->print("PONG");

    displayManager->setTextColor(0x07E0);
    displayManager->setCursor(4, 25);
    displayManager->print(String("P1:") + (player1Mode == PlayerMode::HUMAN ? "OK" : "--"));

    displayManager->setTextColor(0xF800);
    displayManager->setCursor(34, 25);
    displayManager->print(String("P2:") + (player2Mode == PlayerMode::HUMAN ? "OK" : "--"));

    // Istruzioni
    displayManager->setTextColor(0x7BEF);
    displayManager->setCursor(4, 45);
    displayManager->print("Join from app");

    displayManager->setCursor(4, 55);
    if (getPlayerCount() > 0) {
        displayManager->setTextColor(0xFFE0);
        displayManager->print("Ready? START!");
    }
}

void PongEffect::drawGameOver() {
    displayManager->fillScreen(0, 0, 0);

    displayManager->setFont(nullptr);
    displayManager->setTextSize(1);

    // Winner
    displayManager->setCursor(8, 15);
    if (score1 > score2) {
        displayManager->setTextColor(0x07E0);
        displayManager->print("P1 WINS!");
    } else {
        displayManager->setTextColor(0xF800);
        displayManager->print("P2 WINS!");
    }

    // Final score
    displayManager->setTextColor(0xFFFF);
    displayManager->setCursor(20, 35);
    displayManager->print(String(score1) + " - " + String(score2));

    displayManager->setTextColor(0x7BEF);
    displayManager->setCursor(8, 52);
    displayManager->print("Send RESET");
}

// ═══════════════════════════════════════════
// Multiplayer API
// ═══════════════════════════════════════════

bool PongEffect::joinPlayer(int playerNum) {
    if (playerNum == 1 && player1Mode == PlayerMode::AI) {
        player1Mode = PlayerMode::HUMAN;
        DEBUG_PRINTLN("[Pong] Player 1 joined!");

        // Se siamo in playing con AI, passa a waiting
        if (gameState == PongGameState::PLAYING || gameState == PongGameState::GAME_OVER) {
            gameState = PongGameState::WAITING;
            score1 = 0;
            score2 = 0;
        }
        return true;
    }
    if (playerNum == 2 && player2Mode == PlayerMode::AI) {
        player2Mode = PlayerMode::HUMAN;
        DEBUG_PRINTLN("[Pong] Player 2 joined!");

        if (gameState == PongGameState::PLAYING || gameState == PongGameState::GAME_OVER) {
            gameState = PongGameState::WAITING;
            score1 = 0;
            score2 = 0;
        }
        return true;
    }
    return false;
}

bool PongEffect::leavePlayer(int playerNum) {
    if (playerNum == 1 && player1Mode == PlayerMode::HUMAN) {
        player1Mode = PlayerMode::AI;
        player1Input = 0;
        DEBUG_PRINTLN("[Pong] Player 1 left");

        // Se nessun umano, torna a AI vs AI
        if (player2Mode == PlayerMode::AI) {
            gameState = PongGameState::PLAYING;
            resetGame();
        }
        return true;
    }
    if (playerNum == 2 && player2Mode == PlayerMode::HUMAN) {
        player2Mode = PlayerMode::AI;
        player2Input = 0;
        DEBUG_PRINTLN("[Pong] Player 2 left");

        if (player1Mode == PlayerMode::AI) {
            gameState = PongGameState::PLAYING;
            resetGame();
        }
        return true;
    }
    return false;
}

bool PongEffect::isPlayerJoined(int playerNum) const {
    if (playerNum == 1) return player1Mode == PlayerMode::HUMAN;
    if (playerNum == 2) return player2Mode == PlayerMode::HUMAN;
    return false;
}

int PongEffect::getPlayerCount() const {
    int count = 0;
    if (player1Mode == PlayerMode::HUMAN) count++;
    if (player2Mode == PlayerMode::HUMAN) count++;
    return count;
}

void PongEffect::movePlayer(int playerNum, int direction) {
    // direction: -1 = su, 0 = stop, 1 = giù
    direction = constrain(direction, -1, 1);

    if (playerNum == 1 && player1Mode == PlayerMode::HUMAN) {
        player1Input = direction;
    }
    if (playerNum == 2 && player2Mode == PlayerMode::HUMAN) {
        player2Input = direction;
    }
}

void PongEffect::startGame() {
    if (gameState == PongGameState::WAITING || gameState == PongGameState::GAME_OVER) {
        score1 = 0;
        score2 = 0;
        int height = displayManager->getHeight();
        paddle1Y = (height - PADDLE_HEIGHT) / 2;
        paddle2Y = (height - PADDLE_HEIGHT) / 2;
        resetBall();
        gameState = PongGameState::PLAYING;
        DEBUG_PRINTLN("[Pong] Game started!");
    }
}

void PongEffect::pauseGame() {
    if (gameState == PongGameState::PLAYING) {
        gameState = PongGameState::PAUSED;
        DEBUG_PRINTLN("[Pong] Game paused");
    }
}

void PongEffect::resumeGame() {
    if (gameState == PongGameState::PAUSED) {
        gameState = PongGameState::PLAYING;
        DEBUG_PRINTLN("[Pong] Game resumed");
    }
}

void PongEffect::resetGame() {
    score1 = 0;
    score2 = 0;
    int height = displayManager->getHeight();
    paddle1Y = (height - PADDLE_HEIGHT) / 2;
    paddle2Y = (height - PADDLE_HEIGHT) / 2;
    player1Input = 0;
    player2Input = 0;
    resetBall();

    if (getPlayerCount() > 0) {
        gameState = PongGameState::WAITING;
    } else {
        gameState = PongGameState::PLAYING;
    }
    DEBUG_PRINTLN("[Pong] Game reset");
}

String PongEffect::getStateString() const {
    // PONG_STATE,gameState,score1,score2,p1Mode,p2Mode,ballX,ballY
    String state = "PONG_STATE";
    state += ",";
    switch (gameState) {
        case PongGameState::WAITING: state += "waiting"; break;
        case PongGameState::PLAYING: state += "playing"; break;
        case PongGameState::PAUSED: state += "paused"; break;
        case PongGameState::GAME_OVER: state += "gameover"; break;
    }
    state += "," + String(score1);
    state += "," + String(score2);
    state += "," + String(player1Mode == PlayerMode::HUMAN ? "human" : "ai");
    state += "," + String(player2Mode == PlayerMode::HUMAN ? "human" : "ai");
    state += "," + String((int)ballX);
    state += "," + String((int)ballY);
    return state;
}
