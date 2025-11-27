#ifndef PONG_EFFECT_H
#define PONG_EFFECT_H

#include "../Effect.h"

class PongEffect : public Effect {
private:
    // Game state
    float ballX, ballY;
    float ballSpeedX, ballSpeedY;
    int paddle1Y, paddle2Y;
    int score1, score2;
    
    // Constants
    static const int PADDLE_HEIGHT = 12;
    static const int PADDLE_WIDTH = 2;
    
    void resetBall();
    void updateBall();
    void updatePaddles();
    void drawGame();
    
public:
    PongEffect(DisplayManager* dm);
    
    void init() override;
    void update() override;
    void draw() override;
    const char* getName() override { return "Pong"; }
};

#endif
