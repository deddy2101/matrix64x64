#include "PongEffect.h"

PongEffect::PongEffect(DisplayManager* dm) 
    : Effect(dm),
      ballX(0), ballY(0),
      ballSpeedX(0.8), ballSpeedY(0.6),
      paddle1Y(0), paddle2Y(0),
      score1(0), score2(0) {
}

void PongEffect::init() {
    DEBUG_PRINTLN("Initializing Pong Effect");
    
    int width = displayManager->getWidth();
    int height = displayManager->getHeight();
    
    ballX = width / 2;
    ballY = height / 2;
    paddle1Y = height / 2;
    paddle2Y = height / 2;
    score1 = 0;
    score2 = 0;
    
    displayManager->fillScreen(0, 0, 0);
}

void PongEffect::update() {
    updateBall();
    updatePaddles();
}

void PongEffect::updateBall() {
    int width = displayManager->getWidth();
    int height = displayManager->getHeight();
    
    ballX += ballSpeedX;
    ballY += ballSpeedY;
    
    // Rimbalzo top/bottom
    if (ballY <= 0 || ballY >= height - 1) {
        ballSpeedY = -ballSpeedY;
    }
    
    // Collisione paddle sinistro
    if (ballX <= PADDLE_WIDTH && ballY >= paddle1Y && ballY <= paddle1Y + PADDLE_HEIGHT) {
        ballSpeedX = abs(ballSpeedX);
    }
    
    // Collisione paddle destro
    if (ballX >= width - PADDLE_WIDTH - 1 && ballY >= paddle2Y && ballY <= paddle2Y + PADDLE_HEIGHT) {
        ballSpeedX = -abs(ballSpeedX);
    }
    
    // Punto segnato
    if (ballX < 0) {
        score2++;
        resetBall();
    }
    if (ballX >= width) {
        score1++;
        resetBall();
    }
}

void PongEffect::updatePaddles() {
    int height = displayManager->getHeight();
    
    // AI per paddle 1
    if (ballY > paddle1Y + PADDLE_HEIGHT / 2) paddle1Y++;
    if (ballY < paddle1Y + PADDLE_HEIGHT / 2) paddle1Y--;
    
    // AI per paddle 2
    if (ballY > paddle2Y + PADDLE_HEIGHT / 2) paddle2Y++;
    if (ballY < paddle2Y + PADDLE_HEIGHT / 2) paddle2Y--;
    
    // Limita i paddle
    paddle1Y = constrain(paddle1Y, 0, height - PADDLE_HEIGHT);
    paddle2Y = constrain(paddle2Y, 0, height - PADDLE_HEIGHT);
}

void PongEffect::resetBall() {
    int width = displayManager->getWidth();
    int height = displayManager->getHeight();
    
    ballX = width / 2;
    ballY = height / 2;
}

void PongEffect::draw() {
    int width = displayManager->getWidth();
    int height = displayManager->getHeight();
    
    displayManager->fillScreen(0, 0, 0);
    
    // Disegna paddle sinistro (verde)
    for (int i = 0; i < PADDLE_HEIGHT; i++) {
        for (int w = 0; w < PADDLE_WIDTH; w++) {
            displayManager->drawPixel(w, paddle1Y + i, 0, 255, 0);
        }
    }
    
    // Disegna paddle destro (rosso)
    for (int i = 0; i < PADDLE_HEIGHT; i++) {
        for (int w = 0; w < PADDLE_WIDTH; w++) {
            displayManager->drawPixel(width - PADDLE_WIDTH + w, paddle2Y + i, 255, 0, 0);
        }
    }
    
    // Disegna pallina
    displayManager->drawPixel((int)ballX, (int)ballY, 255, 255, 255);
    displayManager->drawPixel((int)ballX + 1, (int)ballY, 255, 255, 255);
    displayManager->drawPixel((int)ballX, (int)ballY + 1, 255, 255, 255);
    displayManager->drawPixel((int)ballX + 1, (int)ballY + 1, 255, 255, 255);
    
    // Disegna linea centrale
    for (int y = 0; y < height; y += 4) {
        displayManager->drawPixel(width / 2, y, 50, 50, 50);
    }
}
