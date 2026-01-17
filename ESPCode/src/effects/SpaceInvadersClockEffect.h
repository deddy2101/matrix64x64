#ifndef SPACE_INVADERS_CLOCK_EFFECT_H
#define SPACE_INVADERS_CLOCK_EFFECT_H

#include "../Effect.h"
#include "../TimeManager.h"

// Stati della navetta
enum ShipState {
    SHIP_IDLE,
    SHIP_PLAYING,      // Playing the game (shooting aliens)
    SHIP_MOVING,       // Moving to shoot a digit
    SHIP_SHOOTING,     // Shooting at a digit
    SHIP_WAITING       // Waiting for explosion to complete
};

// Proiettile
struct Bullet {
    int x, y;
    bool active;
    int targetDigit;  // 0-3 (HH:MM), -1 = shooting alien
};

// Alieno
struct Alien {
    int x, y;
    bool alive;
    int type;  // 0, 1, 2 for different alien types
};

// Explosion per alieni
struct AlienExplosion {
    int x, y;
    int frame;
    bool active;
};

#define MAX_ALIENS 12
#define MAX_ALIEN_EXPLOSIONS 4

class SpaceInvadersClockEffect : public Effect {
private:
    TimeManager* timeManager;

    // Ship position and state
    int shipX, shipY;
    ShipState shipState;
    int shipTargetX;
    bool shipMovingRight;
    int shipDirection;  // -1 left, 0 stop, 1 right (for game mode)

    // Bullet
    Bullet bullet;
    unsigned long lastBulletUpdate;

    // Aliens
    Alien aliens[MAX_ALIENS];
    int aliensAlive;
    int alienDirection;  // 1 = right, -1 = left
    int alienBaseY;
    unsigned long lastAlienUpdate;
    unsigned long lastAlienShootTime;

    // Alien explosions
    AlienExplosion alienExplosions[MAX_ALIEN_EXPLOSIONS];
    unsigned long lastAlienExplosionUpdate;

    // Digit explosion animation
    int explodingDigit;  // -1 = none, 0-3 = which digit
    int explosionFrame;
    unsigned long lastExplosionUpdate;

    // Time tracking
    int lastHour;
    int lastMinute;
    int displayedDigits[4];  // Current displayed digits HH MM

    // Which digits need to change
    bool digitNeedsUpdate[4];
    int currentTargetDigit;  // Which digit we're currently targeting

    // Timing
    unsigned long lastShipUpdate;
    unsigned long lastUpdate;
    bool needsRedraw;

    // Callback
    int minuteCallbackId;

    // Ship sprite (11x8)
    static const int SHIP_WIDTH = 11;
    static const int SHIP_HEIGHT = 8;

    // Alien size
    static const int ALIEN_WIDTH = 8;
    static const int ALIEN_HEIGHT = 6;

    // Digit positions
    int digitX[4];
    int digitY;

    // Colors
    static const uint8_t SHIP_COLOR_R = 0;
    static const uint8_t SHIP_COLOR_G = 255;
    static const uint8_t SHIP_COLOR_B = 0;

    static const uint8_t BULLET_COLOR_R = 255;
    static const uint8_t BULLET_COLOR_G = 255;
    static const uint8_t BULLET_COLOR_B = 255;

    static const uint8_t DIGIT_COLOR_R = 255;
    static const uint8_t DIGIT_COLOR_G = 255;
    static const uint8_t DIGIT_COLOR_B = 255;

    static const uint8_t EXPLOSION_COLOR_R = 255;
    static const uint8_t EXPLOSION_COLOR_G = 165;
    static const uint8_t EXPLOSION_COLOR_B = 0;

    // Drawing
    void drawScene();
    void drawShip();
    void drawBullet();
    void drawDigits();
    void drawDigit(int digit, int x, int y, uint8_t r, uint8_t g, uint8_t b);
    void drawColon();
    void drawExplosion(int digitIndex);
    void drawAliens();
    void drawAlien(int x, int y, int type, bool frame);
    void drawAlienExplosions();

    // Logic
    void updateShip();
    void updateBullet();
    void updateExplosion();
    void updateAliens();
    void updateAlienExplosions();
    void updateGame();
    void startShooting(int digitIndex);
    void onTimeChange(int newHour, int newMinute);
    int findNextDigitToUpdate();
    int getDigitCenterX(int digitIndex);
    void initAliens();
    void respawnAliens();
    void fireAtAlien();
    int findNearestAlien();
    void addAlienExplosion(int x, int y);

    // Font 5x7 for digits
    static const uint8_t FONT_5X7[][7];

public:
    SpaceInvadersClockEffect(DisplayManager* dm, TimeManager* tm);
    ~SpaceInvadersClockEffect();

    void init() override;
    void update() override;
    void draw() override;
    void cleanup() override;
    const char* getName() override { return "Space Invaders"; }
    void reset() override;
};

#endif // SPACE_INVADERS_CLOCK_EFFECT_H
