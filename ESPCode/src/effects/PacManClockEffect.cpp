#include "PacManClockEffect.h"
#include <Fonts/Picopixel.h>

PacManClockEffect::PacManClockEffect(DisplayManager* dm, TimeManager* tm)
    : Effect(dm),
      timeManager(tm),
      pacDirection(PAC_RIGHT),
      pacState(PAC_MOVING),
      pacAnimFrame(false),
      pacColor(PACMAN_COLOR),
      invincibleTimeout(0),
      lastPacmanUpdate(0),
      lastClockUpdate(0),
      lastSecondBlink(0),
      showSeconds(true),
      needsRedraw(true),
      _callbackID(-1)
{
}

PacManClockEffect::~PacManClockEffect() {
}

void PacManClockEffect::init() {
    DEBUG_PRINTLN("[PacManClockEffect] Initializing");

    // Copia mappa iniziale
    resetMap();

    // Trova posizione iniziale di Pacman (blocco 7 nella mappa)
    pacStartX = 0;
    pacStartY = 0;
    for (int j = 0; j < MAP_SIZE; j++) {
        for (int i = 0; i < MAP_SIZE; i++) {
            uint8_t block = pgm_read_byte(&PACMAN_MAP_CONST[j][i]);
            if (block == BLOCK_PACMAN) {
                pacStartX = mapToPixel(i);
                pacStartY = mapToPixel(j);
                break;
            }
        }
    }

    pacX = pacStartX;
    pacY = pacStartY;
    pacDirection = PAC_RIGHT;
    pacState = PAC_MOVING;
    pacAnimFrame = false;
    pacColor = PACMAN_COLOR;

    // Copia sprite iniziale (direzione RIGHT)
    memcpy(currentSprite[0], PACMAN_SPRITE_1, sizeof(PACMAN_SPRITE_1));
    memcpy(currentSprite[1], PACMAN_SPRITE_2, sizeof(PACMAN_SPRITE_2));

    randomSeed(millis() + timeManager->getSecond());

    lastPacmanUpdate = millis();
    //lastClockUpdate = millis();
    //lastSecondBlink = millis();
    _callbackID = timeManager->addOnMinuteChange([this](int /*hour*/, int /*minute*/, int /*second*/){
        this->drawClock();
    });
    showSeconds = true;
    needsRedraw = true;

    DEBUG_PRINTF("[PacManClockEffect] Pacman start position: %d, %d\n", pacX, pacY);
}

void PacManClockEffect::cleanup() {
    timeManager->removeCallback(_callbackID);
    DEBUG_PRINTLN("[PacManClockEffect] Cleanup");
}

void PacManClockEffect::reset() {
    Effect::reset();
    needsRedraw = true;
}

void PacManClockEffect::resetMap() {
    // Copia mappa originale in mappa di gioco
    for (int j = 0; j < MAP_SIZE; j++) {
        for (int i = 0; i < MAP_SIZE; i++) {
            gameMap[j][i] = pgm_read_byte(&PACMAN_MAP_CONST[j][i]);
        }
    }
}

int PacManClockEffect::mapToPixel(int mapCoord) {
    return (mapCoord * BLOCK_SIZE) + MAP_BORDER_SIZE;
}

int PacManClockEffect::pixelToMap(int pixelCoord) {
    return (pixelCoord - MAP_BORDER_SIZE) / BLOCK_SIZE;
}

void PacManClockEffect::update() {
    unsigned long now = millis();

    // Aggiorna blink dei due punti ogni secondo
    if (now - lastSecondBlink >= 1000) {
        showSeconds = !showSeconds;
        drawColonBlink();
        lastSecondBlink = now;
    }

    // // Aggiorna orologio ogni minuto
    // if (now - lastClockUpdate >= 60000) {
    //     drawClock();
    //     lastClockUpdate = now;
    // }

    // Aggiorna Pacman ogni 75ms
    if (now - lastPacmanUpdate >= 75) {
        updatePacman();
        lastPacmanUpdate = now;
    }
}

void PacManClockEffect::draw() {
    if (needsRedraw) {
        drawMap();
        drawClock();
        needsRedraw = false;
    }
}

void PacManClockEffect::drawMap() {
    // Sfondo nero
    displayManager->fillScreen(0, 0, 0);

    // Bordo blu
    uint8_t r = (PACMAN_WALL_COLOR >> 11) << 3;
    uint8_t g = ((PACMAN_WALL_COLOR >> 5) & 0x3F) << 2;
    uint8_t b = (PACMAN_WALL_COLOR & 0x1F) << 3;

    // Rettangolo esterno
    for (int x = 0; x < 64; x++) {
        displayManager->drawPixel(x, 0, r, g, b);
        displayManager->drawPixel(x, 1, r, g, b);
        displayManager->drawPixel(x, 62, r, g, b);
        displayManager->drawPixel(x, 63, r, g, b);
    }
    for (int y = 0; y < 64; y++) {
        displayManager->drawPixel(0, y, r, g, b);
        displayManager->drawPixel(1, y, r, g, b);
        displayManager->drawPixel(62, y, r, g, b);
        displayManager->drawPixel(63, y, r, g, b);
    }

    // Disegna elementi mappa
    uint8_t fr, fg, fb;

    for (int j = 0; j < MAP_SIZE; j++) {
        for (int i = 0; i < MAP_SIZE; i++) {
            uint8_t block = gameMap[j][i];
            int px = mapToPixel(i);
            int py = mapToPixel(j);

            switch (block) {
                case BLOCK_FOOD:
                    // Cibo piccolo (3x1 pixel)
                    fr = (PACMAN_FOOD_COLOR >> 11) << 3;
                    fg = ((PACMAN_FOOD_COLOR >> 5) & 0x3F) << 2;
                    fb = (PACMAN_FOOD_COLOR & 0x1F) << 3;
                    displayManager->drawPixel(px + 1, py + 2, fr, fg, fb);
                    displayManager->drawPixel(px + 2, py + 2, fr, fg, fb);
                    displayManager->drawPixel(px + 3, py + 2, fr, fg, fb);
                    break;

                case BLOCK_WALL:
                case BLOCK_CLOCK:
                    // Muro (5x5 pixel blu)
                    fr = (PACMAN_WALL_COLOR >> 11) << 3;
                    fg = ((PACMAN_WALL_COLOR >> 5) & 0x3F) << 2;
                    fb = (PACMAN_WALL_COLOR & 0x1F) << 3;
                    for (int dy = 0; dy < BLOCK_SIZE; dy++) {
                        for (int dx = 0; dx < BLOCK_SIZE; dx++) {
                            displayManager->drawPixel(px + dx, py + dy, fr, fg, fb);
                        }
                    }
                    break;

                case BLOCK_GATE:
                    // Gate (come cibo)
                    fr = (PACMAN_FOOD_COLOR >> 11) << 3;
                    fg = ((PACMAN_FOOD_COLOR >> 5) & 0x3F) << 2;
                    fb = (PACMAN_FOOD_COLOR & 0x1F) << 3;
                    displayManager->drawPixel(px + 1, py + 2, fr, fg, fb);
                    displayManager->drawPixel(px + 2, py + 2, fr, fg, fb);
                    displayManager->drawPixel(px + 3, py + 2, fr, fg, fb);
                    break;

                case BLOCK_SUPER_FOOD:
                    // Super cibo (3x3 pixel arancione)
                    fr = (PACMAN_SUPER_COLOR >> 11) << 3;
                    fg = ((PACMAN_SUPER_COLOR >> 5) & 0x3F) << 2;
                    fb = (PACMAN_SUPER_COLOR & 0x1F) << 3;
                    for (int dy = 1; dy < 4; dy++) {
                        for (int dx = 1; dx < 4; dx++) {
                            displayManager->drawPixel(px + dx, py + dy, fr, fg, fb);
                        }
                    }
                    break;

                case BLOCK_PACMAN:
                    // Posizione iniziale pacman - disegna pacman qui
                    drawPacman();
                    break;
            }
        }
    }
}

void PacManClockEffect::drawClock() {
    // Pulisci area orologio (centro della mappa, blocchi CLOCK)
    // fillRect manuale
    for (int y = 19; y < 19 + 26; y++) {
        for (int x = 14; x < 14 + 36; x++) {
            displayManager->drawPixel(x, y, 0, 0, 0);
        }
    }

    // Data in piccolo (Picopixel font)
    displayManager->setFont(&Picopixel);
    displayManager->setTextSize(1);

    uint8_t tr = (PACMAN_TEXT_COLOR >> 11) << 3;
    uint8_t tg = ((PACMAN_TEXT_COLOR >> 5) & 0x3F) << 2;
    uint8_t tb = (PACMAN_TEXT_COLOR & 0x1F) << 3;
    displayManager->setTextColor(displayManager->color565(tr, tg, tb));

    // Nomi giorni e mesi
    const char* weekDays[] = {"DOM", "LUN", "MAR", "MER", "GIO", "VEN", "SAB"};
    const char* months[] = {"GEN", "FEB", "MAR", "APR", "MAG", "GIU",
                            "LUG", "AGO", "SET", "OTT", "NOV", "DIC"};

    int month = timeManager->getMonth();
    int day = timeManager->getDay();
    int weekday = timeManager->getWeekday();

    int16_t cursorX = 14;
    int16_t cursorY = 41;

    // Stampa mese
    displayManager->setCursor(cursorX, cursorY);
    if (month >= 1 && month <= 12) {
        displayManager->print(months[month - 1]);
        int16_t x1, y1;
        uint16_t w, h;
        displayManager->getDisplay()->getTextBounds(months[month - 1], cursorX, cursorY, &x1, &y1, &w, &h);
        cursorX += w + 2;  // 2px di spazio
    }

    // Stampa giorno
    displayManager->setCursor(cursorX, cursorY);
    char dayStr[3];
    sprintf(dayStr, "%d", day);
    displayManager->print(dayStr);
    int16_t x1, y1;
    uint16_t w, h;
    displayManager->getDisplay()->getTextBounds(dayStr, cursorX, cursorY, &x1, &y1, &w, &h);
    cursorX += w + 2;  // 2px di spazio

    // Stampa giorno settimana
    displayManager->setCursor(cursorX, cursorY);
    if (weekday >= 0 && weekday <= 6) {
        displayManager->print(weekDays[weekday]);
    }

    // Ora in grande (pacmanHourFont)
    displayManager->setFont(&pacmanHourFont);

    uint8_t hr = (PACMAN_COLOR >> 11) << 3;
    uint8_t hg = ((PACMAN_COLOR >> 5) & 0x3F) << 2;
    uint8_t hb = (PACMAN_COLOR & 0x1F) << 3;
    displayManager->setTextColor(displayManager->color565(hr, hg, hb));

    displayManager->setCursor(15, 28);

    char timeStr[6];
    sprintf(timeStr, "%02d %02d", timeManager->getHour(), timeManager->getMinute());
    displayManager->print(timeStr);
}

void PacManClockEffect::drawColonBlink() {
    uint8_t cr, cg, cb;

    if (showSeconds) {
        cr = (PACMAN_COLON_COLOR >> 11) << 3;
        cg = ((PACMAN_COLON_COLOR >> 5) & 0x3F) << 2;
        cb = (PACMAN_COLON_COLOR & 0x1F) << 3;
    } else {
        cr = 0; cg = 0; cb = 0;
    }

    // Due punti dell'orologio
    displayManager->drawPixel(31, 24, cr, cg, cb);
    displayManager->drawPixel(32, 24, cr, cg, cb);
    displayManager->drawPixel(31, 25, cr, cg, cb);
    displayManager->drawPixel(32, 25, cr, cg, cb);

    displayManager->drawPixel(31, 29, cr, cg, cb);
    displayManager->drawPixel(32, 29, cr, cg, cb);
    displayManager->drawPixel(31, 30, cr, cg, cb);
    displayManager->drawPixel(32, 30, cr, cg, cb);
}

void PacManClockEffect::drawPacman() {
    // Seleziona frame animazione
    const uint16_t* sprite = pacAnimFrame ? currentSprite[1] : currentSprite[0];

    for (int dy = 0; dy < PACMAN_SPRITE_SIZE; dy++) {
        for (int dx = 0; dx < PACMAN_SPRITE_SIZE; dx++) {
            uint16_t color = sprite[dy * PACMAN_SPRITE_SIZE + dx];
            if (color != 0x0000) {  // Non trasparente
                uint8_t r = (color >> 11) << 3;
                uint8_t g = ((color >> 5) & 0x3F) << 2;
                uint8_t b = (color & 0x1F) << 3;
                displayManager->drawPixel(pacX + dx, pacY + dy, r, g, b);
            }
        }
    }
}

void PacManClockEffect::updatePacman() {
    // Controlla se Pacman e' allineato al centro di un blocco
    bool fullBlock = false;

    if ((pacDirection == PAC_LEFT || pacDirection == PAC_RIGHT) &&
        ((pacX - MAP_BORDER_SIZE) % BLOCK_SIZE == 0)) {
        fullBlock = true;
    }
    if ((pacDirection == PAC_UP || pacDirection == PAC_DOWN) &&
        ((pacY - MAP_BORDER_SIZE) % BLOCK_SIZE == 0)) {
        fullBlock = true;
    }

    if (fullBlock) {
        // Coordinate mappa correnti
        int mapR = pixelToMap(pacY);
        int mapC = pixelToMap(pacX);

        // Prendi contenuto blocco corrente PRIMA di mangiarlo
        MapBlock currentBlock = static_cast<MapBlock>(gameMap[mapR][mapC]);

        // Mangia il cibo (svuota il blocco)
        gameMap[mapR][mapC] = BLOCK_EMPTY;

        // Se super cibo, diventa invincibile
        if (currentBlock == BLOCK_SUPER_FOOD) {
            pacState = PAC_INVINCIBLE;
            invincibleTimeout = millis();
        }

        // Decidi prossima direzione con BFS
        directionDecision();

        // Controlla se tutto il cibo e' finito
        if (countBlocks(BLOCK_FOOD) == 0 && countBlocks(BLOCK_SUPER_FOOD) == 0) {
            resetMap();
            drawMap();
            drawClock();
        }
    }

    // Muovi Pacman
    if (pacState == PAC_MOVING || pacState == PAC_INVINCIBLE) {
        // Cancella vecchia posizione (fillRect manuale)
        for (int dy = 0; dy < PACMAN_SPRITE_SIZE; dy++) {
            for (int dx = 0; dx < PACMAN_SPRITE_SIZE; dx++) {
                displayManager->drawPixel(pacX + dx, pacY + dy, 0, 0, 0);
            }
        }

        movePacman();
    }

    // Alterna frame animazione ogni 3 aggiornamenti
    static int animCounter = 0;
    animCounter++;
    if (animCounter % 3 == 0) {
        pacAnimFrame = !pacAnimFrame;
    }

    // Gestione stato invincibile
    if (pacState == PAC_INVINCIBLE) {
        if (animCounter % 2 == 0) {
            pacColor = random(0xFFFF);
        } else {
            pacColor = PACMAN_COLOR;
        }
        changePacmanColor(pacColor);

        // Timeout invincibilita' (7 secondi)
        if (millis() - invincibleTimeout >= 7000) {
            pacState = PAC_MOVING;
            pacColor = PACMAN_COLOR;
            changePacmanColor(pacColor);
        }
    }

    // Disegna Pacman nella nuova posizione
    drawPacman();
}

void PacManClockEffect::movePacman() {
    switch (pacDirection) {
        case PAC_RIGHT: pacX += 1; break;
        case PAC_LEFT:  pacX -= 1; break;
        case PAC_DOWN:  pacY += 1; break;
        case PAC_UP:    pacY -= 1; break;
    }
}

void PacManClockEffect::turnPacman(PacDirection dir) {
    // Resetta sprite alla direzione RIGHT
    memcpy(currentSprite[0], PACMAN_SPRITE_1, sizeof(PACMAN_SPRITE_1));
    memcpy(currentSprite[1], PACMAN_SPRITE_2, sizeof(PACMAN_SPRITE_2));

    // Ruota/flippa per la nuova direzione
    switch (dir) {
        case PAC_LEFT:
            flipSprite();
            break;
        case PAC_DOWN:
            flipSprite();
            rotateSprite();
            flipSprite();
            break;
        case PAC_UP:
            rotateSprite();
            break;
        case PAC_RIGHT:
        default:
            // Gia' nella direzione giusta
            break;
    }

    changePacmanColor(pacColor);
    pacDirection = dir;
}

void PacManClockEffect::flipSprite() {
    int n = PACMAN_SPRITE_SIZE;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n / 2 + 1; j++) {
            std::swap(currentSprite[0][j + (i * n)], currentSprite[0][(i * n) + n - j - 1]);
            std::swap(currentSprite[1][j + (i * n)], currentSprite[1][(i * n) + n - j - 1]);
        }
    }
}

void PacManClockEffect::rotateSprite() {
    int n = PACMAN_SPRITE_SIZE;

    uint16_t temp0[25], temp1[25];
    memcpy(temp0, currentSprite[0], sizeof(temp0));
    memcpy(temp1, currentSprite[1], sizeof(temp1));

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            currentSprite[0][(i * n) + j] = temp0[(n - 1 - i) + (j * n)];
            currentSprite[1][(i * n) + j] = temp1[(n - 1 - i) + (j * n)];
        }
    }
}

void PacManClockEffect::changePacmanColor(uint16_t newColor) {
    for (int i = 0; i < PACMAN_SPRITE_SIZE * PACMAN_SPRITE_SIZE; i++) {
        if (currentSprite[0][i] != 0x0000) {
            currentSprite[0][i] = newColor;
        }
        if (currentSprite[1][i] != 0x0000) {
            currentSprite[1][i] = newColor;
        }
    }
}

MapBlock PacManClockEffect::getNextBlock() {
    return getNextBlock(pacDirection);
}

MapBlock PacManClockEffect::getNextBlock(PacDirection dir) {
    int mapR = pixelToMap(pacY);
    int mapC = pixelToMap(pacX);

    switch (dir) {
        case PAC_RIGHT:
            if (pacX + PACMAN_SPRITE_SIZE < MAP_MAX_POS) {
                return static_cast<MapBlock>(gameMap[mapR][mapC + 1]);
            }
            break;
        case PAC_DOWN:
            if (pacY + PACMAN_SPRITE_SIZE < MAP_MAX_POS) {
                return static_cast<MapBlock>(gameMap[mapR + 1][mapC]);
            }
            break;
        case PAC_LEFT:
            if ((pacX - MAP_BORDER_SIZE) > 0) {
                return static_cast<MapBlock>(gameMap[mapR][mapC - 1]);
            }
            break;
        case PAC_UP:
            if ((pacY - MAP_BORDER_SIZE) > 0) {
                return static_cast<MapBlock>(gameMap[mapR - 1][mapC]);
            }
            break;
    }

    return BLOCK_OUT_OF_MAP;
}

bool PacManClockEffect::contains(int value, const int* values) {
    for (int i = 1; i < values[0] + 1; i++) {
        if (value == values[i]) {
            return true;
        }
    }
    return false;
}

int PacManClockEffect::countBlocks(MapBlock type) {
    int count = 0;
    for (int j = 0; j < MAP_SIZE; j++) {
        for (int i = 0; i < MAP_SIZE; i++) {
            if (gameMap[j][i] == type) {
                count++;
            }
        }
    }
    return count;
}

// ========== BFS PATHFINDING ==========

bool PacManClockEffect::isValidCell(int r, int c) {
    if (r < 0 || r >= MAP_SIZE || c < 0 || c >= MAP_SIZE) {
        return false;
    }

    MapBlock block = static_cast<MapBlock>(gameMap[r][c]);
    return contains(block, movingBlocks) || block == BLOCK_SUPER_FOOD;
}

bool PacManClockEffect::isTargetCell(int r, int c) {
    if (r < 0 || r >= MAP_SIZE || c < 0 || c >= MAP_SIZE) {
        return false;
    }

    MapBlock block = static_cast<MapBlock>(gameMap[r][c]);
    return block == BLOCK_FOOD || block == BLOCK_SUPER_FOOD;
}

void PacManClockEffect::reconstructPath(PacPoint start, PacPoint end, PacDirection& nextMove) {
    PacPoint current = end;
    PacPoint prev = parent[current.x][current.y];

    // Risali il percorso fino al primo passo dopo start
    while (!(prev.x == start.x && prev.y == start.y)) {
        current = prev;
        prev = parent[current.x][current.y];

        // Safety break
        if (current.x == -1 || current.y == -1) return;
    }

    // Determina direzione da start a current
    if (current.x > start.x) nextMove = PAC_DOWN;
    else if (current.x < start.x) nextMove = PAC_UP;
    else if (current.y > start.y) nextMove = PAC_RIGHT;
    else if (current.y < start.y) nextMove = PAC_LEFT;
}

bool PacManClockEffect::findShortestPath(int startR, int startC, PacDirection& nextMove) {
    // Inizializza BFS
    queueFront = 0;
    queueRear = -1;

    for (int i = 0; i < MAP_SIZE; i++) {
        for (int j = 0; j < MAP_SIZE; j++) {
            visited[i][j] = false;
            parent[i][j] = {-1, -1};
        }
    }

    // Punto di partenza
    PacPoint startPoint = {startR, startC};
    visited[startR][startC] = true;
    bfsQueue[++queueRear] = startPoint;

    // Direzioni: UP, DOWN, LEFT, RIGHT
    int dRow[] = {-1, 1, 0, 0};
    int dCol[] = {0, 0, -1, 1};

    while (queueFront <= queueRear) {
        PacPoint current = bfsQueue[queueFront++];

        // Trovato target?
        if (isTargetCell(current.x, current.y)) {
            reconstructPath(startPoint, current, nextMove);
            return true;
        }

        // Esplora vicini
        for (int i = 0; i < 4; i++) {
            int nextR = current.x + dRow[i];
            int nextC = current.y + dCol[i];

            if (isValidCell(nextR, nextC) && !visited[nextR][nextC]) {
                visited[nextR][nextC] = true;
                parent[nextR][nextC] = current;
                bfsQueue[++queueRear] = {nextR, nextC};

                // Previeni overflow
                if (queueRear >= MAX_QUEUE_SIZE - 1) {
                    DEBUG_PRINTLN("[PacMan] BFS Queue Overflow!");
                    return false;
                }
            }
        }
    }

    return false;  // Nessun percorso trovato
}

void PacManClockEffect::directionDecision() {
    int mapR = pixelToMap(pacY);
    int mapC = pixelToMap(pacX);
    PacDirection nextMove = pacDirection;

    if (findShortestPath(mapR, mapC, nextMove)) {
        // Percorso trovato, gira Pacman
        if (nextMove != pacDirection) {
            turnPacman(nextMove);
        }
    } else {
        // Nessun percorso, fallback a movimento casuale
        MapBlock nextBlk = getNextBlock();
        if (contains(nextBlk, blockingBlocks)) {
            turnRandom();
        }
    }
}

void PacManClockEffect::turnRandom() {
    int dir = random(4);

    do {
        turnPacman(static_cast<PacDirection>(dir));
        dir = random(4);
    } while (!contains(getNextBlock(), movingBlocks));

    DEBUG_PRINTF("[PacMan] New random direction: %d\n", pacDirection);
}
