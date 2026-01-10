#ifndef SNAKE_EFFECT_H
#define SNAKE_EFFECT_H

#include "../Effect.h"
#include <vector>

// Stato del gioco Snake
enum class SnakeGameState {
    WAITING,    // In attesa del giocatore
    PLAYING,    // Gioco in corso
    PAUSED,     // Gioco in pausa
    GAME_OVER   // Fine gioco
};

// Direzione del serpente
enum class SnakeDirection {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

// Struttura per rappresentare una posizione
struct SnakeSegment {
    int x;
    int y;
};

// Tipo di cibo speciale
enum class FoodType {
    NORMAL,     // Cibo normale - 1 punto
    BONUS,      // Cibo bonus - 3 punti (lampeggiante)
    SUPER       // Super cibo - 5 punti (arcobaleno)
};

class SnakeEffect : public Effect {
private:
    // Game state
    std::vector<SnakeSegment> snake;
    SnakeSegment food;
    FoodType foodType;
    SnakeDirection direction;
    SnakeDirection nextDirection;
    SnakeGameState gameState;

    int score;
    int highScore;
    int level;
    int foodEaten;

    // Timing
    unsigned long lastMoveTime;
    unsigned long moveInterval;     // ms tra i movimenti (diminuisce col livello)
    unsigned long foodSpawnTime;    // Quando il cibo e' stato generato
    unsigned long bonusFoodDuration; // Durata cibo bonus

    // Visual effects
    unsigned long animationTimer;
    int animationFrame;
    float headPulse;          // Pulsazione della testa
    int trailFade;            // Fade della coda
    bool showGrid;            // Mostra griglia di sfondo

    // Game constants
    static const int GRID_SIZE = 4;       // Ogni cella e' 4x4 pixel
    static const int GRID_WIDTH = 16;     // 64/4 = 16 celle
    static const int GRID_HEIGHT = 16;    // 64/4 = 16 celle
    static const int INITIAL_LENGTH = 3;
    static const int BASE_MOVE_INTERVAL = 200;  // ms
    static const int MIN_MOVE_INTERVAL = 80;    // Velocita' massima

    // Player state
    bool playerJoined;

    // Private methods
    void resetGame();
    void spawnFood();
    bool isPositionOnSnake(int x, int y);
    void moveSnake();
    bool checkCollision();
    void growSnake();
    void updateLevel();

    // Drawing methods
    void drawGame();
    void drawWaiting();
    void drawGameOver();
    void drawSnake();
    void drawFood();
    void drawScore();
    void drawGrid();
    void drawBorder();
    void drawExplosion();  // Animazione morte

    // Color helpers
    void getSnakeHeadColor(uint8_t& r, uint8_t& g, uint8_t& b);
    void getSnakeBodyColor(int segmentIndex, uint8_t& r, uint8_t& g, uint8_t& b);
    void getFoodColor(uint8_t& r, uint8_t& g, uint8_t& b);
    uint16_t hsvToRgb565(float h, float s, float v);

public:
    SnakeEffect(DisplayManager* dm);

    void init() override;
    void update() override;
    void draw() override;
    const char* getName() override { return "Snake"; }

    // ═══════════════════════════════════════════
    // Game API (chiamate da CommandHandler)
    // ═══════════════════════════════════════════

    // Gestione giocatore
    bool joinGame();
    bool leaveGame();
    bool isPlayerJoined() const { return playerJoined; }

    // Controlli direzione
    void setDirection(SnakeDirection dir);
    void setDirectionFromString(const String& dir);

    // Gestione partita
    void startGame();
    void pauseGame();
    void resumeGame();
    void resetToWaiting();

    // Stato
    SnakeGameState getGameState() const { return gameState; }
    int getScore() const { return score; }
    int getHighScore() const { return highScore; }
    int getLevel() const { return level; }
    int getSnakeLength() const { return snake.size(); }

    // Genera stringa stato per broadcast
    String getStateString() const;
};

#endif
