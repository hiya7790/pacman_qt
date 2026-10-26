#include "mainwindow.h"
#include <QPainter>
#include <QRandomGenerator>
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

    currentScreen = SCREEN_HOME_MAP;
    highestUnlockedLevel = 1;
    selectedMapLevel = 1;
    viewedMapWorld = 1;
    hoveredMapLevel = 0;
    currentLevel = 1;

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
    peekToastMessage.clear();
    fogPeeksRemaining = INITIAL_FOG_PEEKS;
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
    p2X = 9;
    p2Y = 19;
    p2Dir = STOP;
    p2NextDir = RIGHT;
    p2MouthAngle = 45;
    p2MouthOpening = false;

    initGame();
    scrollToLevel(1, true);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::gameLoop);
    timer->start(130);

    showFullScreen();
}

MainWindow::~MainWindow()
{
}

void MainWindow::initGame()
{
    score = 0;
    scoreP1 = 0;
    scoreP2 = 0;
    moveCount = 0;
    gameOver = false;
    levelComplete = false;
    p1Alive = true;
    p2Alive = true;
    p1RespawnTicks = 0;
    p2RespawnTicks = 0;
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
    initLevel();
    update();
}

void MainWindow::initLevel()
{
    // Determine dynamic sector grid dimensions for this level
    getSectorDimsForLevel(currentLevel, sectorCols, sectorRows);

    // Dynamic difficulty scaling across 20 stages per world
    currentVisionRadius = std::max(2.6, 3.8 - (currentLevel - 1) * 0.05);
    currentFrightenDuration = std::max(12, 28 - (currentLevel / 3) * 2);
    ghostMoveEveryN = std::clamp(4 - currentLevel / 6, 1, 4);
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
                // Carve a clear 2-tile wide highway from center room (col 9) to East border in sx
                for (int c = 9; c < COLS; ++c) {
                    board[sy][sx][9][c] = 0;
                    board[sy][sx][10][c] = 0;
                    if (c > 11 && c < COLS - 1) {
                        board[sy][sx][9][c] = 2; // Breadcrumb dots
                        board[sy][sx][10][c] = 2;
                    }
                }
                // Carve a clear 2-tile wide highway from West border to center room (col 11) in sx+1
                for (int c = 0; c <= 11; ++c) {
                    board[sy][sx+1][9][c] = 0;
                    board[sy][sx+1][10][c] = 0;
                    if (c > 0 && c < 9) {
                        board[sy][sx+1][9][c] = 2; // Breadcrumb dots
                        board[sy][sx+1][10][c] = 2;
                    }
                }
            }
            // North-South doorway connection at cols 9 and 10
            if (sy < sectorRows - 1) {
                // Carve from center room (row 9) to South border in sy
                for (int r = 9; r < ROWS; ++r) {
                    board[sy][sx][r][9] = 0;
                    board[sy][sx][r][10] = 0;
                    if (r > 11 && r < ROWS - 1) {
                        board[sy][sx][r][9] = 2;
                        board[sy][sx][r][10] = 2;
                    }
                }
                // Carve from North border to center room (row 11) in sy+1
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
    // Start at bottom-left sector
    currentSectorX = 0;
    currentSectorY = std::clamp(sectorRows - 1, 0, MAX_SECTORS_Y - 1);
    sectorDiscovered[currentSectorY][currentSectorX] = true;

    // Exit at top-right sector (furthest from start!)
    exitSectorX = std::clamp(sectorCols - 1, 0, MAX_SECTORS_X - 1);
    exitSectorY = 0;
    exitX = 1;
    exitY = 1;
    board[exitSectorY][exitSectorX][exitY][exitX] = 0;

    // 4. Place Pacman in the start sector with guaranteed clear avenue up to center room
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
    p1Alive = true;
    p1RespawnTicks = 0;

    // Player 2 starts beside Player 1 in the start room
    p2X = 9;
    p2Y = ROWS - 2;
    p2Dir = RIGHT;
    p2NextDir = RIGHT;
    p2MouthAngle = 45;
    p2MouthOpening = false;
    p2Alive = true;
    p2RespawnTicks = 0;

    levelComplete = false;
    gameOver = false;
    isTransitioning = false;
    newSectorJustDiscovered = false;

    // 5. Spawn Ghosts across sectors dynamically based on level
    ghosts.clear();
    // Always 2 ghosts in starting window
    ghosts.push_back({currentSectorX, currentSectorY, 10, 9, UP, 0});     // Blinky (Red)
    ghosts.push_back({currentSectorX, currentSectorY, 9, 10, LEFT, 1});   // Pinky (Pink)

    // 1 ghost guarding the Exit window
    if (exitSectorX != currentSectorX || exitSectorY != currentSectorY) {
        ghosts.push_back({exitSectorX, exitSectorY, 10, 10, RIGHT, 2}); // Inky (Cyan)
    }

    // Ghost count scales progressively across stages (up to 12 ghosts!)
    int targetGhostCount = std::clamp(2 + currentLevel / 2, 2, 12);
    int colorCycle = 3; // Clyde (Orange = 3)

    // Spread roaming ghosts evenly across other sectors
    for (int sy = 0; sy < sectorRows && (int)ghosts.size() < targetGhostCount; ++sy) {
        for (int sx = 0; sx < sectorCols && (int)ghosts.size() < targetGhostCount; ++sx) {
            if (sx == currentSectorX && sy == currentSectorY) continue;
            if (sx == exitSectorX && sy == exitSectorY && currentLevel < 6) continue;

            Direction initDir = static_cast<Direction>(colorCycle % 4);
            ghosts.push_back({sx, sy, 10, 10, initDir, colorCycle % 4});
            colorCycle++;
        }
    }

    // If still have ghost slots, add sentries in sectors
    while ((int)ghosts.size() < targetGhostCount) {
        int randSx = QRandomGenerator::global()->bounded(sectorCols);
        int randSy = QRandomGenerator::global()->bounded(sectorRows);
        int gx = 9 + QRandomGenerator::global()->bounded(3);
        int gy = 9 + QRandomGenerator::global()->bounded(3);
        ghosts.push_back({randSx, randSy, gx, gy, UP, colorCycle % 4});
        colorCycle++;
    }

    ghostsFrightened = false;
    frightenedTicksLeft = 0;

    activeFruits.clear();
    fruitSpawnCounter = QRandomGenerator::global()->bounded(15, 35);

    fogOfWarEnabled = true;
    fogPeekActive = false;
    fogPeeksRemaining = INITIAL_FOG_PEEKS;
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
        update();
        return;
    }

    if (gameOver || levelComplete) {
        update();
        return;
    }

    // Check 1.5-second fog peek timer
    if (fogPeekActive) {
        if (fogPeekTimer.elapsed() >= FOG_PEEK_DURATION_MS) {
            fogOfWarEnabled = true;
            fogPeekActive = false;
        }
    }

    // End transition if time elapsed
    if (isTransitioning) {
        if (transitionTimer.elapsed() >= TRANSITION_DURATION_MS) {
            isTransitioning = false;
        }
    }

    // Multiplayer respawn timer
    if (isMultiplayer) {
        if (!p1Alive && p1RespawnTicks > 0) {
            p1RespawnTicks--;
            if (p1RespawnTicks == 0) {
                p1Alive = true;
                pacmanX = p2X;
                pacmanY = p2Y;
                updateFogOfWar();
            }
        }
        if (!p2Alive && p2RespawnTicks > 0) {
            p2RespawnTicks--;
            if (p2RespawnTicks == 0) {
                p2Alive = true;
                p2X = pacmanX;
                p2Y = pacmanY;
                updateFogOfWar();
            }
        }
    }

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
