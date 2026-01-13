#ifndef PACMAN_CLOCK_EFFECT_H
#define PACMAN_CLOCK_EFFECT_H

#include "../Effect.h"
#include "../TimeManager.h"
#include "../../include/pacman_assets.h"

// Direzioni di movimento
enum PacDirection {
    PAC_RIGHT = 0,
    PAC_DOWN = 1,
    PAC_LEFT = 2,
    PAC_UP = 3
};

// Tipi di blocchi nella mappa
enum MapBlock {
    BLOCK_EMPTY = 0,
    BLOCK_FOOD = 1,
    BLOCK_WALL = 2,
    BLOCK_GATE = 3,
    BLOCK_SUPER_FOOD = 4,
    BLOCK_CLOCK = 5,
    BLOCK_GHOST = 6,
    BLOCK_PACMAN = 7,
    BLOCK_OUT_OF_MAP = 99
};

// Stato di PacMan
enum PacState {
    PAC_MOVING,
    PAC_STOPPED,
    PAC_TURNING,
    PAC_INVINCIBLE
};

// Punto per BFS
struct PacPoint {
    int x, y;
};

class PacManClockEffect : public Effect {
private:
    TimeManager* timeManager;

    // Mappa di gioco (copia modificabile)
    static const int MAP_SIZE = 12;
    uint8_t gameMap[MAP_SIZE][MAP_SIZE];

    // Posizione e stato Pacman
    int pacX, pacY;           // Posizione pixel
    int pacStartX, pacStartY; // Posizione iniziale
    PacDirection pacDirection;
    PacState pacState;
    bool pacAnimFrame;        // Frame animazione (alterna sprite)
    uint16_t pacColor;        // Colore corrente (per invincibilita')
    unsigned long invincibleTimeout;

    // Sprite ruotati/flippati per direzione corrente
    uint16_t currentSprite[2][25];

    // BFS per pathfinding
    static const int MAX_QUEUE_SIZE = MAP_SIZE * MAP_SIZE;
    PacPoint bfsQueue[MAX_QUEUE_SIZE];
    int queueFront, queueRear;
    bool visited[MAP_SIZE][MAP_SIZE];
    PacPoint parent[MAP_SIZE][MAP_SIZE];

    // Timer
    unsigned long lastPacmanUpdate;
    unsigned long lastClockUpdate;
    unsigned long lastSecondBlink;

    // Stato display
    bool showSeconds;
    bool needsRedraw;

    // Blocchi validi per movimento
    const int movingBlocks[4] = {3, BLOCK_EMPTY, BLOCK_FOOD, BLOCK_GATE};
    const int blockingBlocks[4] = {3, BLOCK_OUT_OF_MAP, BLOCK_WALL, BLOCK_CLOCK};

    // Costanti mappa
    static const int MAP_BORDER_SIZE = 2;
    static const int MAP_MIN_POS = 0 + MAP_BORDER_SIZE;
    static const int MAP_MAX_POS = 64 - MAP_BORDER_SIZE;
    static const int BLOCK_SIZE = 5;  // Pixel per blocco

    // Metodi privati - Disegno
    void drawMap();
    void drawClock();
    void drawPacman();
    void drawColonBlink();

    // Metodi privati - Logica Pacman
    void updatePacman();
    void movePacman();
    void turnPacman(PacDirection dir);
    void flipSprite();
    void rotateSprite();
    void changePacmanColor(uint16_t newColor);

    // Metodi privati - Mappa
    MapBlock getNextBlock(PacDirection dir);
    MapBlock getNextBlock();
    bool contains(int value, const int* values);
    int countBlocks(MapBlock type);
    void resetMap();
    int mapToPixel(int mapCoord);
    int pixelToMap(int pixelCoord);

    // Metodi privati - BFS Pathfinding
    bool isValidCell(int r, int c);
    bool isTargetCell(int r, int c);
    bool findShortestPath(int startR, int startC, PacDirection& nextMove);
    void reconstructPath(PacPoint start, PacPoint end, PacDirection& nextMove);
    void directionDecision();
    void turnRandom();
    int _callbackID;

public:
    PacManClockEffect(DisplayManager* dm, TimeManager* tm);
    ~PacManClockEffect();

    void init() override;
    void update() override;
    void draw() override;
    void cleanup() override;
    const char* getName() override { return "Pac-Man Clock"; }
    void reset() override;
};

#endif // PACMAN_CLOCK_EFFECT_H
