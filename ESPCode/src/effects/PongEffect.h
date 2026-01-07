#ifndef PONG_EFFECT_H
#define PONG_EFFECT_H

#include "../Effect.h"

// Modalità giocatore
enum class PlayerMode {
    AI,     // Controllato da AI
    HUMAN   // Controllato da app
};

// Stato del gioco
enum class PongGameState {
    WAITING,    // In attesa di giocatori
    PLAYING,    // Partita in corso
    PAUSED,     // Partita in pausa
    GAME_OVER   // Fine partita
};

class PongEffect : public Effect {
private:
    // Game state
    float ballX, ballY;
    float ballSpeedX, ballSpeedY;
    float baseBallSpeed;
    int paddle1Y, paddle2Y;
    int score1, score2;

    // Multiplayer state
    PlayerMode player1Mode;
    PlayerMode player2Mode;
    PongGameState gameState;
    int winScore;  // Punteggio per vincere

    // Input state (per movimento continuo)
    int player1Input;  // -1 = su, 0 = fermo, 1 = giù
    int player2Input;
    int paddleSpeed;

    // Countdown
    int countdownValue;
    unsigned long countdownStart;

    // Constants
    static const int PADDLE_HEIGHT = 12;
    static const int PADDLE_WIDTH = 2;
    static const int MAX_SCORE = 5;

    void resetBall();
    void updateBall();
    void updatePaddles();
    void drawGame();
    void drawWaiting();
    void drawCountdown();
    void drawGameOver();
    void drawScore();
    void startCountdown();

public:
    PongEffect(DisplayManager* dm);

    void init() override;
    void update() override;
    void draw() override;
    const char* getName() override { return "Pong"; }

    // ═══════════════════════════════════════════
    // Multiplayer API
    // ═══════════════════════════════════════════

    // Gestione giocatori
    bool joinPlayer(int playerNum);  // true se join ok
    bool leavePlayer(int playerNum);
    bool isPlayerJoined(int playerNum) const;
    int getPlayerCount() const;

    // Controlli
    void movePlayer(int playerNum, int direction);  // direction: -1=su, 0=stop, 1=giù

    // Gestione partita
    void startGame();
    void pauseGame();
    void resumeGame();
    void resetGame();

    // Stato
    PongGameState getGameState() const { return gameState; }
    int getScore1() const { return score1; }
    int getScore2() const { return score2; }
    PlayerMode getPlayer1Mode() const { return player1Mode; }
    PlayerMode getPlayer2Mode() const { return player2Mode; }

    // Genera stringa stato per broadcast
    String getStateString() const;
};

#endif
