#ifndef MARIO_CLOCK_EFFECT_H
#define MARIO_CLOCK_EFFECT_H

#include "../Effect.h"
#include "../SpriteRenderer.h"
#include "assets.h"  // Contiene GROUND, HILL, BUSH, CLOUD1, CLOUD2, BLOCK, _MASK, ecc.
#include "Super_Mario_Bros__24pt7b.h"

// Forward declarations
struct MarioBlock;
struct MarioSprite;

enum MarioState {
    IDLE,
    WALKING,
    JUMPING
};

enum JumpTarget {
    NONE,
    HOUR_BLOCK,
    MINUTE_BLOCK,
    BOTH_BLOCKS
};

class MarioClockEffect : public Effect {
private:
    SpriteRenderer* spriteRenderer;
    
    // Strutture
    MarioBlock* hourBlock;
    MarioBlock* minuteBlock;
    MarioSprite* marioSprite;
    
    // State
    MarioState marioState;
    JumpTarget currentJumpTarget;
    int marioTargetX;
    bool marioFacingRight;
    bool walkingToJump;
    bool waitingForNextJump;
    unsigned long nextJumpDelay;
    bool needsRedraw;
    
    // Time tracking
    int fakeHour;
    int fakeMin;
    int fakeSec;
    unsigned long lastSecondUpdate;
    unsigned long lastMarioUpdate;
    
    // Constants
    static const int WALK_SPEED = 2;
    static const int JUMP_HEIGHT = 14;
    static const int MARIO_PACE = 3;
    static const int BLOCK_MAX_MOVE = 4;
    static const int BLOCK_MOVE_PACE = 2;
    
    // Metodi privati helper
    void initBlocks();
    void initMario();
    void updateTime();
    void updateMario();
    void updateMarioWalk();
    void updateBlock(MarioBlock& block, unsigned long& lastUpdate);
    
    void drawScene();
    void drawBackground();
    void drawGround();
    void drawBlock(MarioBlock& block);
    void redrawBackground(int x, int y, int width, int height);
    
    bool checkCollision(MarioSprite& mario, MarioBlock& block);
    void startJump();
    void marioJump(JumpTarget target);
    
public:
    MarioClockEffect(DisplayManager* dm);
    ~MarioClockEffect();
    
    void init() override;
    void update() override;
    void draw() override;
    const char* getName() override { return "Mario Clock"; }
    void reset() override;
};

// Strutture di supporto
struct MarioBlock {
    int x, y, firstY;
    int width, height;
    String text;
    bool isHit;
    int direction;
    int moveAmount;
};

struct MarioSprite {
    int x, y, firstY;
    int width, height;
    bool isJumping;
    int direction;
    int lastY;
    const unsigned short* sprite;
    bool collisionDetected;
};

#endif