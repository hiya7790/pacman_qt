#include "mainwindow.h"
#include "gameserver.h"
#include "soundmanager.h"
#include <QPainter>
#include <QRandomGenerator>
#include <QJsonObject>
#include <QJsonArray>
#include <QGuiApplication>
#include <QClipboard>
#include <cmath>
#include <algorithm>

// Definitions for static constexpr members ensuring ODR linkage across all compiler versions
constexpr int MainWindow::LEVELS_PER_WORLD;
constexpr int MainWindow::ROWS;
constexpr int MainWindow::COLS;
constexpr int MainWindow::MAX_SECTORS_X;
constexpr int MainWindow::MAX_SECTORS_Y;
constexpr double MainWindow::DEFAULT_VISION_RADIUS;
constexpr int MainWindow::FOG_PEEK_DURATION_MS;
constexpr int MainWindow::INITIAL_FOG_PEEKS;
constexpr int MainWindow::TRANSITION_DURATION_MS;
constexpr int MainWindow::FRIGHTEN_DURATION;
constexpr int MainWindow::MAX_FRUITS;

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(700, 750);
    setStyleSheet("background-color: black;");
    setWindowTitle("Pacman: Labyrinth Saga");
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setFocus();

    currentScreen = SCREEN_HOME_MAP;
    loadGameProgress();
    selectedMapLevel = highestUnlockedLevel;
    viewedMapWorld = (selectedMapLevel - 1) / LEVELS_PER_WORLD + 1;
    hoveredMapLevel = 0;
    currentLevel = highestUnlockedLevel;

    mapScrollY = 0.0f;
    targetMapScrollY = 0.0f;
    isDraggingMap = false;
    dragStartY = 0;
    dragStartScrollY = 0.0f;

    currentVisionRadius = DEFAULT_VISION_RADIUS;
    currentFrightenDuration = FRIGHTEN_DURATION;
    fogOfWarEnabled = true;
    fogPeekActive = false;
    fogPeeksRemaining = INITIAL_FOG_PEEKS;
    p1FogPeeks = INITIAL_FOG_PEEKS;
    p2FogPeeks = INITIAL_FOG_PEEKS;
    hero3FogPeeks = INITIAL_FOG_PEEKS;
    hero4FogPeeks = INITIAL_FOG_PEEKS;
    peekToastMessage.clear();
    isTransitioning = false;
    newSectorJustDiscovered = false;

    isMultiplayer = false;
    singlePlayerHero = 1; // 1 = Yellow Pacman, 2 = Pink Ms. Pacman
    arrowPlayer = 1;      // Arrow keys control Player 1 (Yellow) by default
    scoreP1 = 0;
    scoreP2 = 0;
    p1Alive = true;
    p2Alive = true;
    p1RespawnTicks = 0;
    p2RespawnTicks = 0;
    p1InvincibleTicks = 0;
    p2InvincibleTicks = 0;
    p1Lives = INITIAL_LIVES;
    p2Lives = INITIAL_LIVES;
    p1ReachedExit = false;
    p2ReachedExit = false;
    p2X = 9;
    p2Y = 19;
    p2Dir = STOP;
    p2NextDir = RIGHT;
    p2MouthAngle = 45;
    p2MouthOpening = false;

    // Multiplayer Server & Extra Heroes (Mode A: 4P Co-op, Mode B: Ghost Hunters)
    hostRole = "p1";
    showMultiplayerLobby = false;
    linkCopiedFeedback = false;
    p1ClientId = -1;
    p1PlayerName = "";
    p2ClientId = -1;
    p2PlayerName = "";
    hero3.clientId = -1;
    hero3.playerName = "";
    hero3.alive = false;
    hero3.score = 0;
    hero4.clientId = -1;
    hero4.playerName = "";
    hero4.alive = false;
    hero4.score = 0;

    gameServer = new GameServer(this);
    connect(gameServer, &GameServer::playerJoined, this, &MainWindow::onPlayerJoined);
    connect(gameServer, &GameServer::playerLeft, this, &MainWindow::onPlayerLeft);
    connect(gameServer, &GameServer::playerMoved, this, &MainWindow::onPlayerMoved);
    connect(gameServer, &GameServer::playerTriggeredPeek, this, &MainWindow::onPlayerPeek);
    gameServer->startServer(8080);

    initGame();
    scrollToLevel(highestUnlockedLevel, true);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::gameLoop);
    timer->start(130);

    showFullScreen();
}

MainWindow::~MainWindow()
{
    saveGameProgress();
    if (gameServer) {
        gameServer->stopServer();
    }
}

void MainWindow::initGame()
{
    score = 0;
    scoreP1 = 0;
    scoreP2 = 0;
    hero3.score = 0;
    hero4.score = 0;
    moveCount = 0;
    gameOver = false;
    levelComplete = false;
    p1Alive = true;
    p2Alive = true;
    p1RespawnTicks = 0;
    p2RespawnTicks = 0;
    hero3.respawnTicks = 0;
    hero4.respawnTicks = 0;
    p1Lives = INITIAL_LIVES;
    p2Lives = INITIAL_LIVES;
    hero3.lives = INITIAL_LIVES;
    hero4.lives = INITIAL_LIVES;
    p1ReachedExit = false;
    p2ReachedExit = false;
    hero3.reachedExit = false;
    hero4.reachedExit = false;
    currentScreen = SCREEN_HOME_MAP;
}

void MainWindow::startLevel(int lvl)
{
    currentLevel = std::max(1, lvl);
    selectedMapLevel = currentLevel;
    viewedMapWorld = (selectedMapLevel - 1) / LEVELS_PER_WORLD + 1;
    currentScreen = SCREEN_PLAYING;
    gameOver = false;
    levelComplete = false;
    if (!isAnyMultiplayer()) {
        hostRole = (singlePlayerHero == 2 ? "p2" : "p1");
    }
    initLevel();
    setFocus();
    activateWindow();
    broadcastCurrentGameState();
    update();
}

void MainWindow::initLevel()
{
    // Determine dynamic sector grid dimensions for this level
    getSectorDimsForLevel(currentLevel, sectorCols, sectorRows);

    // Dynamic difficulty scaling across 20 stages per world
    currentVisionRadius = std::max(2.6, 3.8 - (currentLevel - 1) * 0.05);
    currentFrightenDuration = std::max(12, 28 - (currentLevel / 3) * 2);
    ghostMoveEveryN = (currentLevel >= 7) ? 1 : 2;
    ghostTickCounter = 0;

    int baseInterval = std::max(85, 130 - (currentLevel / 5) * 5);
    if (timer) {
        timer->setInterval(baseInterval);
    }

    // 1. Generate every sector
    for (int sy = 0; sy < sectorRows; ++sy) {
        for (int sx = 0; sx < sectorCols; ++sx) {
            generateSectorMaze(sx, sy);
            sectorDiscovered[sy][sx] = false;
        }
    }

    // 2. Carve seamless inter-window connecting doorways with guaranteed clear corridors
    for (int sy = 0; sy < sectorRows; ++sy) {
        for (int sx = 0; sx < sectorCols; ++sx) {
            // East-West doorway connection at rows 9 and 10
            if (sx < sectorCols - 1) {
                for (int c = 9; c < COLS; ++c) {
                    board[sy][sx][9][c] = 0;
                    board[sy][sx][10][c] = 0;
                    if (c > 11 && c < COLS - 1) {
                        board[sy][sx][9][c] = 2;
                        board[sy][sx][10][c] = 2;
                    }
                }
                for (int c = 0; c <= 11; ++c) {
                    board[sy][sx+1][9][c] = 0;
                    board[sy][sx+1][10][c] = 0;
                    if (c > 0 && c < 9) {
                        board[sy][sx+1][9][c] = 2;
                        board[sy][sx+1][10][c] = 2;
                    }
                }
            }
            // North-South doorway connection at cols 9 and 10
            if (sy < sectorRows - 1) {
                for (int r = 9; r < ROWS; ++r) {
                    board[sy][sx][r][9] = 0;
                    board[sy][sx][r][10] = 0;
                    if (r > 11 && r < ROWS - 1) {
                        board[sy][sx][r][9] = 2;
                        board[sy][sx][r][10] = 2;
                    }
                }
                for (int r = 0; r <= 11; ++r) {
                    board[sy+1][sx][r][9] = 0;
                    board[sy+1][sx][r][10] = 0;
                    if (r > 0 && r < 9) {
                        board[sy+1][sx][r][9] = 2;
                        board[sy+1][sx][r][10] = 2;
                    }
                }
            }
        }
    }

    // 3. Set Start Sector and Exit Sector
    currentSectorX = 0;
    currentSectorY = std::clamp(sectorRows - 1, 0, MAX_SECTORS_Y - 1);
    sectorDiscovered[currentSectorY][currentSectorX] = true;

    exitSectorX = std::clamp(sectorCols - 1, 0, MAX_SECTORS_X - 1);
    exitSectorY = 0;
    exitX = 1;
    exitY = 1;
    board[exitSectorY][exitSectorX][exitY][exitX] = 0;

    // 4. Place Pacman & Heroes in the start sector
    pacmanX = 10;
    pacmanY = ROWS - 2;
    for (int r = 11; r <= ROWS - 2; ++r) {
        board[currentSectorY][currentSectorX][r][9] = 0;
        board[currentSectorY][currentSectorX][r][10] = 0;
        if (r > 11 && r < ROWS - 2) {
            board[currentSectorY][currentSectorX][r][9] = 2;
            board[currentSectorY][currentSectorX][r][10] = 2;
        }
    }
    board[currentSectorY][currentSectorX][pacmanY][pacmanX] = 0;
    board[currentSectorY][currentSectorX][pacmanY][pacmanX - 1] = 0;
    board[currentSectorY][currentSectorX][pacmanY][pacmanX + 1] = 0;

    pacmanDir = LEFT;
    pacmanNextDir = LEFT;
    pacmanMouthAngle = 45;
    pacmanMouthOpening = false;
    p1Alive = isP1Active();
    p1RespawnTicks = 0;
    p1InvincibleTicks = 16; // 2s spawn grace shield on stage start
    p1Lives = INITIAL_LIVES;
    p1ReachedExit = false;

    // Player 2 (Ms. Pac-Man)
    p2X = 9;
    p2Y = ROWS - 2;
    p2Dir = RIGHT;
    p2NextDir = RIGHT;
    p2MouthAngle = 45;
    p2MouthOpening = false;
    p2Alive = isP2Active();
    p2RespawnTicks = 0;
    p2InvincibleTicks = 16;
    p2Lives = INITIAL_LIVES;
    p2ReachedExit = false;

    // Extra Heroes (Jr. Pac-Man & Miracle Pac-Man)
    hero3.x = 9;
    hero3.y = ROWS - 3;
    hero3.dir = RIGHT;
    hero3.nextDir = RIGHT;
    hero3.mouthAngle = 45;
    hero3.mouthOpening = false;
    hero3.alive = isP3Active();
    hero3.respawnTicks = 0;
    hero3.invincibleTicks = 16;
    hero3.lives = INITIAL_LIVES;
    hero3.reachedExit = false;

    hero4.x = 10;
    hero4.y = ROWS - 3;
    hero4.dir = LEFT;
    hero4.nextDir = LEFT;
    hero4.mouthAngle = 45;
    hero4.mouthOpening = false;
    hero4.alive = isP4Active();
    hero4.respawnTicks = 0;
    hero4.invincibleTicks = 16;
    hero4.lives = INITIAL_LIVES;
    hero4.reachedExit = false;

    board[currentSectorY][currentSectorX][ROWS - 3][9] = 0;
    board[currentSectorY][currentSectorX][ROWS - 3][10] = 0;

    levelComplete = false;
    gameOver = false;
    isTransitioning = false;
    newSectorJustDiscovered = false;

    // 5. Spawn Ghosts across sectors dynamically based on level
    ghosts.clear();

    // Helper to find a safe, walkable spawn tile in a given sector
    auto findValidGhostSpawn = [&](int sx, int sy, int &outX, int &outY) {
        static const int preferredPos[][2] = {
            {10, 9}, {9, 10}, {10, 10}, {9, 9}, {11, 10}, {10, 11}, {8, 10}, {10, 8}
        };
        for (const auto &pos : preferredPos) {
            if (pos[1] >= 0 && pos[1] < ROWS && pos[0] >= 0 && pos[0] < COLS) {
                if (board[sy][sx][pos[1]][pos[0]] == 0) {
                    outX = pos[0]; outY = pos[1];
                    return;
                }
            }
        }
        for (int r = 1; r < ROWS - 1; ++r) {
            for (int c = 1; c < COLS - 1; ++c) {
                if (board[sy][sx][r][c] == 0) {
                    outX = c; outY = r;
                    return;
                }
            }
        }
        outX = 10; outY = 10;
    };

    // --- The Core 4 Classic Ghosts (Blinky, Pinky, Inky, Clyde) ---
    // Ghost 0: Blinky (Red) - Direct Hunter in Start Sector
    int bX = 10, bY = 9;
    findValidGhostSpawn(currentSectorX, currentSectorY, bX, bY);
    ghosts.push_back({currentSectorX, currentSectorY, bX, bY, UP, 0, -1, ""});

    // Ghost 1: Pinky (Pink) - Ambusher in Start Sector
    int pX = 9, pY = 10;
    findValidGhostSpawn(currentSectorX, currentSectorY, pX, pY);
    ghosts.push_back({currentSectorX, currentSectorY, pX, pY, LEFT, 1, -1, ""});

    // Ghost 2: Inky (Cyan) - Patrols Exit Sector (or Start if single sector)
    int inkySx = exitSectorX, inkySy = exitSectorY;
    int iX = 10, iY = 10;
    findValidGhostSpawn(inkySx, inkySy, iX, iY);
    ghosts.push_back({inkySx, inkySy, iX, iY, RIGHT, 2, -1, ""});

    // Ghost 3: Clyde (Orange) - Roams Exit Sector or adjacent sector
    int clydeSx = (exitSectorX != currentSectorX || exitSectorY != currentSectorY) ? exitSectorX : currentSectorX;
    int clydeSy = (exitSectorX != currentSectorX || exitSectorY != currentSectorY) ? exitSectorY : currentSectorY;
    int cX = 9, cY = 9;
    findValidGhostSpawn(clydeSx, clydeSy, cX, cY);
    ghosts.push_back({clydeSx, clydeSy, cX, cY, DOWN, 3, -1, ""});

    // --- Dynamic Target Ghost Count (Increased for beginner levels & scales with labyrinth size) ---
    // Beginner levels:
    // Level 1 (2 sectors): 4 ghosts (2 in start, 2 in exit)
    // Level 2 (4 sectors): 6 ghosts (1-2 per sector)
    // Level 3 (6 sectors): 8 ghosts
    // Level 4 (6 sectors): 8 ghosts
    // Level 5 (9 sectors): 11 ghosts
    // Higher levels: scales gracefully up to 22 ghosts across large multi-window labyrinths
    int baseGhosts = 4 + (currentLevel - 1) * 3 / 2;
    int sectorMin = sectorCols * sectorRows + (sectorCols * sectorRows > 2 ? 2 : 0);
    int targetGhostCount = std::clamp(std::max(baseGhosts, sectorMin), 4, 22);

    int colorCycle = 0;

    // Pass 1: Ensure every sector has at least 1 ghost guarding it
    for (int sy = 0; sy < sectorRows && (int)ghosts.size() < targetGhostCount; ++sy) {
        for (int sx = 0; sx < sectorCols && (int)ghosts.size() < targetGhostCount; ++sx) {
            int ghostsInSector = 0;
            for (const Ghost &g : ghosts) {
                if (g.sectorX == sx && g.sectorY == sy) ghostsInSector++;
            }
            if (ghostsInSector < 1) {
                int gx = 10, gy = 10;
                findValidGhostSpawn(sx, sy, gx, gy);
                Direction initDir = static_cast<Direction>((colorCycle) % 4);
                ghosts.push_back({sx, sy, gx, gy, initDir, colorCycle % 4, -1, ""});
                colorCycle++;
            }
        }
    }

    // Pass 2: Deterministically distribute remaining ghosts across sectors
    quint32 ghostSeed = static_cast<quint32>(currentLevel) * 9184711u + 54321u;
    QRandomGenerator ghostRng(ghostSeed);

    while ((int)ghosts.size() < targetGhostCount) {
        int randSx = ghostRng.bounded(sectorCols);
        int randSy = ghostRng.bounded(sectorRows);
        int gx = 10, gy = 10;
        findValidGhostSpawn(randSx, randSy, gx, gy);
        Direction initDir = static_cast<Direction>((colorCycle) % 4);
        ghosts.push_back({randSx, randSy, gx, gy, initDir, colorCycle % 4, -1, ""});
        colorCycle++;
    }

    // Re-link human controlled ghosts if clients are already joined
    if (gameServer) {
        for (ConnectedClient *c : gameServer->getActiveClients()) {
            if (c->role == "blinky" && ghosts.size() > 0) {
                ghosts[0].clientId = c->id; ghosts[0].playerName = c->playerName;
            } else if (c->role == "pinky" && ghosts.size() > 1) {
                ghosts[1].clientId = c->id; ghosts[1].playerName = c->playerName;
            } else if (c->role == "inky" && ghosts.size() > 2) {
                ghosts[2].clientId = c->id; ghosts[2].playerName = c->playerName;
            } else if (c->role == "clyde" && ghosts.size() > 3) {
                ghosts[3].clientId = c->id; ghosts[3].playerName = c->playerName;
            }
        }
    }

    ghostsFrightened = false;
    frightenedTicksLeft = 0;

    activeFruits.clear();
    fruitSpawnCounter = 20 + (currentLevel % 15);

    fogOfWarEnabled = true;
    fogPeekActive = false;
    fogPeeksRemaining = INITIAL_FOG_PEEKS;
    p1FogPeeks = INITIAL_FOG_PEEKS;
    p2FogPeeks = INITIAL_FOG_PEEKS;
    hero3FogPeeks = INITIAL_FOG_PEEKS;
    hero4FogPeeks = INITIAL_FOG_PEEKS;
    peekToastMessage.clear();

    updateFogOfWar();
    elapsedTimer.start();
}

// ========================================================
// Game Loop
// ========================================================

void MainWindow::gameLoop()
{
    if (currentScreen == SCREEN_HOME_MAP) {
        if (!isDraggingMap) {
            float diff = targetMapScrollY - mapScrollY;
            if (std::abs(diff) > 0.4f) {
                mapScrollY += diff * 0.25f;
            } else {
                mapScrollY = targetMapScrollY;
            }
        }
        static int mapBrdCount = 0;
        if (++mapBrdCount % 10 == 0) {
            broadcastCurrentGameState();
        }
        update();
        return;
    }

    if (gameOver || levelComplete) {
        static int endBrdCount = 0;
        if (++endBrdCount % 10 == 0) {
            broadcastCurrentGameState();
        }
        update();
        return;
    }

    // Check 1.0-second fog peek timer
    if (fogPeekActive) {
        if (fogPeekTimer.elapsed() >= FOG_PEEK_DURATION_MS) {
            fogOfWarEnabled = true;
            fogPeekActive = false;
            broadcastCurrentGameState();
            update();
        }
    }

    // End transition if time elapsed
    if (isTransitioning) {
        if (transitionTimer.elapsed() >= TRANSITION_DURATION_MS) {
            isTransitioning = false;
        }
    }

    // Heroes respawn timers (only respawn if hero has lives remaining)
    if (!p1Alive && p1RespawnTicks > 0) {
        p1RespawnTicks--;
        if (p1RespawnTicks == 0) {
            if (p1Lives > 0) {
                p1Alive = true;
                p1InvincibleTicks = 25; // ~3.3s spawn protection shield
                QPoint safePos = findSafeHeroSpawnPos(currentSectorX, currentSectorY);
                pacmanX = safePos.x();
                pacmanY = safePos.y();
                pacmanDir = STOP;
                pacmanNextDir = STOP;

                // Push away any ghosts in this sector within 5.0 tiles of spawn point
                for (Ghost &g : ghosts) {
                    if (g.sectorX == currentSectorX && g.sectorY == currentSectorY) {
                        if (std::hypot(g.x - pacmanX, g.y - pacmanY) < 5.0) {
                            respawnGhostFarAway(g);
                        }
                    }
                }

                updateFogOfWar();
                QString heroName = (!isAnyMultiplayer() && singlePlayerHero == 2) ? "Ms. Pac-Man" : "Pac-Man";
                peekToastMessage = QString("🛡️ %1 RESPAWNED! (Spawn Shield Active)").arg(heroName);
                peekToastTimer.start();
                SoundManager::instance().playFruit();
            }
        }
    }
    if (isP2Active() && !p2Alive && p2RespawnTicks > 0) {
        p2RespawnTicks--;
        if (p2RespawnTicks == 0) {
            if (p2Lives > 0) {
                p2Alive = true;
                p2InvincibleTicks = 25;
                QPoint safePos = findSafeHeroSpawnPos(currentSectorX, currentSectorY);
                p2X = safePos.x();
                p2Y = safePos.y();
                p2Dir = STOP;
                p2NextDir = STOP;

                for (Ghost &g : ghosts) {
                    if (g.sectorX == currentSectorX && g.sectorY == currentSectorY) {
                        if (std::hypot(g.x - p2X, g.y - p2Y) < 5.0) {
                            respawnGhostFarAway(g);
                        }
                    }
                }

                updateFogOfWar();
                peekToastMessage = "🛡️ Ms. Pac-Man RESPAWNED! (Spawn Shield Active)";
                peekToastTimer.start();
                SoundManager::instance().playFruit();
            }
        }
    }
    if (isP3Active() && !hero3.alive && hero3.respawnTicks > 0) {
        hero3.respawnTicks--;
        if (hero3.respawnTicks == 0) {
            if (hero3.lives > 0) {
                hero3.alive = true;
                hero3.invincibleTicks = 25;
                QPoint safePos = findSafeHeroSpawnPos(currentSectorX, currentSectorY);
                hero3.x = safePos.x();
                hero3.y = safePos.y();
                hero3.dir = STOP;
                hero3.nextDir = STOP;

                for (Ghost &g : ghosts) {
                    if (g.sectorX == currentSectorX && g.sectorY == currentSectorY) {
                        if (std::hypot(g.x - hero3.x, g.y - hero3.y) < 5.0) {
                            respawnGhostFarAway(g);
                        }
                    }
                }

                updateFogOfWar();
                peekToastMessage = "🛡️ Jr. Pac-Man RESPAWNED! (Spawn Shield Active)";
                peekToastTimer.start();
                SoundManager::instance().playFruit();
            }
        }
    }
    if (isP4Active() && !hero4.alive && hero4.respawnTicks > 0) {
        hero4.respawnTicks--;
        if (hero4.respawnTicks == 0) {
            if (hero4.lives > 0) {
                hero4.alive = true;
                hero4.invincibleTicks = 25;
                QPoint safePos = findSafeHeroSpawnPos(currentSectorX, currentSectorY);
                hero4.x = safePos.x();
                hero4.y = safePos.y();
                hero4.dir = STOP;
                hero4.nextDir = STOP;

                for (Ghost &g : ghosts) {
                    if (g.sectorX == currentSectorX && g.sectorY == currentSectorY) {
                        if (std::hypot(g.x - hero4.x, g.y - hero4.y) < 5.0) {
                            respawnGhostFarAway(g);
                        }
                    }
                }

                updateFogOfWar();
                peekToastMessage = "🛡️ Miracle Pac-Man RESPAWNED! (Spawn Shield Active)";
                peekToastTimer.start();
                SoundManager::instance().playFruit();
            }
        }
    }

    // Decrement spawn protection shield ticks
    if (p1InvincibleTicks > 0) p1InvincibleTicks--;
    if (p2InvincibleTicks > 0) p2InvincibleTicks--;
    if (hero3.invincibleTicks > 0) hero3.invincibleTicks--;
    if (hero4.invincibleTicks > 0) hero4.invincibleTicks--;

    ghostTickCounter++;
    if (ghostTickCounter >= ghostMoveEveryN) {
        ghostTickCounter = 0;
        moveGhosts();
        checkGhostCollisions();
    }

    if (ghostsFrightened) {
        frightenedTicksLeft--;
        if (frightenedTicksLeft <= 0) {
            ghostsFrightened = false;
        }
    }

    trySpawnFruit();

    for (int i = activeFruits.size() - 1; i >= 0; --i) {
        activeFruits[i].ticksRemaining--;
        if (activeFruits[i].ticksRemaining <= 0) {
            activeFruits.removeAt(i);
        }
    }

    broadcastCurrentGameState();
    update();
}

void MainWindow::broadcastCurrentGameState()
{
    if (!gameServer || !gameServer->isRunning()) return;

    QJsonObject state;
    state["type"] = "gameState";
    state["screen"] = (currentScreen == SCREEN_HOME_MAP ? "map" : "playing");
    state["level"] = currentLevel;
    state["world"] = (currentLevel - 1) / LEVELS_PER_WORLD + 1;
    state["stage"] = (currentLevel - 1) % LEVELS_PER_WORLD + 1;
    state["score"] = score;
    state["frightened"] = ghostsFrightened;
    state["frightenedTicks"] = frightenedTicksLeft;
    state["fogPeeks"] = p1FogPeeks;
    QJsonObject peeks;
    peeks["p1"] = p1FogPeeks;
    peeks["p2"] = p2FogPeeks;
    peeks["p3"] = hero3FogPeeks;
    peeks["p4"] = hero4FogPeeks;
    state["peeks"] = peeks;
    state["fog"] = fogOfWarEnabled;
    state["peek"] = fogPeekActive;
    state["radius"] = currentVisionRadius;
    state["gameOver"] = gameOver;
    state["levelComplete"] = levelComplete;
    state["secX"] = currentSectorX;
    state["secY"] = currentSectorY;
    state["hostRole"] = hostRole;

    if (currentScreen == SCREEN_PLAYING) {
        QString boardStr;
        boardStr.reserve(ROWS * COLS);
        QString expStr;
        expStr.reserve(ROWS * COLS);
        for (int r = 0; r < ROWS; ++r) {
            for (int c = 0; c < COLS; ++c) {
                boardStr.append(QString::number(board[currentSectorY][currentSectorX][r][c]));
                expStr.append(explored[currentSectorY][currentSectorX][r][c] ? '1' : '0');
            }
        }
        state["board"] = boardStr;
        state["explored"] = expStr;

        // Heroes
        QJsonObject h1;
        h1["x"] = pacmanX; h1["y"] = pacmanY; h1["dir"] = (int)pacmanDir;
        h1["mouth"] = pacmanMouthAngle; h1["alive"] = (isP1Active() && p1Alive); h1["score"] = scoreP1; h1["active"] = isP1Active();
        h1["name"] = (hostRole == "p1" ? "Host" : p1PlayerName);
        h1["respawnTicks"] = p1RespawnTicks;
        h1["invincible"] = (p1InvincibleTicks > 0);
        h1["lives"] = p1Lives;
        h1["reachedExit"] = p1ReachedExit;
        state["p1"] = h1;

        QJsonObject h2;
        h2["x"] = p2X; h2["y"] = p2Y; h2["dir"] = (int)p2Dir;
        h2["mouth"] = p2MouthAngle; h2["alive"] = (isP2Active() && p2Alive); h2["score"] = scoreP2;
        h2["name"] = (hostRole == "p2" ? "Host" : p2PlayerName); h2["active"] = isP2Active();
        h2["respawnTicks"] = p2RespawnTicks;
        h2["invincible"] = (p2InvincibleTicks > 0);
        h2["lives"] = p2Lives;
        h2["reachedExit"] = p2ReachedExit;
        state["p2"] = h2;

        QJsonObject h3;
        h3["x"] = hero3.x; h3["y"] = hero3.y; h3["dir"] = (int)hero3.dir;
        h3["mouth"] = hero3.mouthAngle; h3["alive"] = (isP3Active() && hero3.alive); h3["score"] = hero3.score;
        h3["name"] = (hostRole == "p3" ? "Host" : hero3.playerName); h3["active"] = isP3Active();
        h3["respawnTicks"] = hero3.respawnTicks;
        h3["invincible"] = (hero3.invincibleTicks > 0);
        h3["lives"] = hero3.lives;
        h3["reachedExit"] = hero3.reachedExit;
        state["p3"] = h3;

        QJsonObject h4;
        h4["x"] = hero4.x; h4["y"] = hero4.y; h4["dir"] = (int)hero4.dir;
        h4["mouth"] = hero4.mouthAngle; h4["alive"] = (isP4Active() && hero4.alive); h4["score"] = hero4.score;
        h4["name"] = (hostRole == "p4" ? "Host" : hero4.playerName); h4["active"] = isP4Active();
        h4["respawnTicks"] = hero4.respawnTicks;
        h4["invincible"] = (hero4.invincibleTicks > 0);
        h4["lives"] = hero4.lives;
        h4["reachedExit"] = hero4.reachedExit;
        state["p4"] = h4;

        QJsonObject livesObj;
        livesObj["p1"] = p1Lives;
        livesObj["p2"] = p2Lives;
        livesObj["p3"] = hero3.lives;
        livesObj["p4"] = hero4.lives;
        state["lives"] = livesObj;

        state["exitEligible"] = getEligibleHeroCount();
        state["exitReached"] = getReachedExitHeroCount();

        // Exit portal
        state["hasExit"] = (currentSectorX == exitSectorX && currentSectorY == exitSectorY);
        state["exitX"] = exitX;
        state["exitY"] = exitY;

        // Ghosts in current sector
        QJsonArray gArr;
        for (int i = 0; i < ghosts.size(); ++i) {
            const Ghost &g = ghosts[i];
            if (g.sectorX == currentSectorX && g.sectorY == currentSectorY) {
                QJsonObject gObj;
                gObj["x"] = g.x;
                gObj["y"] = g.y;
                gObj["dir"] = (int)g.dir;
                gObj["color"] = g.colorIndex;
                bool isHost = (getHostGhostIndex() == i);
                gObj["name"] = isHost ? "Host" : g.playerName;
                gObj["isHuman"] = (g.clientId >= 0 || isHost);
                gArr.append(gObj);
            }
        }
        state["ghosts"] = gArr;

        // Active fruits in current sector
        QJsonArray fArr;
        for (const BonusFruit &f : activeFruits) {
            if (f.sectorX == currentSectorX && f.sectorY == currentSectorY) {
                QJsonObject fObj;
                fObj["x"] = f.x;
                fObj["y"] = f.y;
                fObj["type"] = (int)f.type;
                fArr.append(fObj);
            }
        }
        state["fruits"] = fArr;
    }

    gameServer->broadcastGameState(state);
}

void MainWindow::onPlayerJoined(int clientId, const QString &role, const QString &name)
{
    if (role == "p1") {
        p1ClientId = clientId;
        p1PlayerName = name;
        p1Alive = true;
        p1RespawnTicks = 0;
        p1Lives = INITIAL_LIVES;
        p1ReachedExit = false;
        pacmanX = 10;
        pacmanY = ROWS - 2;
        peekToastMessage = QString("🟡 %1 joined as Pac-Man!").arg(name);
        peekToastTimer.start();
    } else if (role == "p2") {
        p2ClientId = clientId;
        p2PlayerName = name;
        p2Alive = true;
        p2RespawnTicks = 0;
        p2Lives = INITIAL_LIVES;
        p2ReachedExit = false;
        p2X = 9;
        p2Y = ROWS - 2;
        peekToastMessage = QString("🎀 %1 joined as Ms. Pac-Man!").arg(name);
        peekToastTimer.start();
    } else if (role == "p3") {
        hero3.clientId = clientId;
        hero3.playerName = name;
        hero3.alive = true;
        hero3.respawnTicks = 0;
        hero3.lives = INITIAL_LIVES;
        hero3.reachedExit = false;
        hero3.x = 9;
        hero3.y = ROWS - 3;
        peekToastMessage = QString("🧢 %1 joined as Jr. Pac-Man!").arg(name);
        peekToastTimer.start();
    } else if (role == "p4") {
        hero4.clientId = clientId;
        hero4.playerName = name;
        hero4.alive = true;
        hero4.respawnTicks = 0;
        hero4.lives = INITIAL_LIVES;
        hero4.reachedExit = false;
        hero4.x = 10;
        hero4.y = ROWS - 3;
        peekToastMessage = QString("🟢 %1 joined as Miracle Pac-Man!").arg(name);
        peekToastTimer.start();
    } else if (role == "blinky") {
        if (!ghosts.isEmpty()) {
            ghosts[0].clientId = clientId;
            ghosts[0].playerName = name;
        }
        peekToastMessage = QString("🔴 %1 joined as Blinky!").arg(name);
        peekToastTimer.start();
    } else if (role == "pinky") {
        if (ghosts.size() > 1) {
            ghosts[1].clientId = clientId;
            ghosts[1].playerName = name;
        }
        peekToastMessage = QString("🌸 %1 joined as Pinky!").arg(name);
        peekToastTimer.start();
    } else if (role == "inky") {
        if (ghosts.size() > 2) {
            ghosts[2].clientId = clientId;
            ghosts[2].playerName = name;
        }
        peekToastMessage = QString("🩵 %1 joined as Inky!").arg(name);
        peekToastTimer.start();
    } else if (role == "clyde") {
        if (ghosts.size() > 3) {
            ghosts[3].clientId = clientId;
            ghosts[3].playerName = name;
        }
        peekToastMessage = QString("🟠 %1 joined as Clyde!").arg(name);
        peekToastTimer.start();
    }
    updateFogOfWar();
    broadcastCurrentGameState();
    update();
}

void MainWindow::onPlayerLeft(int clientId, const QString &role, const QString &name)
{
    if (role == "p1" && p1ClientId == clientId) {
        p1ClientId = -1;
        p1PlayerName.clear();
        p1Alive = isP1Active();
        peekToastMessage = QString("👋 %1 left (Pac-Man)").arg(name);
        peekToastTimer.start();
    } else if (role == "p2" && p2ClientId == clientId) {
        p2ClientId = -1;
        p2PlayerName.clear();
        p2Alive = isMultiplayer; // only keep alive if host is in local co-op
        peekToastMessage = QString("👋 %1 left (Ms. Pac-Man)").arg(name);
        peekToastTimer.start();
    } else if (role == "p3" && hero3.clientId == clientId) {
        hero3.clientId = -1;
        hero3.playerName.clear();
        hero3.alive = false;
        peekToastMessage = QString("👋 %1 left (Jr. Pac-Man)").arg(name);
        peekToastTimer.start();
    } else if (role == "p4" && hero4.clientId == clientId) {
        hero4.clientId = -1;
        hero4.playerName.clear();
        hero4.alive = false;
        peekToastMessage = QString("👋 %1 left (Miracle Pac-Man)").arg(name);
        peekToastTimer.start();
    } else {
        for (Ghost &g : ghosts) {
            if (g.clientId == clientId) {
                g.clientId = -1;
                g.playerName.clear();
                peekToastMessage = QString("👋 %1 left (Ghost returned to AI)").arg(name);
                peekToastTimer.start();
                break;
            }
        }
    }
    broadcastCurrentGameState();
    update();
}

void MainWindow::onPlayerMoved(int clientId, const QString &role, int dirInt)
{
    Direction d = STOP;
    if (dirInt == 0) d = UP;
    else if (dirInt == 1) d = DOWN;
    else if (dirInt == 2) d = LEFT;
    else if (dirInt == 3) d = RIGHT;

    if (role == "p1" && p1ClientId == clientId) {
        pacmanNextDir = d;
        if (p1Alive) {
            if (canMove(pacmanX, pacmanY, d)) {
                pacmanDir = d;
                movePacman();
                checkFruitPickup();
                checkGhostCollisions();
                checkLevelExit();
                updateFogOfWar();
            } else {
                pacmanDir = d;
            }
            broadcastCurrentGameState();
            update();
        }
    } else if (role == "p2" && p2ClientId == clientId) {
        p2NextDir = d;
        if (p2Alive) {
            if (canMove(p2X, p2Y, d)) {
                p2Dir = d;
                movePlayer2();
                checkFruitPickup();
                checkGhostCollisions();
                checkLevelExit();
                updateFogOfWar();
            } else {
                p2Dir = d;
            }
            broadcastCurrentGameState();
            update();
        }
    } else if (role == "p3" && hero3.clientId == clientId) {
        hero3.nextDir = d;
        if (hero3.alive) {
            if (canMove(hero3.x, hero3.y, d)) {
                hero3.dir = d;
                moveHero(hero3, 3);
                checkFruitPickup();
                checkGhostCollisions();
                checkLevelExit();
                updateFogOfWar();
            } else {
                hero3.dir = d;
            }
            broadcastCurrentGameState();
            update();
        }
    } else if (role == "p4" && hero4.clientId == clientId) {
        hero4.nextDir = d;
        if (hero4.alive) {
            if (canMove(hero4.x, hero4.y, d)) {
                hero4.dir = d;
                moveHero(hero4, 4);
                checkFruitPickup();
                checkGhostCollisions();
                checkLevelExit();
                updateFogOfWar();
            } else {
                hero4.dir = d;
            }
            broadcastCurrentGameState();
            update();
        }
    } else if (role == "blinky" && !ghosts.isEmpty() && ghosts[0].clientId == clientId) {
        ghosts[0].dir = d;
        int nx = ghosts[0].x, ny = ghosts[0].y;
        if (d == UP) ny--; else if (d == DOWN) ny++; else if (d == LEFT) nx--; else if (d == RIGHT) nx++;
        if (nx >= 1 && nx < COLS - 1 && ny >= 1 && ny < ROWS - 1 && board[ghosts[0].sectorY][ghosts[0].sectorX][ny][nx] != 1) {
            ghosts[0].x = nx; ghosts[0].y = ny;
            checkGhostCollisions();
            broadcastCurrentGameState();
            update();
        }
    } else if (role == "pinky" && ghosts.size() > 1 && ghosts[1].clientId == clientId) {
        ghosts[1].dir = d;
        int nx = ghosts[1].x, ny = ghosts[1].y;
        if (d == UP) ny--; else if (d == DOWN) ny++; else if (d == LEFT) nx--; else if (d == RIGHT) nx++;
        if (nx >= 1 && nx < COLS - 1 && ny >= 1 && ny < ROWS - 1 && board[ghosts[1].sectorY][ghosts[1].sectorX][ny][nx] != 1) {
            ghosts[1].x = nx; ghosts[1].y = ny;
            checkGhostCollisions();
            broadcastCurrentGameState();
            update();
        }
    } else if (role == "inky" && ghosts.size() > 2 && ghosts[2].clientId == clientId) {
        ghosts[2].dir = d;
        int nx = ghosts[2].x, ny = ghosts[2].y;
        if (d == UP) ny--; else if (d == DOWN) ny++; else if (d == LEFT) nx--; else if (d == RIGHT) nx++;
        if (nx >= 1 && nx < COLS - 1 && ny >= 1 && ny < ROWS - 1 && board[ghosts[2].sectorY][ghosts[2].sectorX][ny][nx] != 1) {
            ghosts[2].x = nx; ghosts[2].y = ny;
            checkGhostCollisions();
            broadcastCurrentGameState();
            update();
        }
    } else if (role == "clyde" && ghosts.size() > 3 && ghosts[3].clientId == clientId) {
        ghosts[3].dir = d;
        int nx = ghosts[3].x, ny = ghosts[3].y;
        if (d == UP) ny--; else if (d == DOWN) ny++; else if (d == LEFT) nx--; else if (d == RIGHT) nx++;
        if (nx >= 1 && nx < COLS - 1 && ny >= 1 && ny < ROWS - 1 && board[ghosts[3].sectorY][ghosts[3].sectorX][ny][nx] != 1) {
            ghosts[3].x = nx; ghosts[3].y = ny;
            checkGhostCollisions();
            broadcastCurrentGameState();
            update();
        }
    }
}

void MainWindow::onPlayerPeek(int clientId, const QString &role)
{
    if (fogPeekActive) return;

    if (role == "p1" && p1ClientId == clientId) {
        if (p1FogPeeks > 0) {
            p1FogPeeks--;
            fogOfWarEnabled = false;
            fogPeekActive = true;
            fogPeekTimer.start();
            QString name = p1PlayerName.isEmpty() ? "Pac-Man" : p1PlayerName;
            peekToastMessage = QString("👁️ %1 ACTIVATED FOG PEEK! (%2 left for them)").arg(name).arg(p1FogPeeks);
            peekToastTimer.start();
            broadcastCurrentGameState();
            update();
        }
    } else if (role == "p2" && p2ClientId == clientId) {
        if (p2FogPeeks > 0) {
            p2FogPeeks--;
            fogOfWarEnabled = false;
            fogPeekActive = true;
            fogPeekTimer.start();
            QString name = p2PlayerName.isEmpty() ? "Ms. Pac-Man" : p2PlayerName;
            peekToastMessage = QString("👁️ %1 ACTIVATED FOG PEEK! (%2 left for them)").arg(name).arg(p2FogPeeks);
            peekToastTimer.start();
            broadcastCurrentGameState();
            update();
        } else {
            peekToastMessage = QString("⚠️ %1 HAS NO FOG PEEKS LEFT! Eat fruit 🍒 to earn more!").arg(p2PlayerName.isEmpty() ? "Ms. Pac-Man" : p2PlayerName);
            peekToastTimer.start();
            update();
        }
    } else if (role == "p3" && hero3.clientId == clientId) {
        if (hero3FogPeeks > 0) {
            hero3FogPeeks--;
            fogOfWarEnabled = false;
            fogPeekActive = true;
            fogPeekTimer.start();
            QString name = hero3.playerName.isEmpty() ? "Jr. Pac-Man" : hero3.playerName;
            peekToastMessage = QString("👁️ %1 ACTIVATED FOG PEEK! (%2 left for them)").arg(name).arg(hero3FogPeeks);
            peekToastTimer.start();
            broadcastCurrentGameState();
            update();
        }
    } else if (role == "p4" && hero4.clientId == clientId) {
        if (hero4FogPeeks > 0) {
            hero4FogPeeks--;
            fogOfWarEnabled = false;
            fogPeekActive = true;
            fogPeekTimer.start();
            QString name = hero4.playerName.isEmpty() ? "Miracle Pac-Man" : hero4.playerName;
            peekToastMessage = QString("👁️ %1 ACTIVATED FOG PEEK! (%2 left for them)").arg(name).arg(hero4FogPeeks);
            peekToastTimer.start();
            broadcastCurrentGameState();
            update();
        }
    }
}

void MainWindow::copyRoomLinkToClipboard()
{
    QString roomUrl = gameServer ? gameServer->getServerUrl() : "http://127.0.0.1:8080";
    QGuiApplication::clipboard()->setText(roomUrl);
    linkCopiedFeedback = true;
    linkCopiedTimer.start();
    peekToastMessage = QString("📋 ROOM LINK COPIED! Paste to friends: %1").arg(roomUrl);
    peekToastTimer.start();
    update();
}

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (currentScreen == SCREEN_HOME_MAP) {
        drawCandyCrushMap(painter);
    } else {
        drawGameScreen(painter);
    }
}

void MainWindow::switchHostRole(const QString &newRole)
{
    // Check if role is taken by an online client
    if (gameServer) {
        for (ConnectedClient *c : gameServer->getActiveClients()) {
            if (c->role == newRole) {
                peekToastMessage = QString("⚠️ %1 is already played by %2!").arg(getRoleDisplayName(newRole)).arg(c->playerName);
                peekToastTimer.start();
                update();
                return;
            }
        }
    }

    hostRole = newRole;

    if (hostRole == "p1") {
        p1Alive = true;
        p1RespawnTicks = 0;
        if (p1Lives <= 0) p1Lives = INITIAL_LIVES;
    } else if (hostRole == "p2") {
        p2Alive = true;
        p2RespawnTicks = 0;
        if (p2Lives <= 0) p2Lives = INITIAL_LIVES;
    } else if (hostRole == "p3") {
        hero3.alive = true;
        hero3.respawnTicks = 0;
        if (hero3.lives <= 0) hero3.lives = INITIAL_LIVES;
    } else if (hostRole == "p4") {
        hero4.alive = true;
        hero4.respawnTicks = 0;
        if (hero4.lives <= 0) hero4.lives = INITIAL_LIVES;
    }

    if (gameServer) {
        gameServer->setHostRole(hostRole);
    }

    peekToastMessage = QString("🎮 HOST PLAYING AS: %1 %2").arg(getRoleIcon(hostRole)).arg(getRoleDisplayName(hostRole));
    peekToastTimer.start();
    updateFogOfWar();
    broadcastCurrentGameState();
    update();
}

void MainWindow::cycleNextHostRole()
{
    QStringList allRoles = {"p1", "p2", "p3", "p4", "blinky", "pinky", "inky", "clyde"};
    int curIdx = allRoles.indexOf(hostRole);
    if (curIdx == -1) curIdx = 0;

    for (int i = 1; i <= allRoles.size(); ++i) {
        QString candidate = allRoles[(curIdx + i) % allRoles.size()];
        bool taken = false;
        if (gameServer) {
            for (ConnectedClient *c : gameServer->getActiveClients()) {
                if (c->role == candidate) { taken = true; break; }
            }
        }
        if (!taken) {
            switchHostRole(candidate);
            return;
        }
    }
}

bool MainWindow::isOnlineMultiplayer() const
{
    return (p1ClientId >= 0) || (p2ClientId >= 0) || (hero3.clientId != -1) || (hero4.clientId != -1) ||
           (gameServer && gameServer->getConnectedClientCount() > 0);
}

bool MainWindow::isP1Active() const
{
    if (isOnlineMultiplayer()) {
        return (hostRole == "p1") || (p1ClientId >= 0);
    }
    return (hostRole == "p1") || (p1ClientId >= 0) || (!isMultiplayer && singlePlayerHero == 1);
}

bool MainWindow::isP2Active() const
{
    if (isOnlineMultiplayer()) {
        return (hostRole == "p2") || (p2ClientId >= 0);
    }
    return (hostRole == "p2") || (p2ClientId >= 0) || isMultiplayer || (!isMultiplayer && singlePlayerHero == 2);
}

bool MainWindow::isP3Active() const
{
    return (hostRole == "p3") || (hero3.clientId != -1);
}

bool MainWindow::isP4Active() const
{
    return (hostRole == "p4") || (hero4.clientId != -1);
}

bool MainWindow::isAnyMultiplayer() const
{
    return isMultiplayer || isOnlineMultiplayer() || isHostGhost();
}

QString MainWindow::getRoleDisplayName(const QString &role) const
{
    if (role == "p1") return "Pac-Man";
    if (role == "p2") return "Ms. Pac-Man";
    if (role == "p3") return "Jr. Pac-Man";
    if (role == "p4") return "Miracle Pac-Man";
    if (role == "blinky") return "Blinky (Red Ghost)";
    if (role == "pinky") return "Pinky (Pink Ghost)";
    if (role == "inky") return "Inky (Cyan Ghost)";
    if (role == "clyde") return "Clyde (Orange Ghost)";
    return "Pac-Man";
}

QString MainWindow::getRoleIcon(const QString &role) const
{
    if (role == "p1") return "🟡";
    if (role == "p2") return "🎀";
    if (role == "p3") return "🧢";
    if (role == "p4") return "🟢";
    if (role == "blinky") return "🔴";
    if (role == "pinky") return "🌸";
    if (role == "inky") return "🩵";
    if (role == "clyde") return "🟠";
    return "🎮";
}

QRect MainWindow::getLobbyRoleRect(const QString &role) const
{
    int modalW = std::min(860, width() - 80);
    int modalX = (width() - modalW) / 2;
    int modalY = std::max(20, (height() - 560) / 2);
    int colW = (modalW - 70) / 2;
    int col1X = modalX + 25;
    int col2X = modalX + 45 + colW;
    int cardY = modalY + 138;

    if (role == "p1") return QRect(col1X + 15, cardY + 65, colW - 30, 52);
    if (role == "p2") return QRect(col1X + 15, cardY + 125, colW - 30, 52);
    if (role == "p3") return QRect(col1X + 15, cardY + 185, colW - 30, 52);
    if (role == "p4") return QRect(col1X + 15, cardY + 245, colW - 30, 52);
    if (role == "blinky") return QRect(col2X + 15, cardY + 65, colW - 30, 52);
    if (role == "pinky") return QRect(col2X + 15, cardY + 125, colW - 30, 52);
    if (role == "inky") return QRect(col2X + 15, cardY + 185, colW - 30, 52);
    if (role == "clyde") return QRect(col2X + 15, cardY + 245, colW - 30, 52);
    return QRect();
}

void MainWindow::moveHostGhost(int ghostIdx, Direction d)
{
    if (ghostIdx < 0 || ghostIdx >= ghosts.size()) return;
    Ghost &g = ghosts[ghostIdx];
    g.dir = d;
    int nx = g.x, ny = g.y;
    if (d == UP) ny--;
    else if (d == DOWN) ny++;
    else if (d == LEFT) nx--;
    else if (d == RIGHT) nx++;
    if (nx >= 0 && nx < COLS && ny >= 0 && ny < ROWS && board[g.sectorY][g.sectorX][ny][nx] != 1) {
        g.x = nx;
        g.y = ny;
        checkGhostCollisions();
        updateFogOfWar();
        broadcastCurrentGameState();
        update();
    }
}

void MainWindow::saveGameProgress()
{
    // 1. Native QSettings storage (macOS plist / Windows registry / Linux conf)
    QSettings settings("PacmanGame", "LabyrinthSaga");
    settings.setValue("highestUnlockedLevel", highestUnlockedLevel);
    settings.setValue("totalCareerScore", totalCareerScore);
    settings.setValue("singlePlayerHero", singlePlayerHero);

    settings.beginGroup("HighScores");
    for (auto it = levelHighScores.begin(); it != levelHighScores.end(); ++it) {
        settings.setValue(QString::number(it.key()), it.value());
    }
    settings.endGroup();

    settings.beginGroup("Stars");
    for (auto it = levelStars.begin(); it != levelStars.end(); ++it) {
        settings.setValue(QString::number(it.key()), it.value());
    }
    settings.endGroup();
    settings.sync();

    // 2. Portable JSON backup in AppDataLocation
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataDir);
    QString saveFilePath = appDataDir + "/savegame.json";

    QJsonObject root;
    root["highestUnlockedLevel"] = highestUnlockedLevel;
    root["totalCareerScore"] = totalCareerScore;
    root["singlePlayerHero"] = singlePlayerHero;

    QJsonObject scoresObj;
    for (auto it = levelHighScores.begin(); it != levelHighScores.end(); ++it) {
        scoresObj[QString::number(it.key())] = it.value();
    }
    root["highScores"] = scoresObj;

    QJsonObject starsObj;
    for (auto it = levelStars.begin(); it != levelStars.end(); ++it) {
        starsObj[QString::number(it.key())] = it.value();
    }
    root["stars"] = starsObj;

    QJsonDocument doc(root);
    QFile file(saveFilePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

void MainWindow::loadGameProgress()
{
    highestUnlockedLevel = 1;
    totalCareerScore = 0;
    singlePlayerHero = 1;
    levelHighScores.clear();
    levelStars.clear();

    bool loadedFromSettings = false;
    QSettings settings("PacmanGame", "LabyrinthSaga");
    if (settings.contains("highestUnlockedLevel")) {
        highestUnlockedLevel = settings.value("highestUnlockedLevel", 1).toInt();
        totalCareerScore = settings.value("totalCareerScore", 0).toInt();
        singlePlayerHero = settings.value("singlePlayerHero", 1).toInt();

        settings.beginGroup("HighScores");
        const QStringList scoreKeys = settings.childKeys();
        for (const QString &k : scoreKeys) {
            bool ok;
            int lvl = k.toInt(&ok);
            if (ok) {
                levelHighScores[lvl] = settings.value(k).toInt();
            }
        }
        settings.endGroup();

        settings.beginGroup("Stars");
        const QStringList starKeys = settings.childKeys();
        for (const QString &k : starKeys) {
            bool ok;
            int lvl = k.toInt(&ok);
            if (ok) {
                levelStars[lvl] = settings.value(k).toInt();
            }
        }
        settings.endGroup();
        loadedFromSettings = true;
    }

    if (!loadedFromSettings) {
        // Fallback: Read savegame.json
        QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QString saveFilePath = appDataDir + "/savegame.json";
        QFile file(saveFilePath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isObject()) {
                QJsonObject root = doc.object();
                highestUnlockedLevel = root.value("highestUnlockedLevel").toInt(1);
                totalCareerScore = root.value("totalCareerScore").toInt(0);
                singlePlayerHero = root.value("singlePlayerHero").toInt(1);

                QJsonObject scoresObj = root.value("highScores").toObject();
                for (auto it = scoresObj.begin(); it != scoresObj.end(); ++it) {
                    levelHighScores[it.key().toInt()] = it.value().toInt();
                }

                QJsonObject starsObj = root.value("stars").toObject();
                for (auto it = starsObj.begin(); it != starsObj.end(); ++it) {
                    levelStars[it.key().toInt()] = it.value().toInt();
                }
            }
        }
    }

    if (highestUnlockedLevel < 1) highestUnlockedLevel = 1;
    if (singlePlayerHero != 1 && singlePlayerHero != 2) singlePlayerHero = 1;
    arrowPlayer = singlePlayerHero;

    // Persist defaults on first launch
    if (!loadedFromSettings) {
        saveGameProgress();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveGameProgress();
    QWidget::closeEvent(event);
}

void MainWindow::resetGameProgress()
{
    highestUnlockedLevel = 1;
    selectedMapLevel = 1;
    viewedMapWorld = 1;
    totalCareerScore = 0;
    levelHighScores.clear();
    levelStars.clear();

    saveGameProgress();
    scrollToLevel(1, true);
    update();
}

int MainWindow::getLevelStars(int lvl) const
{
    if (lvl < highestUnlockedLevel) {
        return levelStars.value(lvl, 3); // Default 3 stars if completed
    }
    return levelStars.value(lvl, 0);
}

int MainWindow::getLevelHighScore(int lvl) const
{
    return levelHighScores.value(lvl, 0);
}
