#ifndef MARIO_CLOCK_EFFECT_H
#define MARIO_CLOCK_EFFECT_H

#include "../Effect.h"
#include "../SpriteRenderer.h"
#include "../TimeManager.h"
#include "assets.h"  // Contiene GROUND, HILL, BUSH, CLOUD1, CLOUD2, BLOCK, _MASK, ecc.
#include "Super_Mario_Bros__24pt7b.h"


// ========== FEATURE FLAGS ==========
#define ENABLE_PIPE_ANIMATION 1  // Metti 0 per disabilitare il tubo
// ===================================

// Forward declarations
struct MarioBlock;
struct MarioSprite;
struct MarioPipe;

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

// Stati del tubo
enum PipeState {
    PIPE_HIDDEN,
    PIPE_RISING,
    PIPE_VISIBLE,
    PIPE_LOWERING
};

class MarioClockEffect : public Effect {
private:
    SpriteRenderer* spriteRenderer;
    TimeManager* timeManager;
    
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
    
    // Ora corrente (cache locale)
    int lastHour;
    int lastMinute;
    int minuteCallbackId;
    int hourCallbackId;
    
    // Timer
    unsigned long lastMarioUpdate;
    
    // ========== PIPE ANIMATION ==========
#if ENABLE_PIPE_ANIMATION
    MarioPipe* pipe;
    unsigned long lastPipeUpdate;
    unsigned long pipeVisibleUntil;
    
    void initPipe();
    void updatePipe();
    void drawPipe();
    void triggerPipe();  // Chiama questa per far apparire il tubo
#endif
    // ====================================
    
    // Constants
    static const int WALK_SPEED = 2;
    static const int JUMP_HEIGHT = 14;
    static const int MARIO_PACE = 3;
    static const int BLOCK_MAX_MOVE = 4;
    static const int BLOCK_MOVE_PACE = 2;
    
    // Pipe constants
    static const int PIPE_RISE_SPEED = 1;
    static const int PIPE_VISIBLE_TIME = 20000;  // ms che resta visibile
    
    // Metodi privati helper
    void initBlocks();
    void initMario();
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
    MarioClockEffect(DisplayManager* dm, TimeManager* tm);
    ~MarioClockEffect();
    
    void init() override;
    void update() override;
    void draw() override;
    void cleanup() override;
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

// Struttura tubo
struct MarioPipe {
    int x;              // Posizione X
    int y;              // Posizione Y corrente (top del tubo)
    int targetY;        // Posizione finale quando completamente visibile
    int hiddenY;        // Posizione quando nascosto (sotto il pavimento)
    int width;
    int height;
    PipeState state;
};

#endif