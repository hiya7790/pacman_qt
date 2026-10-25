#include "mainwindow.h"
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QRandomGenerator>
#include <QPoint>
#include <cmath>
#include <algorithm>
#include <utility>

// Definitions for static constexpr members ensuring ODR linkage across all compiler versions
constexpr int MainWindow::TOTAL_LEVELS;
constexpr int MainWindow::ROWS;
constexpr int MainWindow::COLS;
constexpr int MainWindow::MAX_SECTORS_X;
constexpr int MainWindow::MAX_SECTORS_Y;
constexpr double MainWindow::VISION_RADIUS;
constexpr int MainWindow::FOG_PEEK_DURATION_MS;
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
    hoveredMapLevel = 0;
    currentLevel = 1;

    fogOfWarEnabled = true;
    fogPeekActive = false;
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
    currentLevel = std::max(1, std::min(TOTAL_LEVELS, lvl));
    selectedMapLevel = currentLevel;
    currentScreen = SCREEN_PLAYING;
    gameOver = false;
    levelComplete = false;
    initLevel();
    update();
}

void MainWindow::generateSectorMaze(int sx, int sy)
{
    // 1. Fill sector with walls
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            board[sy][sx][r][c] = 1;
            explored[sy][sx][r][c] = false;
        }
    }

    // 2. Randomized DFS carving on odd coordinates
    bool visited[ROWS][COLS] = {};
    QVector<QPoint> stack;

    visited[1][1] = true;
    board[sy][sx][1][1] = 0;
    stack.push_back(QPoint(1, 1));

    const int dr[] = {-2, 2, 0, 0};
    const int dc[] = {0, 0, -2, 2};

    while (!stack.isEmpty()) {
        QPoint curr = stack.last();
        QVector<int> neighbors;

        for (int i = 0; i < 4; ++i) {
            int nr = curr.y() + dr[i];
            int nc = curr.x() + dc[i];

            if (nr >= 1 && nr < ROWS - 1 && nc >= 1 && nc < COLS - 1) {
                if (!visited[nr][nc]) {
                    neighbors.push_back(i);
                }
            }
        }

        if (!neighbors.isEmpty()) {
            int pick = neighbors[QRandomGenerator::global()->bounded(neighbors.size())];
            int nr = curr.y() + dr[pick];
            int nc = curr.x() + dc[pick];

            int wallR = curr.y() + dr[pick] / 2;
            int wallC = curr.x() + dc[pick] / 2;
            board[sy][sx][wallR][wallC] = 0;

            board[sy][sx][nr][nc] = 0;
            visited[nr][nc] = true;
            stack.push_back(QPoint(nc, nr));
        } else {
            stack.pop_back();
        }
    }

    // 3. Central open area in each sector for smooth maneuvering
    for (int r = 9; r <= 11; ++r) {
        for (int c = 9; c <= 11; ++c) {
            board[sy][sx][r][c] = 0;
        }
    }
    board[sy][sx][8][10] = 0;
    board[sy][sx][12][10] = 0;

    // 4. Loop braiding: Remove ~28% of internal dividing walls
    for (int r = 2; r < ROWS - 2; ++r) {
        for (int c = 2; c < COLS - 2; ++c) {
            if (board[sy][sx][r][c] == 1) {
                if (board[sy][sx][r-1][c] == 0 && board[sy][sx][r+1][c] == 0 &&
                    board[sy][sx][r][c-1] == 1 && board[sy][sx][r][c+1] == 1) {
                    if (QRandomGenerator::global()->bounded(100) < 28) {
                        board[sy][sx][r][c] = 0;
                    }
                } else if (board[sy][sx][r][c-1] == 0 && board[sy][sx][r][c+1] == 0 &&
                           board[sy][sx][r-1][c] == 1 && board[sy][sx][r+1][c] == 1) {
                    if (QRandomGenerator::global()->bounded(100) < 28) {
                        board[sy][sx][r][c] = 0;
                    }
                }
            }
        }
    }

    // 5. Populate dots (2) on floor tiles
    for (int r = 1; r < ROWS - 1; ++r) {
        for (int c = 1; c < COLS - 1; ++c) {
            if (board[sy][sx][r][c] == 0) {
                // Keep center room clear of dots
                if (r >= 9 && r <= 11 && c >= 9 && c <= 11) continue;
                board[sy][sx][r][c] = 2; // Dot
            }
        }
    }
}

void MainWindow::initLevel()
{
    // Determine number of interconnected windows for this level
    if (currentLevel == 1) {
        sectorCols = 2; sectorRows = 1; // 2 windows side-by-side
    } else if (currentLevel == 2) {
        sectorCols = 2; sectorRows = 2; // 2x2 = 4 windows
    } else if (currentLevel == 3) {
        sectorCols = 3; sectorRows = 2; // 3x2 = 6 windows
    } else if (currentLevel == 4) {
        sectorCols = 2; sectorRows = 3; // 2x3 = 6 vertical windows
    } else {
        sectorCols = 3; sectorRows = 3; // 3x3 = 9 windows (mega labyrinth!)
    }
    sectorCols = std::clamp(sectorCols, 1, MAX_SECTORS_X);
    sectorRows = std::clamp(sectorRows, 1, MAX_SECTORS_Y);

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

    // 5. Spawn Ghosts across sectors
    ghosts.clear();
    // 2 ghosts in the starting window
    ghosts.push_back({currentSectorX, currentSectorY, 10, 9, UP, 0});     // Blinky (Red)
    ghosts.push_back({currentSectorX, currentSectorY, 9, 10, LEFT, 1});   // Pinky (Pink)

    // 1 ghost guarding the Exit window
    if (exitSectorX != currentSectorX || exitSectorY != currentSectorY) {
        ghosts.push_back({exitSectorX, exitSectorY, 10, 10, RIGHT, 2}); // Inky (Cyan)
    }

    // Additional roaming ghost in intermediate window if available
    if (sectorCols * sectorRows > 2) {
        int midSx = sectorCols / 2;
        int midSy = sectorRows / 2;
        ghosts.push_back({midSx, midSy, 10, 10, DOWN, 3}); // Clyde (Orange)
    }

    // Extra ghosts for higher levels (Level 6+)
    if (currentLevel >= 6) {
        ghosts.push_back({0, 0, 10, 10, UP, 0});
    }
    if (currentLevel >= 7) {
        ghosts.push_back({exitSectorX, exitSectorY, 9, 9, DOWN, 1});
    }

    ghostsFrightened = false;
    frightenedTicksLeft = 0;

    ghostMoveEveryN = std::max(1, 4 - currentLevel / 2);
    ghostTickCounter = 0;

    activeFruits.clear();
    fruitSpawnCounter = QRandomGenerator::global()->bounded(15, 35);

    fogOfWarEnabled = true;
    fogPeekActive = false;

    updateFogOfWar();
    elapsedTimer.start();
}

void MainWindow::updateFogOfWar()
{
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            double dist1 = (p1Alive ? std::hypot(c - pacmanX, r - pacmanY) : 999.0);
            double dist2 = (isMultiplayer && p2Alive ? std::hypot(c - p2X, r - p2Y) : 999.0);
            if (dist1 <= VISION_RADIUS || dist2 <= VISION_RADIUS) {
                explored[currentSectorY][currentSectorX][r][c] = true;
            }
        }
    }
}

bool MainWindow::isCellVisible(int sx, int sy, int x, int y) const
{
    if (!fogOfWarEnabled) return true;
    if (sx != currentSectorX || sy != currentSectorY) return false;
    double dist1 = (p1Alive ? std::hypot(x - pacmanX, y - pacmanY) : 999.0);
    double dist2 = (isMultiplayer && p2Alive ? std::hypot(x - p2X, y - p2Y) : 999.0);
    return (dist1 <= VISION_RADIUS || dist2 <= VISION_RADIUS);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // Toggle Fullscreen with F11
    if (event->key() == Qt::Key_F11) {
        if (isFullScreen()) {
            showNormal();
        } else {
            showFullScreen();
        }
        update();
        return;
    }

    // Exit fullscreen with Escape key
    if (event->key() == Qt::Key_Escape && isFullScreen()) {
        showNormal();
        update();
        return;
    }

    // Home Map Key Handling
    if (currentScreen == SCREEN_HOME_MAP) {
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            startLevel(selectedMapLevel);
            return;
        }
        if (event->key() == Qt::Key_Tab || event->key() == Qt::Key_C || event->key() == Qt::Key_P) {
            if (!isMultiplayer && singlePlayerHero == 1) {
                singlePlayerHero = 2; // Switch to Pink solo
                isMultiplayer = false;
            } else if (!isMultiplayer && singlePlayerHero == 2) {
                isMultiplayer = true; // Switch to 2P Co-op
            } else {
                isMultiplayer = false;
                singlePlayerHero = 1; // Switch to Yellow solo
            }
            update();
            return;
        }
        if (event->key() == Qt::Key_1) {
            isMultiplayer = false;
            singlePlayerHero = 1;
            update();
            return;
        }
        if (event->key() == Qt::Key_2) {
            isMultiplayer = false;
            singlePlayerHero = 2;
            update();
            return;
        }
        if (event->key() == Qt::Key_3) {
            isMultiplayer = true;
            update();
            return;
        }
        if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Right) {
            if (selectedMapLevel < highestUnlockedLevel) {
                selectedMapLevel++;
                update();
            }
            return;
        }
        if (event->key() == Qt::Key_Down || event->key() == Qt::Key_Left) {
            if (selectedMapLevel > 1) {
                selectedMapLevel--;
                update();
            }
            return;
        }
        return;
    }

    // In-Game Game Over (Retry from that particular level!)
    if (gameOver) {
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_R) {
            initLevel();
            gameOver = false;
            update();
        } else if (event->key() == Qt::Key_M) {
            currentScreen = SCREEN_HOME_MAP;
            selectedMapLevel = currentLevel;
            update();
        }
        return;
    }

    // In-Game Level Complete
    if (levelComplete) {
        if (event->key() == Qt::Key_Space) {
            currentLevel++;
            if (currentLevel > TOTAL_LEVELS) currentLevel = TOTAL_LEVELS;
            if (currentLevel > highestUnlockedLevel) highestUnlockedLevel = currentLevel;
            selectedMapLevel = currentLevel;
            initLevel();
            levelComplete = false;
            update();
        } else if (event->key() == Qt::Key_M) {
            currentScreen = SCREEN_HOME_MAP;
            selectedMapLevel = std::min(TOTAL_LEVELS, currentLevel + 1);
            update();
        }
        return;
    }

    // Return to Saga Map with [M]
    if (event->key() == Qt::Key_M) {
        currentScreen = SCREEN_HOME_MAP;
        selectedMapLevel = currentLevel;
        update();
        return;
    }

    // Restart current level with [R]
    if (event->key() == Qt::Key_R) {
        initLevel();
        update();
        return;
    }

    // Toggle Fog of War: 1.5-second temporary peek
    if (event->key() == Qt::Key_F) {
        if (fogOfWarEnabled) {
            fogOfWarEnabled = false;
            fogPeekActive = true;
            fogPeekTimer.start();
        } else {
            fogOfWarEnabled = true;
            fogPeekActive = false;
        }
        update();
        return;
    }

    // In-Game Switch Active Arrow-Controlled Hero: [Tab] or [C]
    if (event->key() == Qt::Key_Tab || event->key() == Qt::Key_C) {
        if (!isMultiplayer) {
            // In Solo mode, flip hero between Yellow and Pink
            singlePlayerHero = (singlePlayerHero == 1 ? 2 : 1);
            arrowPlayer = singlePlayerHero;
        } else {
            // In Co-op mode, switch which character is steered by Arrow keys
            arrowPlayer = (arrowPlayer == 1 ? 2 : 1);
        }
        update();
        return;
    }

    // Toggle 2-Player Co-op drop-in on the fly with [2]
    if (event->key() == Qt::Key_2 && !isMultiplayer) {
        isMultiplayer = true;
        p2Alive = true;
        p2RespawnTicks = 0;
        p2X = (pacmanX > 1 && board[currentSectorY][currentSectorX][pacmanY][pacmanX - 1] != 1) ? pacmanX - 1 : pacmanX;
        p2Y = pacmanY;
        update();
        return;
    }

    // Prevent input during active camera window slide transition
    if (isTransitioning) return;

    // Arrow keys input
    Direction arrowAttempt = STOP;
    if (event->key() == Qt::Key_Up) arrowAttempt = UP;
    else if (event->key() == Qt::Key_Down) arrowAttempt = DOWN;
    else if (event->key() == Qt::Key_Left) arrowAttempt = LEFT;
    else if (event->key() == Qt::Key_Right) arrowAttempt = RIGHT;

    // WASD keys input
    Direction wasdAttempt = STOP;
    int k = event->key();
    QString txt = event->text().toLower();
    if (k == Qt::Key_W || txt == "w") wasdAttempt = UP;
    else if (k == Qt::Key_S || txt == "s") wasdAttempt = DOWN;
    else if (k == Qt::Key_A || txt == "a") wasdAttempt = LEFT;
    else if (k == Qt::Key_D || txt == "d") wasdAttempt = RIGHT;

    // Neither movement key pressed
    if (arrowAttempt == STOP && wasdAttempt == STOP) {
        QWidget::keyPressEvent(event);
        return;
    }

    // 1. Single Player Mode (Move your chosen hero with Arrows OR WASD)
    if (!isMultiplayer) {
        Direction dir = (arrowAttempt != STOP) ? arrowAttempt : wasdAttempt;
        if (dir != STOP && p1Alive) {
            pacmanNextDir = dir;
            movePacman();
            moveCount++;
            checkFruitPickup();
            checkGhostCollisions();
            checkLevelExit();
            update();
        }
        return;
    }

    // 2. 2-Player Co-op Mode
    // arrowPlayer determines who responds to the Arrow Keys!
    // The other player responds to WASD!
    Direction p1Cmd = (arrowPlayer == 1) ? arrowAttempt : wasdAttempt;
    Direction p2Cmd = (arrowPlayer == 2) ? arrowAttempt : wasdAttempt;

    if (p1Cmd != STOP && p1Alive) {
        pacmanNextDir = p1Cmd;
        movePacman();
        moveCount++;
        checkFruitPickup();
        checkGhostCollisions();
        checkLevelExit();
        update();
        return;
    }

    if (p2Cmd != STOP && p2Alive) {
        p2NextDir = p2Cmd;
        movePlayer2();
        moveCount++;
        checkFruitPickup();
        checkGhostCollisions();
        checkLevelExit();
        update();
        return;
    }

    QWidget::keyPressEvent(event);
}

void MainWindow::checkLevelExit()
{
    bool p1Won = p1Alive && (pacmanX == exitX && pacmanY == exitY);
    bool p2Won = isMultiplayer && p2Alive && (p2X == exitX && p2Y == exitY);

    if (currentSectorX == exitSectorX && currentSectorY == exitSectorY && (p1Won || p2Won)) {
        levelComplete = true;
        int clearBonus = 500 * (sectorCols * sectorRows);
        if (p1Won && p2Won) {
            scoreP1 += clearBonus / 2;
            scoreP2 += clearBonus / 2;
        } else if (p1Won) {
            scoreP1 += clearBonus;
        } else {
            scoreP2 += clearBonus;
        }
        score = scoreP1 + scoreP2;
        if (currentLevel >= highestUnlockedLevel) {
            highestUnlockedLevel = std::min(TOTAL_LEVELS, currentLevel + 1);
        }
        selectedMapLevel = std::min(TOTAL_LEVELS, currentLevel + 1);
    }
}

bool MainWindow::canMove(int x, int y, Direction dir)
{
    int nextX = x, nextY = y;
    if (dir == UP) nextY--;
    else if (dir == DOWN) nextY++;
    else if (dir == LEFT) nextX--;
    else if (dir == RIGHT) nextX++;

    // Inside current sector
    if (nextX >= 0 && nextX < COLS && nextY >= 0 && nextY < ROWS) {
        return board[currentSectorY][currentSectorX][nextY][nextX] != 1;
    }

    // Crossing to East window
    if (nextX >= COLS) {
        return (currentSectorX + 1 < sectorCols) &&
               (board[currentSectorY][currentSectorX][y][x] != 1) &&
               (board[currentSectorY][currentSectorX + 1][y][0] != 1);
    }
    // Crossing to West window
    if (nextX < 0) {
        return (currentSectorX - 1 >= 0) &&
               (board[currentSectorY][currentSectorX][y][x] != 1) &&
               (board[currentSectorY][currentSectorX - 1][y][COLS - 1] != 1);
    }
    // Crossing to South window
    if (nextY >= ROWS) {
        return (currentSectorY + 1 < sectorRows) &&
               (board[currentSectorY][currentSectorX][y][x] != 1) &&
               (board[currentSectorY + 1][currentSectorX][0][x] != 1);
    }
    // Crossing to North window
    if (nextY < 0) {
        return (currentSectorY - 1 >= 0) &&
               (board[currentSectorY][currentSectorX][y][x] != 1) &&
               (board[currentSectorY - 1][currentSectorX][ROWS - 1][x] != 1);
    }

    return false;
}

void MainWindow::movePacman()
{
    if (!p1Alive) return;

    if (canMove(pacmanX, pacmanY, pacmanNextDir)) {
        pacmanDir = pacmanNextDir;
    }

    if (canMove(pacmanX, pacmanY, pacmanDir)) {
        int nextX = pacmanX, nextY = pacmanY;
        if (pacmanDir == UP) nextY--;
        else if (pacmanDir == DOWN) nextY++;
        else if (pacmanDir == LEFT) nextX--;
        else if (pacmanDir == RIGHT) nextX++;

        // Check if crossing into an adjacent window
        if (nextX >= COLS) {
            fromSectorX = currentSectorX; fromSectorY = currentSectorY;
            toSectorX = currentSectorX + 1; toSectorY = currentSectorY;
            transitionDirX = 1; transitionDirY = 0;
            currentSectorX = toSectorX;
            pacmanX = 0;
            if (isMultiplayer) {
                p2X = 0;
                p2Y = (pacmanY == 9 ? 10 : 9);
            }
            isTransitioning = true;
            transitionTimer.start();
            if (!sectorDiscovered[currentSectorY][currentSectorX]) {
                newSectorJustDiscovered = true;
                newSectorTimer.start();
            }
            sectorDiscovered[currentSectorY][currentSectorX] = true;
        } else if (nextX < 0) {
            fromSectorX = currentSectorX; fromSectorY = currentSectorY;
            toSectorX = currentSectorX - 1; toSectorY = currentSectorY;
            transitionDirX = -1; transitionDirY = 0;
            currentSectorX = toSectorX;
            pacmanX = COLS - 1;
            if (isMultiplayer) {
                p2X = COLS - 1;
                p2Y = (pacmanY == 9 ? 10 : 9);
            }
            isTransitioning = true;
            transitionTimer.start();
            if (!sectorDiscovered[currentSectorY][currentSectorX]) {
                newSectorJustDiscovered = true;
                newSectorTimer.start();
            }
            sectorDiscovered[currentSectorY][currentSectorX] = true;
        } else if (nextY >= ROWS) {
            fromSectorX = currentSectorX; fromSectorY = currentSectorY;
            toSectorX = currentSectorX; toSectorY = currentSectorY + 1;
            transitionDirX = 0; transitionDirY = 1;
            currentSectorY = toSectorY;
            pacmanY = 0;
            if (isMultiplayer) {
                p2Y = 0;
                p2X = (pacmanX == 9 ? 10 : 9);
            }
            isTransitioning = true;
            transitionTimer.start();
            if (!sectorDiscovered[currentSectorY][currentSectorX]) {
                newSectorJustDiscovered = true;
                newSectorTimer.start();
            }
            sectorDiscovered[currentSectorY][currentSectorX] = true;
        } else if (nextY < 0) {
            fromSectorX = currentSectorX; fromSectorY = currentSectorY;
            toSectorX = currentSectorX; toSectorY = currentSectorY - 1;
            transitionDirX = 0; transitionDirY = -1;
            currentSectorY = toSectorY;
            pacmanY = ROWS - 1;
            if (isMultiplayer) {
                p2Y = ROWS - 1;
                p2X = (pacmanX == 9 ? 10 : 9);
            }
            isTransitioning = true;
            transitionTimer.start();
            if (!sectorDiscovered[currentSectorY][currentSectorX]) {
                newSectorJustDiscovered = true;
                newSectorTimer.start();
            }
            sectorDiscovered[currentSectorY][currentSectorX] = true;
        } else {
            pacmanX = nextX;
            pacmanY = nextY;
        }

        updateFogOfWar();

        // Eat dot
        if (board[currentSectorY][currentSectorX][pacmanY][pacmanX] == 2) {
            board[currentSectorY][currentSectorX][pacmanY][pacmanX] = 0;
            scoreP1 += 10;
            score = scoreP1 + scoreP2;
        }

        // Mouth animation
        if (pacmanMouthOpening) {
            pacmanMouthAngle += 15;
            if (pacmanMouthAngle >= 45) pacmanMouthOpening = false;
        } else {
            pacmanMouthAngle -= 15;
            if (pacmanMouthAngle <= 0) pacmanMouthOpening = true;
        }
    }
}

void MainWindow::movePlayer2()
{
    if (!p2Alive) return;

    if (canMove(p2X, p2Y, p2NextDir)) {
        p2Dir = p2NextDir;
    }

    if (canMove(p2X, p2Y, p2Dir)) {
        int nextX = p2X, nextY = p2Y;
        if (p2Dir == UP) nextY--;
        else if (p2Dir == DOWN) nextY++;
        else if (p2Dir == LEFT) nextX--;
        else if (p2Dir == RIGHT) nextX++;

        // Check if crossing into an adjacent window
        if (nextX >= COLS) {
            fromSectorX = currentSectorX; fromSectorY = currentSectorY;
            toSectorX = currentSectorX + 1; toSectorY = currentSectorY;
            transitionDirX = 1; transitionDirY = 0;
            currentSectorX = toSectorX;
            p2X = 0;
            pacmanX = 0;
            pacmanY = (p2Y == 9 ? 10 : 9);
            isTransitioning = true;
            transitionTimer.start();
            if (!sectorDiscovered[currentSectorY][currentSectorX]) {
                newSectorJustDiscovered = true;
                newSectorTimer.start();
            }
            sectorDiscovered[currentSectorY][currentSectorX] = true;
        } else if (nextX < 0) {
            fromSectorX = currentSectorX; fromSectorY = currentSectorY;
            toSectorX = currentSectorX - 1; toSectorY = currentSectorY;
            transitionDirX = -1; transitionDirY = 0;
            currentSectorX = toSectorX;
            p2X = COLS - 1;
            pacmanX = COLS - 1;
            pacmanY = (p2Y == 9 ? 10 : 9);
            isTransitioning = true;
            transitionTimer.start();
            if (!sectorDiscovered[currentSectorY][currentSectorX]) {
                newSectorJustDiscovered = true;
                newSectorTimer.start();
            }
            sectorDiscovered[currentSectorY][currentSectorX] = true;
        } else if (nextY >= ROWS) {
            fromSectorX = currentSectorX; fromSectorY = currentSectorY;
            toSectorX = currentSectorX; toSectorY = currentSectorY + 1;
            transitionDirX = 0; transitionDirY = 1;
            currentSectorY = toSectorY;
            p2Y = 0;
            pacmanY = 0;
            pacmanX = (p2X == 9 ? 10 : 9);
            isTransitioning = true;
            transitionTimer.start();
            if (!sectorDiscovered[currentSectorY][currentSectorX]) {
                newSectorJustDiscovered = true;
                newSectorTimer.start();
            }
            sectorDiscovered[currentSectorY][currentSectorX] = true;
        } else if (nextY < 0) {
            fromSectorX = currentSectorX; fromSectorY = currentSectorY;
            toSectorX = currentSectorX; toSectorY = currentSectorY - 1;
            transitionDirX = 0; transitionDirY = -1;
            currentSectorY = toSectorY;
            p2Y = ROWS - 1;
            pacmanY = ROWS - 1;
            pacmanX = (p2X == 9 ? 10 : 9);
            isTransitioning = true;
            transitionTimer.start();
            if (!sectorDiscovered[currentSectorY][currentSectorX]) {
                newSectorJustDiscovered = true;
                newSectorTimer.start();
            }
            sectorDiscovered[currentSectorY][currentSectorX] = true;
        } else {
            p2X = nextX;
            p2Y = nextY;
        }

        updateFogOfWar();

        // Eat dot
        if (board[currentSectorY][currentSectorX][p2Y][p2X] == 2) {
            board[currentSectorY][currentSectorX][p2Y][p2X] = 0;
            scoreP2 += 10;
            score = scoreP1 + scoreP2;
        }

        // Mouth animation
        if (p2MouthOpening) {
            p2MouthAngle += 15;
            if (p2MouthAngle >= 45) p2MouthOpening = false;
        } else {
            p2MouthAngle -= 15;
            if (p2MouthAngle <= 0) p2MouthOpening = true;
        }
    }
}

void MainWindow::moveGhosts()
{
    for (Ghost &g : ghosts) {
        if (ghostsFrightened && QRandomGenerator::global()->bounded(3) == 0) {
            continue;
        }

        QVector<Direction> possibleDirs;
        Direction dirs[] = {UP, DOWN, LEFT, RIGHT};

        Direction reverseDir = STOP;
        if (g.dir == UP) reverseDir = DOWN;
        if (g.dir == DOWN) reverseDir = UP;
        if (g.dir == LEFT) reverseDir = RIGHT;
        if (g.dir == RIGHT) reverseDir = LEFT;

        for (Direction d : dirs) {
            int nx = g.x, ny = g.y;
            if (d == UP) ny--;
            else if (d == DOWN) ny++;
            else if (d == LEFT) nx--;
            else if (d == RIGHT) nx++;

            if (nx >= 0 && nx < COLS && ny >= 0 && ny < ROWS) {
                if (board[g.sectorY][g.sectorX][ny][nx] != 1) {
                    if (d != reverseDir) possibleDirs.push_back(d);
                }
            }
        }

        if (possibleDirs.isEmpty()) {
            g.dir = reverseDir;
        } else {
            // Target nearest living player
            int targetX = pacmanX;
            int targetY = pacmanY;
            if (isMultiplayer) {
                if (!p1Alive && p2Alive) {
                    targetX = p2X; targetY = p2Y;
                } else if (p1Alive && p2Alive) {
                    int d1 = std::abs(g.x - pacmanX) + std::abs(g.y - pacmanY);
                    int d2 = std::abs(g.x - p2X) + std::abs(g.y - p2Y);
                    if (d2 < d1) {
                        targetX = p2X; targetY = p2Y;
                    }
                }
            }

            // If in the same sector as target, actively hunt
            if (g.sectorX == currentSectorX && g.sectorY == currentSectorY && !ghostsFrightened) {
                int bestDist = 999;
                Direction bestDir = possibleDirs[0];
                for (Direction d : possibleDirs) {
                    int nx = g.x, ny = g.y;
                    if (d == UP) ny--;
                    else if (d == DOWN) ny++;
                    else if (d == LEFT) nx--;
                    else if (d == RIGHT) nx++;
                    int dist = std::abs(nx - targetX) + std::abs(ny - targetY);
                    if (dist < bestDist) {
                        bestDist = dist;
                        bestDir = d;
                    }
                }
                g.dir = bestDir;
            } else if (g.sectorX == currentSectorX && g.sectorY == currentSectorY && ghostsFrightened) {
                // Frightened: run away from target
                int bestDist = -1;
                Direction bestDir = possibleDirs[0];
                for (Direction d : possibleDirs) {
                    int nx = g.x, ny = g.y;
                    if (d == UP) ny--;
                    else if (d == DOWN) ny++;
                    else if (d == LEFT) nx--;
                    else if (d == RIGHT) nx++;
                    int dist = std::abs(nx - targetX) + std::abs(ny - targetY);
                    if (dist > bestDist) {
                        bestDist = dist;
                        bestDir = d;
                    }
                }
                g.dir = bestDir;
            } else {
                int r = QRandomGenerator::global()->bounded(possibleDirs.size());
                g.dir = possibleDirs[r];
            }
        }

        if (g.dir == UP) g.y--;
        else if (g.dir == DOWN) g.y++;
        else if (g.dir == LEFT) g.x--;
        else if (g.dir == RIGHT) g.x++;

        // Keep ghost safely in bounds of its sector
        if (g.x < 1) g.x = 1;
        if (g.x >= COLS - 1) g.x = COLS - 2;
        if (g.y < 1) g.y = 1;
        if (g.y >= ROWS - 1) g.y = ROWS - 2;
    }
}

void MainWindow::checkGhostCollisions()
{
    for (Ghost &g : ghosts) {
        if (g.sectorX != currentSectorX || g.sectorY != currentSectorY) continue;

        // Player 1 collision
        if (p1Alive && g.x == pacmanX && g.y == pacmanY) {
            if (ghostsFrightened) {
                scoreP1 += 200;
                score = scoreP1 + scoreP2;
                respawnGhostFarAway(g);
            } else {
                if (!isMultiplayer) {
                    gameOver = true;
                } else {
                    p1Alive = false;
                    p1RespawnTicks = 30; // ~3.9 seconds
                    if (!p2Alive) {
                        gameOver = true;
                    }
                }
            }
        }

        // Player 2 collision (Multiplayer)
        if (isMultiplayer && p2Alive && g.x == p2X && g.y == p2Y) {
            if (ghostsFrightened) {
                scoreP2 += 200;
                score = scoreP1 + scoreP2;
                respawnGhostFarAway(g);
            } else {
                p2Alive = false;
                p2RespawnTicks = 30; // ~3.9 seconds
                if (!p1Alive) {
                    gameOver = true;
                }
            }
        }
    }
}

void MainWindow::respawnGhostFarAway(Ghost &g)
{
    // Respawn ghost in a distant sector if available
    int bestDist = -1;
    int bestSx = currentSectorX, bestSy = currentSectorY;

    for (int sy = 0; sy < sectorRows; ++sy) {
        for (int sx = 0; sx < sectorCols; ++sx) {
            int d = std::abs(sx - currentSectorX) + std::abs(sy - currentSectorY);
            if (d > bestDist) {
                bestDist = d;
                bestSx = sx;
                bestSy = sy;
            }
        }
    }

    g.sectorX = bestSx;
    g.sectorY = bestSy;
    g.x = 10;
    g.y = 10;
    g.dir = UP;
}

// ========================================================
// Bonus Fruit System
// ========================================================

void MainWindow::trySpawnFruit()
{
    fruitSpawnCounter--;
    if (fruitSpawnCounter > 0) return;

    fruitSpawnCounter = QRandomGenerator::global()->bounded(20, 50);
    if (activeFruits.size() >= MAX_FRUITS) return;

    // Pick any discovered sector to spawn fruit
    QVector<QPair<int,int>> discSectors;
    for (int sy = 0; sy < sectorRows; ++sy) {
        for (int sx = 0; sx < sectorCols; ++sx) {
            if (sectorDiscovered[sy][sx]) discSectors.push_back({sx, sy});
        }
    }
    if (discSectors.isEmpty()) return;

    int sIdx = QRandomGenerator::global()->bounded(discSectors.size());
    int fsx = discSectors[sIdx].first;
    int fsy = discSectors[sIdx].second;

    QVector<QPair<int,int>> candidates;
    for (int r = 1; r < ROWS - 1; ++r) {
        for (int c = 1; c < COLS - 1; ++c) {
            if (board[fsy][fsx][r][c] == 0) {
                if (fsx == currentSectorX && fsy == currentSectorY && c == pacmanX && r == pacmanY) continue;
                if (fsx == exitSectorX && fsy == exitSectorY && c == exitX && r == exitY) continue;
                candidates.push_back({c, r});
            }
        }
    }

    if (candidates.isEmpty()) return;

    int idx = QRandomGenerator::global()->bounded(candidates.size());
    FruitType type = static_cast<FruitType>(QRandomGenerator::global()->bounded(4));
    int lifespan = QRandomGenerator::global()->bounded(45, 90);

    activeFruits.push_back({fsx, fsy, candidates[idx].first, candidates[idx].second, type, lifespan});
}

void MainWindow::checkFruitPickup()
{
    for (int i = activeFruits.size() - 1; i >= 0; --i) {
        if (activeFruits[i].sectorX == currentSectorX && activeFruits[i].sectorY == currentSectorY) {
            bool p1Picked = p1Alive && (activeFruits[i].x == pacmanX && activeFruits[i].y == pacmanY);
            bool p2Picked = isMultiplayer && p2Alive && (activeFruits[i].x == p2X && activeFruits[i].y == p2Y);
            if (p1Picked || p2Picked) {
                if (p1Picked) scoreP1 += 100;
                if (p2Picked) scoreP2 += 100;
                score = scoreP1 + scoreP2;
                activeFruits.removeAt(i);

                ghostsFrightened = true;
                frightenedTicksLeft = FRIGHTEN_DURATION;
            }
        }
    }
}

// ========================================================
// Responsive Scaled Fruit Drawing Helpers
// ========================================================

void MainWindow::drawCherry(QPainter &painter, int cx, int cy, int s)
{
    painter.setPen(QPen(QColor(0, 160, 0), std::max(2, s / 15)));
    painter.drawLine(cx + s/2, cy + s * 4 / 30, cx + s/2 - s * 4 / 30, cy + s/2 - s * 2 / 30);
    painter.drawLine(cx + s/2, cy + s * 4 / 30, cx + s/2 + s * 4 / 30, cy + s/2 - s * 2 / 30);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(220, 20, 20));
    int cherryD = std::max(6, s * 10 / 30);
    painter.drawEllipse(cx + s/2 - s * 9 / 30, cy + s/2 - s * 3 / 30, cherryD, cherryD);
    painter.setBrush(QColor(200, 10, 10));
    painter.drawEllipse(cx + s/2 + s * 1 / 30, cy + s/2 - s * 3 / 30, cherryD, cherryD);

    painter.setBrush(QColor(255, 180, 180, 180));
    int hlD = std::max(2, s * 3 / 30);
    painter.drawEllipse(cx + s/2 - s * 7 / 30, cy + s/2, hlD, hlD);
    painter.drawEllipse(cx + s/2 + s * 3 / 30, cy + s/2, hlD, hlD);
}

void MainWindow::drawApple(QPainter &painter, int cx, int cy, int s)
{
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(50, 200, 50));
    painter.drawEllipse(cx + s * 5 / 30, cy + s * 7 / 30, s - s * 10 / 30, s - s * 10 / 30);

    painter.setBrush(QColor(120, 255, 120, 150));
    painter.drawEllipse(cx + s * 8 / 30, cy + s * 9 / 30, std::max(3, s * 5 / 30), std::max(3, s * 5 / 30));

    painter.setPen(QPen(QColor(100, 60, 20), std::max(2, s / 15)));
    painter.drawLine(cx + s/2, cy + s * 7 / 30, cx + s/2, cy + s * 3 / 30);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(30, 160, 30));
    painter.drawEllipse(cx + s/2, cy + s * 2 / 30, std::max(3, s * 6 / 30), std::max(2, s * 4 / 30));
}

void MainWindow::drawStrawberry(QPainter &painter, int cx, int cy, int s)
{
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(220, 30, 50));
    QPolygon strawberry;
    strawberry << QPoint(cx + s/2, cy + s - s * 5 / 30)
               << QPoint(cx + s * 5 / 30, cy + s * 8 / 30)
               << QPoint(cx + s - s * 5 / 30, cy + s * 8 / 30);
    painter.drawPolygon(strawberry);
    painter.drawEllipse(cx + s * 5 / 30, cy + s * 6 / 30, s - s * 10 / 30, std::max(4, s * 8 / 30));

    painter.setBrush(QColor(255, 255, 100));
    int seedD = std::max(2, s * 2 / 30);
    painter.drawEllipse(cx + s/2 - s * 3 / 30, cy + s * 12 / 30, seedD, seedD);
    painter.drawEllipse(cx + s/2 + s * 2 / 30, cy + s * 14 / 30, seedD, seedD);
    painter.drawEllipse(cx + s/2 - s * 1 / 30, cy + s * 17 / 30, seedD, seedD);

    painter.setBrush(QColor(30, 160, 30));
    int leafW = std::max(3, s * 5 / 30);
    int leafH = std::max(2, s * 4 / 30);
    painter.drawEllipse(cx + s/2 - s * 5 / 30, cy + s * 4 / 30, leafW, leafH);
    painter.drawEllipse(cx + s/2 + s * 1 / 30, cy + s * 4 / 30, leafW, leafH);
}

void MainWindow::drawOrangeFruit(QPainter &painter, int cx, int cy, int s)
{
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 160, 30));
    painter.drawEllipse(cx + s * 5 / 30, cy + s * 6 / 30, s - s * 10 / 30, s - s * 10 / 30);

    painter.setBrush(QColor(255, 220, 120, 160));
    int hlD = std::max(3, s * 5 / 30);
    painter.drawEllipse(cx + s * 8 / 30, cy + s * 9 / 30, hlD, hlD);

    painter.setPen(QPen(QColor(100, 60, 20), std::max(2, s / 15)));
    painter.drawLine(cx + s/2, cy + s * 6 / 30, cx + s/2, cy + s * 3 / 30);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(30, 160, 30));
    painter.drawEllipse(cx + s/2, cy + s * 1 / 30, std::max(3, s * 6 / 30), std::max(2, s * 4 / 30));
}

// ========================================================
// Drawing a Single Sector Window
// ========================================================

void MainWindow::drawSector(QPainter &painter, int sx, int sy, int originX, int originY, int cellSize)
{
    bool isCurrent = (sx == currentSectorX && sy == currentSectorY);

    // 1. Draw Maze Tiles & Dots
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            int cx = originX + c * cellSize;
            int cy = originY + r * cellSize;

            bool visible = isCellVisible(sx, sy, c, r);
            bool wasExplored = explored[sy][sx][r][c];

            if (fogOfWarEnabled && !visible && !wasExplored) {
                painter.fillRect(cx, cy, cellSize, cellSize, QColor(0, 0, 0));
                continue;
            }

            if (board[sy][sx][r][c] == 1) {
                // Wall
                if (visible) {
                    painter.setBrush(QColor(25, 45, 130));
                    painter.setPen(QPen(QColor(40, 80, 200), std::max(1, cellSize / 25)));
                } else {
                    painter.setBrush(QColor(14, 22, 50));
                    painter.setPen(QPen(QColor(24, 38, 80), std::max(1, cellSize / 25)));
                }
                int cornerR = std::max(3, cellSize / 7);
                painter.drawRoundedRect(cx + 1, cy + 1, cellSize - 2, cellSize - 2, cornerR, cornerR);
            } else {
                // Open corridor
                if (visible) {
                    painter.fillRect(cx, cy, cellSize, cellSize, QColor(8, 8, 16));
                    if (board[sy][sx][r][c] == 2) {
                        painter.setBrush(QColor(255, 230, 180));
                        painter.setPen(Qt::NoPen);
                        int dotSize = std::max(4, cellSize * 5 / 30);
                        painter.drawEllipse(cx + cellSize/2 - dotSize/2, cy + cellSize/2 - dotSize/2, dotSize, dotSize);
                    }
                } else {
                    painter.fillRect(cx, cy, cellSize, cellSize, QColor(4, 4, 10));
                    if (board[sy][sx][r][c] == 2) {
                        painter.setBrush(QColor(100, 95, 80, 75));
                        painter.setPen(Qt::NoPen);
                        int dotSize = std::max(2, cellSize * 3 / 30);
                        painter.drawEllipse(cx + cellSize/2 - dotSize/2, cy + cellSize/2 - dotSize/2, dotSize, dotSize);
                    }
                }
            }
        }
    }

    // 2. Draw Bonus Fruits in this sector
    for (const BonusFruit &fruit : std::as_const(activeFruits)) {
        if (fruit.sectorX != sx || fruit.sectorY != sy) continue;
        if (!isCellVisible(sx, sy, fruit.x, fruit.y)) continue;

        int fx = originX + fruit.x * cellSize;
        int fy = originY + fruit.y * cellSize;

        switch (fruit.type) {
            case CHERRY:       drawCherry(painter, fx, fy, cellSize); break;
            case APPLE:        drawApple(painter, fx, fy, cellSize); break;
            case STRAWBERRY:   drawStrawberry(painter, fx, fy, cellSize); break;
            case ORANGE_FRUIT: drawOrangeFruit(painter, fx, fy, cellSize); break;
        }

        painter.setPen(QColor(255, 255, 100));
        painter.setFont(QFont("Arial", std::max(7, cellSize / 4), QFont::Bold));
        painter.drawText(fx + 3, fy + cellSize - 1, "100");
    }

    // 5. Draw Primary Hero / Player 1 (if in this sector)
    if (isCurrent && (p1Alive || (p1RespawnTicks > 0 && p1RespawnTicks % 4 < 2))) {
        int px = originX + pacmanX * cellSize;
        int py = originY + pacmanY * cellSize;

        bool isPinkSolo = (!isMultiplayer && singlePlayerHero == 2);

        if (isPinkSolo) {
            // Drawn as Pink Ms. Pacman in solo mode
            painter.setBrush(p1Alive ? QColor(255, 105, 180) : QColor(255, 150, 200, 110));
        } else {
            painter.setBrush(p1Alive ? QColor(255, 220, 0) : QColor(255, 240, 100, 110));
        }
        painter.setPen(Qt::NoPen);
        int startAngle = 0;
        if (pacmanDir == RIGHT) startAngle = pacmanMouthAngle;
        else if (pacmanDir == DOWN) startAngle = 270 + pacmanMouthAngle;
        else if (pacmanDir == LEFT) startAngle = 180 + pacmanMouthAngle;
        else if (pacmanDir == UP) startAngle = 90 + pacmanMouthAngle;

        int spanAngle = 360 - 2 * pacmanMouthAngle;
        painter.drawPie(px + 2, py + 2, cellSize - 4, cellSize - 4, startAngle * 16, spanAngle * 16);

        if (isPinkSolo) {
            // Cute Red Ribbon Bow on head
            painter.setBrush(QColor(235, 20, 60));
            int bowSize = std::max(4, cellSize / 4);
            int bx = px + cellSize / 2;
            int by = py + 2;
            painter.drawEllipse(bx - bowSize, by, bowSize * 3 / 4, bowSize / 2);
            painter.drawEllipse(bx, by, bowSize * 3 / 4, bowSize / 2);
            painter.setBrush(QColor(255, 220, 230));
            painter.drawEllipse(bx - bowSize / 4, by + bowSize / 6, bowSize / 2, bowSize / 2);

            // Eye beauty mark
            painter.setBrush(QColor(20, 20, 60));
            painter.drawEllipse(px + cellSize * 3 / 5, py + cellSize / 3, std::max(2, cellSize / 7), std::max(2, cellSize / 7));
        }

        // Active control indicator badge
        painter.setFont(QFont("Arial", std::max(6, cellSize / 4), QFont::Bold));
        if (isMultiplayer) {
            if (arrowPlayer == 1) {
                painter.setPen(QColor(255, 240, 0));
                painter.drawText(px - 10, py - 4, cellSize + 20, cellSize / 2, Qt::AlignCenter, "P1 [ARROWS]");
            } else {
                painter.setPen(QColor(200, 200, 140));
                painter.drawText(px - 10, py - 4, cellSize + 20, cellSize / 2, Qt::AlignCenter, "P1 [WASD]");
            }
        }
    }

    // Draw Player 2 (Ms. Pacman - Pink with bow) (if multiplayer and in this sector)
    if (isMultiplayer && isCurrent && (p2Alive || (p2RespawnTicks > 0 && p2RespawnTicks % 4 < 2))) {
        int px = originX + p2X * cellSize;
        int py = originY + p2Y * cellSize;

        painter.setBrush(p2Alive ? QColor(255, 105, 180) : QColor(255, 150, 200, 110));
        painter.setPen(Qt::NoPen);
        int startAngle = 0;
        if (p2Dir == RIGHT) startAngle = p2MouthAngle;
        else if (p2Dir == DOWN) startAngle = 270 + p2MouthAngle;
        else if (p2Dir == LEFT) startAngle = 180 + p2MouthAngle;
        else if (p2Dir == UP) startAngle = 90 + p2MouthAngle;

        int spanAngle = 360 - 2 * p2MouthAngle;
        painter.drawPie(px + 2, py + 2, cellSize - 4, cellSize - 4, startAngle * 16, spanAngle * 16);

        // Cute Red Ribbon Bow on head
        painter.setBrush(QColor(235, 20, 60));
        int bowSize = std::max(4, cellSize / 4);
        int bx = px + cellSize / 2;
        int by = py + 2;
        painter.drawEllipse(bx - bowSize, by, bowSize * 3 / 4, bowSize / 2);
        painter.drawEllipse(bx, by, bowSize * 3 / 4, bowSize / 2);
        painter.setBrush(QColor(255, 220, 230));
        painter.drawEllipse(bx - bowSize / 4, by + bowSize / 6, bowSize / 2, bowSize / 2);

        // Eye beauty mark
        painter.setBrush(QColor(20, 20, 60));
        painter.drawEllipse(px + cellSize * 3 / 5, py + cellSize / 3, std::max(2, cellSize / 7), std::max(2, cellSize / 7));

        // Active control indicator badge
        painter.setFont(QFont("Arial", std::max(6, cellSize / 4), QFont::Bold));
        if (arrowPlayer == 2) {
            painter.setPen(QColor(255, 130, 200));
            painter.drawText(px - 10, py - 4, cellSize + 20, cellSize / 2, Qt::AlignCenter, "P2 [ARROWS]");
        } else {
            painter.setPen(QColor(220, 160, 200));
            painter.drawText(px - 10, py - 4, cellSize + 20, cellSize / 2, Qt::AlignCenter, "P2 [WASD]");
        }
    }

    // 6. Draw Ghosts in this sector
    QColor ghostNormalColors[] = {
        QColor(255, 50, 50),   // Blinky (Red)
        QColor(255, 180, 220), // Pinky (Pink)
        QColor(50, 220, 255),  // Inky (Cyan)
        QColor(255, 160, 50)   // Clyde (Orange)
    };

    for (const Ghost &g : std::as_const(ghosts)) {
        if (g.sectorX != sx || g.sectorY != sy) continue;
        if (!isCellVisible(sx, sy, g.x, g.y)) continue;

        int gx = originX + g.x * cellSize;
        int gy = originY + g.y * cellSize;

        if (ghostsFrightened) {
            if (frightenedTicksLeft <= 8 && (frightenedTicksLeft % 2 == 0)) {
                painter.setBrush(QColor(255, 255, 255));
            } else {
                painter.setBrush(QColor(30, 30, 200));
            }
        } else {
            painter.setBrush(ghostNormalColors[g.colorIndex]);
        }
        painter.setPen(Qt::NoPen);

        // Body
        painter.drawEllipse(gx + 2, gy + 2, cellSize - 4, cellSize - 4);
        painter.drawRect(gx + 2, gy + cellSize/2, cellSize - 4, (cellSize - 4)/2);

        // Wavy bottom
        int waveW = (cellSize - 4) / 3;
        for (int w = 0; w < 3; ++w) {
            int wx = gx + 2 + w * waveW;
            int wy = gy + cellSize - cellSize / 5;
            painter.drawEllipse(wx, wy, waveW, cellSize / 5);
        }

        if (ghostsFrightened) {
            painter.setBrush(QColor(255, 200, 200));
            int eyeSize = std::max(3, cellSize * 4 / 30);
            painter.drawEllipse(gx + cellSize * 8 / 30, gy + cellSize * 10 / 30, eyeSize, eyeSize);
            painter.drawEllipse(gx + cellSize * 18 / 30, gy + cellSize * 10 / 30, eyeSize, eyeSize);

            painter.setPen(QPen(QColor(255, 200, 200), 1));
            for (int m = 0; m < 4; ++m) {
                int mx = gx + cellSize * 6 / 30 + m * (cellSize * 5 / 30);
                int my = gy + cellSize * 18 / 30 + (m % 2 == 0 ? 0 : 2);
                painter.drawLine(mx, my, mx + cellSize * 4 / 30, my + (m % 2 == 0 ? 2 : -2));
            }
            painter.setPen(Qt::NoPen);
        } else {
            painter.setBrush(Qt::white);
            int eyeSize = std::max(5, cellSize * 6 / 30);
            painter.drawEllipse(gx + cellSize * 6 / 30, gy + cellSize * 8 / 30, eyeSize, eyeSize);
            painter.drawEllipse(gx + cellSize * 16 / 30, gy + cellSize * 8 / 30, eyeSize, eyeSize);

            painter.setBrush(Qt::blue);
            int pupilSize = std::max(3, cellSize * 3 / 30);
            painter.drawEllipse(gx + cellSize * 8 / 30, gy + cellSize * 10 / 30, pupilSize, pupilSize);
            painter.drawEllipse(gx + cellSize * 18 / 30, gy + cellSize * 10 / 30, pupilSize, pupilSize);
        }
    }

    // 5. Lantern Light Spotlight (only for current sector)
    if (isCurrent && fogOfWarEnabled) {
        if (p1Alive) {
            int pxCenter = originX + pacmanX * cellSize + cellSize / 2;
            int pyCenter = originY + pacmanY * cellSize + cellSize / 2;

            QRadialGradient lantern(pxCenter, pyCenter, (VISION_RADIUS + 0.6) * cellSize);
            lantern.setColorAt(0.0, QColor(255, 235, 170, 25));
            lantern.setColorAt(0.45, QColor(255, 200, 100, 10));
            lantern.setColorAt(0.75, QColor(0, 0, 0, 60));
            lantern.setColorAt(1.0, QColor(0, 0, 0, 220));

            painter.fillRect(originX, originY, COLS * cellSize, ROWS * cellSize, lantern);
        }
        if (isMultiplayer && p2Alive) {
            int p2xCenter = originX + p2X * cellSize + cellSize / 2;
            int p2yCenter = originY + p2Y * cellSize + cellSize / 2;

            QRadialGradient lantern2(p2xCenter, p2yCenter, (VISION_RADIUS + 0.6) * cellSize);
            lantern2.setColorAt(0.0, QColor(255, 190, 230, 25));
            lantern2.setColorAt(0.45, QColor(255, 150, 200, 10));
            lantern2.setColorAt(0.75, QColor(0, 0, 0, 60));
            lantern2.setColorAt(1.0, QColor(0, 0, 0, 220));

            painter.fillRect(originX, originY, COLS * cellSize, ROWS * cellSize, lantern2);
        }
    }

    // 6. Glowing Sci-Fi Gateway Portals (Cuts through darkness & visible across fog!)
    float pulseTime = elapsedTimer.elapsed() * 0.007f;
    float pulseVal = (std::sin(pulseTime) + 1.0f) * 0.5f;
    int gateAlpha = 190 + int(65 * pulseVal);

    // East Gateway (Connects to sector sx + 1)
    if (sx < sectorCols - 1 && board[sy][sx][9][COLS - 1] == 0) {
        int gx = originX + (COLS - 1) * cellSize;
        int gy = originY + 9 * cellSize;
        int gw = cellSize;
        int gh = 2 * cellSize;

        // Ambient cyan aura radiating into corridor
        QRadialGradient aura(gx + gw, gy + gh / 2, cellSize * 2.8);
        aura.setColorAt(0.0, QColor(0, 240, 255, 110));
        aura.setColorAt(0.4, QColor(0, 180, 255, 40));
        aura.setColorAt(1.0, QColor(0, 0, 0, 0));
        painter.fillRect(gx - cellSize * 2, gy - cellSize / 2, cellSize * 3, gh + cellSize, aura);

        // Pulsing neon portal border
        painter.setBrush(QColor(0, 140, 255, 50 + int(45 * pulseVal)));
        painter.setPen(QPen(QColor(0, 255, 240, gateAlpha), std::max(2, cellSize / 9)));
        painter.drawRoundedRect(gx, gy, gw, gh, 5, 5);

        // Animated Chevrons
        painter.setPen(QColor(255, 255, 255, gateAlpha));
        painter.setFont(QFont("Arial", std::max(10, cellSize / 2), QFont::Bold));
        painter.drawText(gx + cellSize * 0.12, gy + cellSize * 0.72, "▶");
        painter.drawText(gx + cellSize * 0.12, gy + cellSize * 1.62, "▶");

        // Glowing badge label
        painter.setFont(QFont("Arial", std::max(7, cellSize / 4), QFont::Bold));
        painter.setPen(QColor(0, 255, 240));
        bool isEastDiscovered = (sx + 1 < sectorCols && sectorDiscovered[sy][sx + 1]);
        QString eastLabel = isEastDiscovered ? QString("SECTOR (%1,%2) ▶").arg(sx + 2).arg(sy + 1) : "EXPLORE ▶";
        painter.drawText(gx - cellSize * 3 - 6, gy + cellSize * 1.1, eastLabel);
    }

    // West Gateway (Connects to sector sx - 1)
    if (sx > 0 && board[sy][sx][9][0] == 0) {
        int gx = originX;
        int gy = originY + 9 * cellSize;
        int gw = cellSize;
        int gh = 2 * cellSize;

        QRadialGradient aura(gx, gy + gh / 2, cellSize * 2.8);
        aura.setColorAt(0.0, QColor(0, 240, 255, 110));
        aura.setColorAt(0.4, QColor(0, 180, 255, 40));
        aura.setColorAt(1.0, QColor(0, 0, 0, 0));
        painter.fillRect(gx, gy - cellSize / 2, cellSize * 3, gh + cellSize, aura);

        painter.setBrush(QColor(0, 140, 255, 50 + int(45 * pulseVal)));
        painter.setPen(QPen(QColor(0, 255, 240, gateAlpha), std::max(2, cellSize / 9)));
        painter.drawRoundedRect(gx, gy, gw, gh, 5, 5);

        painter.setPen(QColor(255, 255, 255, gateAlpha));
        painter.setFont(QFont("Arial", std::max(10, cellSize / 2), QFont::Bold));
        painter.drawText(gx + cellSize * 0.12, gy + cellSize * 0.72, "◀");
        painter.drawText(gx + cellSize * 0.12, gy + cellSize * 1.62, "◀");

        painter.setFont(QFont("Arial", std::max(7, cellSize / 4), QFont::Bold));
        painter.setPen(QColor(0, 255, 240));
        bool isWestDiscovered = (sx - 1 >= 0 && sectorDiscovered[sy][sx - 1]);
        QString westLabel = isWestDiscovered ? QString("◀ SECTOR (%1,%2)").arg(sx).arg(sy + 1) : "◀ EXPLORE";
        painter.drawText(gx + gw + 4, gy + cellSize * 1.1, westLabel);
    }

    // South Gateway (Connects to sector sy + 1)
    if (sy < sectorRows - 1 && board[sy][sx][ROWS - 1][9] == 0) {
        int gx = originX + 9 * cellSize;
        int gy = originY + (ROWS - 1) * cellSize;
        int gw = 2 * cellSize;
        int gh = cellSize;

        QRadialGradient aura(gx + gw / 2, gy + gh, cellSize * 2.8);
        aura.setColorAt(0.0, QColor(0, 240, 255, 110));
        aura.setColorAt(0.4, QColor(0, 180, 255, 40));
        aura.setColorAt(1.0, QColor(0, 0, 0, 0));
        painter.fillRect(gx - cellSize / 2, gy - cellSize * 2, gw + cellSize, cellSize * 3, aura);

        painter.setBrush(QColor(0, 140, 255, 50 + int(45 * pulseVal)));
        painter.setPen(QPen(QColor(0, 255, 240, gateAlpha), std::max(2, cellSize / 9)));
        painter.drawRoundedRect(gx, gy, gw, gh, 5, 5);

        painter.setPen(QColor(255, 255, 255, gateAlpha));
        painter.setFont(QFont("Arial", std::max(10, cellSize / 2), QFont::Bold));
        painter.drawText(gx + cellSize * 0.35, gy + cellSize * 0.75, "▼");
        painter.drawText(gx + cellSize * 1.35, gy + cellSize * 0.75, "▼");

        painter.setFont(QFont("Arial", std::max(7, cellSize / 4), QFont::Bold));
        painter.setPen(QColor(0, 255, 240));
        bool isSouthDiscovered = (sy + 1 < sectorRows && sectorDiscovered[sy + 1][sx]);
        QString southLabel = isSouthDiscovered ? QString("▼ SECTOR (%1,%2) ▼").arg(sx + 1).arg(sy + 2) : "▼ EXPLORE ▼";
        painter.drawText(gx - 4, gy - 6, southLabel);
    }

    // North Gateway (Connects to sector sy - 1)
    if (sy > 0 && board[sy][sx][0][9] == 0) {
        int gx = originX + 9 * cellSize;
        int gy = originY;
        int gw = 2 * cellSize;
        int gh = cellSize;

        QRadialGradient aura(gx + gw / 2, gy, cellSize * 2.8);
        aura.setColorAt(0.0, QColor(0, 240, 255, 110));
        aura.setColorAt(0.4, QColor(0, 180, 255, 40));
        aura.setColorAt(1.0, QColor(0, 0, 0, 0));
        painter.fillRect(gx - cellSize / 2, gy, gw + cellSize, cellSize * 3, aura);

        painter.setBrush(QColor(0, 140, 255, 50 + int(45 * pulseVal)));
        painter.setPen(QPen(QColor(0, 255, 240, gateAlpha), std::max(2, cellSize / 9)));
        painter.drawRoundedRect(gx, gy, gw, gh, 5, 5);

        painter.setPen(QColor(255, 255, 255, gateAlpha));
        painter.setFont(QFont("Arial", std::max(10, cellSize / 2), QFont::Bold));
        painter.drawText(gx + cellSize * 0.35, gy + cellSize * 0.75, "▲");
        painter.drawText(gx + cellSize * 1.35, gy + cellSize * 0.75, "▲");

        painter.setFont(QFont("Arial", std::max(7, cellSize / 4), QFont::Bold));
        painter.setPen(QColor(0, 255, 240));
        bool isNorthDiscovered = (sy - 1 >= 0 && sectorDiscovered[sy - 1][sx]);
        QString northLabel = isNorthDiscovered ? QString("▲ SECTOR (%1,%2) ▲").arg(sx + 1).arg(sy) : "▲ EXPLORE ▲";
        painter.drawText(gx - 4, gy + gh + 14, northLabel);
    }

    // 7. Glowing EXIT Portal (Visible beacon shining through fog)
    if (sx == exitSectorX && sy == exitSectorY) {
        bool exitVisible = isCellVisible(sx, sy, exitX, exitY);
        bool exitExplored = explored[sy][sx][exitY][exitX];

        if (!fogOfWarEnabled || exitVisible || exitExplored) {
            int ex = originX + exitX * cellSize;
            int ey = originY + exitY * cellSize;

            float exitPulse = (std::sin(pulseTime * 1.2f) + 1.0f) * 0.5f;

            // Emerald aura
            QRadialGradient exitAura(ex + cellSize / 2, ey + cellSize / 2, cellSize * 2.2);
            exitAura.setColorAt(0.0, QColor(0, 255, 120, 130));
            exitAura.setColorAt(0.5, QColor(0, 200, 80, 40));
            exitAura.setColorAt(1.0, QColor(0, 0, 0, 0));
            painter.fillRect(ex - cellSize, ey - cellSize, cellSize * 3, cellSize * 3, exitAura);

            // Glowing portal box
            painter.setBrush(QColor(0, 255, 120, 70 + int(50 * exitPulse)));
            painter.setPen(QPen(QColor(0, 255, 140, 220), std::max(2, cellSize / 10)));
            painter.drawRoundedRect(ex + 2, ey + 2, cellSize - 4, cellSize - 4, 6, 6);

            // Inner core
            painter.setBrush(QColor(200, 255, 220, 180 + int(60 * exitPulse)));
            painter.setPen(Qt::NoPen);
            int coreSize = std::max(6, cellSize / 2);
            painter.drawEllipse(ex + cellSize/2 - coreSize/2, ey + cellSize/2 - coreSize/2, coreSize, coreSize);

            // Bold white text
            painter.setPen(QColor(255, 255, 255));
            painter.setFont(QFont("Arial", std::max(7, cellSize / 4), QFont::Bold));
            painter.drawText(ex, ey + cellSize * 0.72, cellSize, cellSize * 0.28, Qt::AlignCenter, "EXIT");
        }
    }
}

// ========================================================
// Game Loop
// ========================================================

void MainWindow::gameLoop()
{
    if (currentScreen == SCREEN_HOME_MAP) {
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

// ========================================================
// Dynamic Fullscreen Rendering with Multi-Window Pan & Map
// ========================================================

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (currentScreen == SCREEN_HOME_MAP) {
        drawCandyCrushMap(painter);
        return;
    }

    int hudHeight = 72;
    int availW = width();
    int availH = height() - hudHeight;

    int cellSize = std::min(availW / COLS, availH / ROWS);
    if (cellSize < 12) cellSize = 12;

    int mazeW = COLS * cellSize;
    int mazeH = ROWS * cellSize;
    int startX = (availW - mazeW) / 2;
    int startY = (availH - mazeH) / 2;

    // Fill background letterbox
    painter.fillRect(rect(), QColor(5, 5, 8));

    // Top-Left Saga Map Button
    QRect mapBtn = getMapButtonRect();
    bool mapBtnHovered = mapBtn.contains(mapFromGlobal(QCursor::pos()));
    painter.setBrush(mapBtnHovered ? QColor(25, 45, 90, 230) : QColor(14, 25, 55, 200));
    painter.setPen(QPen(mapBtnHovered ? QColor(0, 255, 240) : QColor(0, 180, 255), 1.5));
    painter.drawRoundedRect(mapBtn, 6, 6);

    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.setPen(mapBtnHovered ? Qt::white : QColor(220, 240, 255));
    painter.drawText(mapBtn, Qt::AlignCenter, "🗺️ SAGA MAP [M]");

    // Render Sector(s)
    if (!isTransitioning) {
        drawSector(painter, currentSectorX, currentSectorY, startX, startY, cellSize);
    } else {
        // Smooth camera slide animation between windows
        float t = std::min(1.0f, transitionTimer.elapsed() / (float)TRANSITION_DURATION_MS);
        float ease = 0.5f - 0.5f * std::cos(t * 3.14159265f);

        painter.save();
        painter.setClipRect(startX, startY, mazeW, mazeH);

        // Previous window sliding out
        int fromOriginX = startX - std::round(ease * transitionDirX * mazeW);
        int fromOriginY = startY - std::round(ease * transitionDirY * mazeH);
        drawSector(painter, fromSectorX, fromSectorY, fromOriginX, fromOriginY, cellSize);

        // New window sliding in
        int toOriginX = startX + std::round((1.0f - ease) * transitionDirX * mazeW);
        int toOriginY = startY + std::round((1.0f - ease) * transitionDirY * mazeH);
        drawSector(painter, toSectorX, toSectorY, toOriginX, toOriginY, cellSize);

        painter.restore();

        if (t >= 1.0f) {
            isTransitioning = false;
        }
    }

    // ========================================================
    // Toast Notification Banner when a new sector is revealed/unfolded
    // ========================================================
    if (newSectorJustDiscovered) {
        qint64 ms = newSectorTimer.elapsed();
        if (ms < 1800) {
            float alpha = 1.0f;
            if (ms > 1200) alpha = (1800 - ms) / 600.0f;
            int a = int(220 * alpha);

            int bannerW = std::min(420, mazeW - 20);
            int bannerH = 34;
            int bx = startX + (mazeW - bannerW) / 2;
            int by = startY + 16;

            painter.setBrush(QColor(8, 22, 50, a));
            painter.setPen(QPen(QColor(0, 240, 255, a), 2));
            painter.drawRoundedRect(bx, by, bannerW, bannerH, 8, 8);

            painter.setPen(QColor(255, 235, 100, int(255 * alpha)));
            painter.setFont(QFont("Arial", std::max(11, bannerH / 3), QFont::Bold));
            painter.drawText(bx, by, bannerW, bannerH, Qt::AlignCenter,
                             "✨ UNCHARTED SECTOR UNFOLDED! ✨");
        } else {
            newSectorJustDiscovered = false;
        }
    }

    // ========================================================
    // Dynamic Unfolding Sector Mini-Map (Top-Right HUD Corner)
    // ========================================================
    {
        // 1. Calculate bounding box of all discovered sectors
        int minSx = currentSectorX, maxSx = currentSectorX;
        int minSy = currentSectorY, maxSy = currentSectorY;
        int discCount = 0;

        for (int sy = 0; sy < sectorRows; ++sy) {
            for (int sx = 0; sx < sectorCols; ++sx) {
                if (sectorDiscovered[sy][sx]) {
                    discCount++;
                    minSx = std::min(minSx, sx);
                    maxSx = std::max(maxSx, sx);
                    minSy = std::min(minSy, sy);
                    maxSy = std::max(maxSy, sy);
                }
            }
        }

        int unfoldCols = maxSx - minSx + 1;
        int unfoldRows = maxSy - minSy + 1;
        int mapBoxSize = std::max(20, cellSize * 3 / 5);
        int mapGap = 5;
        int totalMapW = unfoldCols * mapBoxSize + (unfoldCols - 1) * mapGap;
        int totalMapH = unfoldRows * mapBoxSize + (unfoldRows - 1) * mapGap;
        int mapX = startX + mazeW - totalMapW - 16;
        int mapY = startY + 16;

        // Map container frame
        painter.fillRect(mapX - 8, mapY - 20, totalMapW + 16, totalMapH + 28, QColor(6, 10, 20, 215));
        painter.setPen(QPen(QColor(0, 180, 255, 120), 1));
        painter.drawRoundedRect(mapX - 8, mapY - 20, totalMapW + 16, totalMapH + 28, 6, 6);

        // Map title: strictly reveals ONLY discovered count, no totals!
        painter.setFont(QFont("Arial", std::max(7, mapBoxSize / 3), QFont::Bold));
        painter.setPen(QColor(0, 220, 255));
        QString mapTitle = (discCount == 1) ? "MAP (1 SECTOR)" : QString("MAP (%1 SECTORS)").arg(discCount);
        painter.drawText(mapX - 2, mapY - 6, mapTitle);

        // Render ONLY discovered sectors as they unfold
        for (int sy = minSy; sy <= maxSy; ++sy) {
            for (int sx = minSx; sx <= maxSx; ++sx) {
                if (!sectorDiscovered[sy][sx]) continue;

                int rx = mapX + (sx - minSx) * (mapBoxSize + mapGap);
                int ry = mapY + (sy - minSy) * (mapBoxSize + mapGap);

                // Sector base tile
                painter.fillRect(rx, ry, mapBoxSize, mapBoxSize, QColor(18, 42, 85));
                painter.setPen(QPen(QColor(50, 130, 220), 1));
                painter.drawRect(rx, ry, mapBoxSize, mapBoxSize);

                // Mark EXIT only if this discovered sector contains the exit
                if (sx == exitSectorX && sy == exitSectorY) {
                    painter.setPen(QColor(0, 255, 120));
                    painter.setFont(QFont("Arial", std::max(7, mapBoxSize / 3), QFont::Bold));
                    painter.drawText(rx, ry, mapBoxSize, mapBoxSize, Qt::AlignCenter, "EXIT");
                } else {
                    // Small sector coordinate label
                    painter.setFont(QFont("Arial", std::max(6, mapBoxSize / 4)));
                    painter.setPen(QColor(130, 180, 240, 160));
                    painter.drawText(rx + 2, ry + mapBoxSize - 3, QString("%1,%2").arg(sx + 1).arg(sy + 1));
                }

                // Current window cursor (Golden pulsing outline + Pacman dot)
                if (sx == currentSectorX && sy == currentSectorY) {
                    painter.setPen(QPen(QColor(255, 220, 0), 2));
                    painter.drawRect(rx - 1, ry - 1, mapBoxSize + 2, mapBoxSize + 2);

                    painter.setBrush(QColor(255, 220, 0));
                    painter.setPen(Qt::NoPen);
                    painter.drawEllipse(rx + mapBoxSize/2 - 2, ry + mapBoxSize/2 - 2, 4, 4);

                    // Show small pulsing exploration chevrons on edges that lead to UNCHARTED sectors!
                    float chevPulse = (std::sin(elapsedTimer.elapsed() * 0.008f) + 1.0f) * 0.5f;
                    int chevAlpha = 140 + int(115 * chevPulse);
                    painter.setPen(QColor(0, 255, 240, chevAlpha));
                    painter.setFont(QFont("Arial", std::max(7, mapBoxSize / 3), QFont::Bold));

                    if (sx + 1 < sectorCols && !sectorDiscovered[sy][sx + 1]) {
                        painter.drawText(rx + mapBoxSize - 1, ry + mapBoxSize * 3 / 4, "▶");
                    }
                    if (sx - 1 >= 0 && !sectorDiscovered[sy][sx - 1]) {
                        painter.drawText(rx - 7, ry + mapBoxSize * 3 / 4, "◀");
                    }
                    if (sy + 1 < sectorRows && !sectorDiscovered[sy + 1][sx]) {
                        painter.drawText(rx + mapBoxSize / 3, ry + mapBoxSize + 7, "▼");
                    }
                    if (sy - 1 >= 0 && !sectorDiscovered[sy - 1][sx]) {
                        painter.drawText(rx + mapBoxSize / 3, ry - 1, "▲");
                    }
                }

                // Pathway bridge connectors between adjacent DISCOVERED sectors
                if (sx < maxSx && sectorDiscovered[sy][sx + 1]) {
                    painter.setPen(QPen(QColor(0, 230, 255, 200), 2));
                    int cy = ry + mapBoxSize / 2;
                    painter.drawLine(rx + mapBoxSize, cy, rx + mapBoxSize + mapGap, cy);
                }
                if (sy < maxSy && sectorDiscovered[sy + 1][sx]) {
                    painter.setPen(QPen(QColor(0, 230, 255, 200), 2));
                    int cx = rx + mapBoxSize / 2;
                    painter.drawLine(cx, ry + mapBoxSize, cx, ry + mapBoxSize + mapGap);
                }
            }
        }
    }

    // ========================================================
    // Bottom Fullscreen HUD Bar (3-Line Command Center)
    // ========================================================
    int hudY = height() - hudHeight;
    painter.fillRect(0, hudY, width(), hudHeight, QColor(14, 16, 26));
    painter.setPen(QPen(QColor(40, 60, 100), 2));
    painter.drawLine(0, hudY, width(), hudY);

    int baseFont1 = std::max(11, width() / 110);
    int baseFont2 = std::max(9, width() / 135);

    // Count discovered sectors for HUD
    int discCount = 0;
    for (int sy = 0; sy < sectorRows; ++sy) {
        for (int sx = 0; sx < sectorCols; ++sx) {
            if (sectorDiscovered[sy][sx]) discCount++;
        }
    }

    // Line 1: Stats & Window location (hudY + 20)
    painter.setFont(QFont("Arial", baseFont1, QFont::Bold));
    if (!isMultiplayer) {
        painter.setPen(Qt::white);
        painter.drawText(20, hudY + 20, QString("SCORE: %1").arg(score));

        painter.setPen(QColor(0, 230, 160));
        painter.drawText(width() * 0.16, hudY + 20, QString("LEVEL: %1").arg(currentLevel));

        painter.setPen(QColor(0, 210, 255));
        painter.drawText(width() * 0.28, hudY + 20,
                         QString("SECTOR: (%1, %2)").arg(currentSectorX + 1).arg(currentSectorY + 1));

        painter.setPen(QColor(255, 180, 0));
        painter.drawText(width() * 0.42, hudY + 20,
                         QString("EXPLORED: %1").arg(discCount));

        painter.setPen(QColor(180, 200, 255));
        painter.drawText(width() * 0.54, hudY + 20, QString("MOVES: %1").arg(moveCount));
    } else {
        // P1 Status (Yellow)
        painter.setPen(p1Alive ? QColor(255, 230, 30) : QColor(160, 160, 80));
        QString p1Str = p1Alive ? QString("P1 (🟡): %1").arg(scoreP1)
                                : QString("P1: RESPAWN (%1s)").arg((p1RespawnTicks * 130 + 999) / 1000);
        painter.drawText(20, hudY + 20, p1Str);

        // P2 Status (Pink)
        painter.setPen(p2Alive ? QColor(255, 120, 190) : QColor(160, 80, 120));
        QString p2Str = p2Alive ? QString("P2 (🎀): %1").arg(scoreP2)
                                : QString("P2: RESPAWN (%1s)").arg((p2RespawnTicks * 130 + 999) / 1000);
        painter.drawText(width() * 0.18, hudY + 20, p2Str);

        // Team Total Score
        painter.setPen(Qt::white);
        painter.drawText(width() * 0.36, hudY + 20, QString("TEAM: %1").arg(score));

        painter.setPen(QColor(0, 230, 160));
        painter.drawText(width() * 0.48, hudY + 20, QString("LEVEL: %1").arg(currentLevel));

        painter.setPen(QColor(0, 210, 255));
        painter.drawText(width() * 0.58, hudY + 20,
                         QString("SECTOR: (%1, %2)").arg(currentSectorX + 1).arg(currentSectorY + 1));
    }

    qint64 elapsedSec = elapsedTimer.elapsed() / 1000;
    int mins = elapsedSec / 60;
    int secs = elapsedSec % 60;
    painter.setPen(QColor(255, 200, 100));
    painter.drawText(width() * 0.65, hudY + 20, QString("TIME: %1:%2")
                     .arg(mins, 2, 10, QChar('0'))
                     .arg(secs, 2, 10, QChar('0')));

    if (fogPeekActive) {
        int msLeft = std::max(0, FOG_PEEK_DURATION_MS - (int)fogPeekTimer.elapsed());
        double secLeft = msLeft / 1000.0;
        painter.setPen(QColor(255, 215, 0));
        painter.drawText(width() * 0.77, hudY + 20, QString("[PEEK: %1s ☀️]").arg(secLeft, 0, 'f', 1));
    } else if (fogOfWarEnabled) {
        painter.setPen(QColor(120, 220, 255));
        painter.drawText(width() * 0.77, hudY + 20, "[FOG: ON 🌑 (F)]");
    } else {
        painter.setPen(QColor(150, 150, 160));
        painter.drawText(width() * 0.77, hudY + 20, "[FOG: OFF ☀️]");
    }

    if (ghostsFrightened) {
        painter.setPen(QColor(255, 100, 140));
        painter.drawText(width() * 0.89, hudY + 20, QString("⚡ POWER! %1").arg(frightenedTicksLeft));
    }

    // Line 2: Prominent Mission Objective Guidance (hudY + 42)
    QString objectiveText;
    if (currentSectorX == exitSectorX && currentSectorY == exitSectorY) {
        objectiveText = QString("🏁 OBJECTIVE: Find and enter the green [EXIT] portal in this window to complete Level %1!").arg(currentLevel);
        painter.setPen(QColor(60, 255, 140));
    } else {
        QString dirStr;
        if (currentSectorX < exitSectorX) dirStr = "RIGHT wall [▶ GATEWAY]";
        else if (currentSectorX > exitSectorX) dirStr = "LEFT wall [◀ GATEWAY]";
        else if (currentSectorY < exitSectorY) dirStr = "BOTTOM wall [▼ GATEWAY]";
        else if (currentSectorY > exitSectorY) dirStr = "TOP wall [▲ GATEWAY]";

        objectiveText = QString("🧭 OBJECTIVE: Venture through the glowing %1 into uncharted territory!").arg(dirStr);
        painter.setPen(QColor(255, 220, 80));
    }
    painter.setFont(QFont("Arial", baseFont1, QFont::Bold));
    painter.drawText(20, hudY + 42, objectiveText);

    // Line 3: Controls (hudY + 62)
    painter.setFont(QFont("Arial", baseFont2));
    painter.setPen(QColor(140, 160, 190));
    if (!isMultiplayer) {
        if (singlePlayerHero == 2) {
            painter.drawText(20, hudY + 62,
                "HERO: [🎀 Pink Ms. Pacman]  |  [Arrows] or [WASD] Move  |  [Tab] Switch to Yellow  |  [F] Fog Peek  |  [M] Saga Map  |  [F11] Fullscreen");
        } else {
            painter.drawText(20, hudY + 62,
                "HERO: [🟡 Yellow Pacman]  |  [Arrows] or [WASD] Move  |  [Tab] Switch to Pink  |  [F] Fog Peek  |  [M] Saga Map  |  [F11] Fullscreen");
        }
    } else {
        if (arrowPlayer == 2) {
            painter.drawText(20, hudY + 62,
                "ARROWS STEER: [🎀 Pink P2]  |  WASD: [🟡 Yellow P1]  |  [Tab] Swap Arrows to Yellow!  |  [F] Fog Peek  |  [M] Saga Map");
        } else {
            painter.drawText(20, hudY + 62,
                "ARROWS STEER: [🟡 Yellow P1]  |  WASD: [🎀 Pink P2]  |  [Tab] Swap Arrows to Pink!  |  [F] Fog Peek  |  [M] Saga Map");
        }
    }

    // Game Over / Level Complete Banner
    if (gameOver) {
        painter.fillRect(0, height() / 2 - 45, width(), 95, QColor(0, 0, 0, 235));
        painter.setPen(QColor(255, 60, 60));
        painter.setFont(QFont("Arial", std::max(18, width() / 50), QFont::Bold));
        QString goMsg = isMultiplayer
            ? QString("GAME OVER! Both Players Defeated (Level %1)").arg(currentLevel)
            : QString("GAME OVER! (Level %1)").arg(currentLevel);
        painter.drawText(0, height() / 2 - 18, width(), 30, Qt::AlignCenter, goMsg);

        painter.setPen(QColor(255, 220, 100));
        painter.setFont(QFont("Arial", std::max(12, width() / 70)));
        painter.drawText(0, height() / 2 + 16, width(), 25, Qt::AlignCenter,
                         QString("Press [R] or [Space] to Retry Level %1  |  Press [M] for Saga Map").arg(currentLevel));
    } else if (levelComplete) {
        painter.fillRect(0, height() / 2 - 50, width(), 100, QColor(0, 0, 0, 235));
        painter.setPen(QColor(60, 255, 120));
        painter.setFont(QFont("Arial", std::max(16, width() / 55), QFont::Bold));
        QString levelMsg = isMultiplayer
            ? QString("LEVEL %1 COMPLETE! Team Escaped the Labyrinth!").arg(currentLevel)
            : QString("LEVEL %1 COMPLETE! Labyrinth Escaped!").arg(currentLevel);
        painter.drawText(0, height() / 2 - 18, width(), 30, Qt::AlignCenter, levelMsg);

        painter.setPen(QColor(255, 220, 100));
        painter.setFont(QFont("Arial", std::max(12, width() / 70)));
        painter.drawText(0, height() / 2 + 16, width(), 25, Qt::AlignCenter,
                         "Press [Space] for Next Level  |  Press [M] for Saga Map");
    }
}

// ========================================================
// Mouse Interaction & Candy Crush Saga Level Map
// ========================================================

QString MainWindow::getLevelTitle(int lvl) const
{
    switch (lvl) {
        case 1: return "Level 1: Dual Chamber (2 Windows)";
        case 2: return "Level 2: Quad Labyrinth (4 Windows)";
        case 3: return "Level 3: Twin Hallways (6 Windows)";
        case 4: return "Level 4: Vertical Depths (6 Windows)";
        case 5: return "Level 5: Nine Realms (9 Windows)";
        case 6: return "Level 6: Ghost Citadel (9 Windows)";
        case 7: return "Level 7: Nightmare Labyrinth (9 Windows)";
        case 8: return "Level 8: The Grand Master Vault (9 Windows)";
        default: return QString("Level %1").arg(lvl);
    }
}

QRect MainWindow::getPlayButtonRect() const
{
    int btnW = std::min(320, int(width() * 0.45));
    int btnH = std::max(42, int(height() * 0.054));
    if (btnH > 52) btnH = 52;
    int btnX = (width() - btnW) / 2;
    int btnY = height() - btnH - 12;
    return QRect(btnX, btnY, btnW, btnH);
}

QRect MainWindow::getModeSelectorRect() const
{
    int selW = std::min(460, int(width() * 0.62));
    int selH = std::max(36, int(height() * 0.046));
    if (selH > 42) selH = 42;
    int selX = (width() - selW) / 2;
    int selY = getPlayButtonRect().top() - selH - 8;
    return QRect(selX, selY, selW, selH);
}

QRect MainWindow::getModeYellowRect() const
{
    QRect total = getModeSelectorRect();
    int w3 = total.width() / 3;
    return QRect(total.left(), total.top(), w3, total.height());
}

QRect MainWindow::getModePinkRect() const
{
    QRect total = getModeSelectorRect();
    int w3 = total.width() / 3;
    return QRect(total.left() + w3, total.top(), w3, total.height());
}

QRect MainWindow::getModeCoopRect() const
{
    QRect total = getModeSelectorRect();
    int w3 = total.width() / 3;
    return QRect(total.left() + 2 * w3, total.top(), total.width() - 2 * w3, total.height());
}

QRect MainWindow::getMapButtonRect() const
{
    return QRect(16, 14, 115, 32);
}

QPoint MainWindow::getLevelNodePos(int lvl, int mapW, int mapH) const
{
    static const double coords[8][2] = {
        {0.48, 0.81}, // Level 1 (bottom center)
        {0.68, 0.72}, // Level 2 (lower right)
        {0.72, 0.58}, // Level 3 (mid right)
        {0.50, 0.50}, // Level 4 (center)
        {0.28, 0.42}, // Level 5 (mid left)
        {0.32, 0.28}, // Level 6 (upper left)
        {0.52, 0.20}, // Level 7 (upper center)
        {0.65, 0.12}  // Level 8 (top crown peak)
    };
    int idx = std::max(1, std::min(8, lvl)) - 1;
    return QPoint(int(coords[idx][0] * mapW), int(coords[idx][1] * mapH));
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (currentScreen == SCREEN_HOME_MAP) {
        QPoint pos = event->pos();

        // 1. Clicked Mode Selector buttons
        if (getModeYellowRect().contains(pos)) {
            isMultiplayer = false;
            singlePlayerHero = 1;
            arrowPlayer = 1;
            update();
            return;
        }
        if (getModePinkRect().contains(pos)) {
            isMultiplayer = false;
            singlePlayerHero = 2;
            arrowPlayer = 2;
            update();
            return;
        }
        if (getModeCoopRect().contains(pos)) {
            isMultiplayer = true;
            update();
            return;
        }

        // 2. Clicked Play button
        if (getPlayButtonRect().contains(pos)) {
            startLevel(selectedMapLevel);
            return;
        }

        // 3. Clicked Level Node
        int radius = std::min(width(), height()) / 22;
        if (radius < 24) radius = 24;
        if (radius > 36) radius = 36;

        for (int lvl = 1; lvl <= TOTAL_LEVELS; ++lvl) {
            QPoint np = getLevelNodePos(lvl, width(), height());
            int dx = pos.x() - np.x();
            int dy = pos.y() - np.y();
            if (dx * dx + dy * dy <= (radius + 8) * (radius + 8)) {
                if (lvl <= highestUnlockedLevel) {
                    selectedMapLevel = lvl;
                    startLevel(lvl);
                    return;
                }
            }
        }
        update();
        return;
    }

    if (currentScreen == SCREEN_PLAYING) {
        QPoint pos = event->pos();

        // Clicked Saga Map button in HUD
        if (getMapButtonRect().contains(pos)) {
            currentScreen = SCREEN_HOME_MAP;
            selectedMapLevel = currentLevel;
            update();
            return;
        }

        // Click on Pacman to toggle arrow control!
        int hudHeight = 72;
        int availW = width();
        int availH = height() - hudHeight;
        int cellSize = std::min(availW / COLS, availH / ROWS);
        int originX = (availW - COLS * cellSize) / 2;
        int originY = (availH - ROWS * cellSize) / 2;

        int p1CenterX = originX + pacmanX * cellSize + cellSize / 2;
        int p1CenterY = originY + pacmanY * cellSize + cellSize / 2;
        int p2CenterX = originX + p2X * cellSize + cellSize / 2;
        int p2CenterY = originY + p2Y * cellSize + cellSize / 2;

        if (std::hypot(pos.x() - p1CenterX, pos.y() - p1CenterY) <= cellSize * 1.5) {
            arrowPlayer = 1;
            if (!isMultiplayer) singlePlayerHero = 1;
            update();
            return;
        }
        if (isMultiplayer && std::hypot(pos.x() - p2CenterX, pos.y() - p2CenterY) <= cellSize * 1.5) {
            arrowPlayer = 2;
            update();
            return;
        }

        if (gameOver) {
            initLevel();
            gameOver = false;
            update();
            return;
        }

        if (levelComplete) {
            currentLevel++;
            if (currentLevel > TOTAL_LEVELS) currentLevel = TOTAL_LEVELS;
            if (currentLevel > highestUnlockedLevel) highestUnlockedLevel = currentLevel;
            selectedMapLevel = currentLevel;
            initLevel();
            levelComplete = false;
            update();
            return;
        }
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (currentScreen == SCREEN_HOME_MAP) {
        QPoint pos = event->pos();
        hoveredMapLevel = 0;

        int radius = std::min(width(), height()) / 22;
        if (radius < 24) radius = 24;
        if (radius > 36) radius = 36;

        for (int lvl = 1; lvl <= TOTAL_LEVELS; ++lvl) {
            QPoint np = getLevelNodePos(lvl, width(), height());
            int dx = pos.x() - np.x();
            int dy = pos.y() - np.y();
            if (dx * dx + dy * dy <= (radius + 8) * (radius + 8)) {
                hoveredMapLevel = lvl;
                break;
            }
        }

        bool onMode = getModeSelectorRect().contains(pos);
        if ((hoveredMapLevel != 0 && hoveredMapLevel <= highestUnlockedLevel) ||
            getPlayButtonRect().contains(pos) || onMode) {
            setCursor(Qt::PointingHandCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }

        update();
    } else {
        if (getMapButtonRect().contains(event->pos())) {
            setCursor(Qt::PointingHandCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
    }
}

void MainWindow::drawPacmanAvatar(QPainter &painter, int cx, int cy, int size)
{
    painter.save();
    int r = size / 2;

    // Golden glow behind Pacman
    float pulse = (std::sin(elapsedTimer.elapsed() * 0.009f) + 1.0f) * 0.5f;
    QRadialGradient glow(cx, cy, r * 1.8);
    glow.setColorAt(0.0, QColor(255, 230, 80, 130 + int(70 * pulse)));
    glow.setColorAt(0.6, QColor(255, 200, 30, 50));
    glow.setColorAt(1.0, QColor(0, 0, 0, 0));
    painter.fillRect(cx - r * 2, cy - r * 2, r * 4, r * 4, glow);

    // Pacman body with animated mouth
    int startAngle = int(35 * std::abs(std::sin(elapsedTimer.elapsed() * 0.008f)));
    int spanAngle = 360 - 2 * startAngle;

    painter.setBrush(QColor(255, 225, 20));
    painter.setPen(QPen(QColor(220, 160, 0), 2));
    painter.drawPie(cx - r, cy - r, size, size, startAngle * 16, spanAngle * 16);

    // Eye
    painter.setBrush(QColor(30, 30, 30));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(cx + r / 5, cy - r / 2, std::max(3, r / 4), std::max(4, r / 3));

    // Specular shine
    painter.setBrush(QColor(255, 255, 255, 180));
    painter.drawEllipse(cx - r / 3, cy - r / 2, std::max(3, r / 3), std::max(2, r / 4));

    // Little banner badge above Pacman ("P1" in multiplayer, "YOU" in solo)
    painter.setFont(QFont("Arial", std::max(7, size / 4), QFont::Bold));
    painter.setBrush(QColor(255, 50, 90));
    painter.setPen(QPen(QColor(255, 220, 220), 1));
    int bW = std::max(36, size + 12);
    int bH = 16;
    painter.drawRoundedRect(cx - bW / 2, cy - r - bH - 3, bW, bH, 4, 4);

    painter.setPen(Qt::white);
    painter.drawText(cx - bW / 2, cy - r - bH - 3, bW, bH, Qt::AlignCenter, isMultiplayer ? "P1" : "YOU");

    painter.restore();
}

void MainWindow::drawMsPacmanAvatar(QPainter &painter, int cx, int cy, int size)
{
    painter.save();
    int r = size / 2;

    // Pink glow behind Ms. Pacman
    float pulse = (std::sin(elapsedTimer.elapsed() * 0.009f + 1.6f) + 1.0f) * 0.5f;
    QRadialGradient glow(cx, cy, r * 1.8);
    glow.setColorAt(0.0, QColor(255, 100, 180, 130 + int(70 * pulse)));
    glow.setColorAt(0.6, QColor(255, 50, 140, 50));
    glow.setColorAt(1.0, QColor(0, 0, 0, 0));
    painter.fillRect(cx - r * 2, cy - r * 2, r * 4, r * 4, glow);

    // Coral Pink body with animated mouth
    int startAngle = int(35 * std::abs(std::sin(elapsedTimer.elapsed() * 0.008f + 1.2f)));
    int spanAngle = 360 - 2 * startAngle;

    painter.setBrush(QColor(255, 105, 180));
    painter.setPen(QPen(QColor(220, 50, 120), 2));
    painter.drawPie(cx - r, cy - r, size, size, (180 + startAngle) * 16, spanAngle * 16);

    // Eye
    painter.setBrush(QColor(20, 20, 50));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(cx - r / 5, cy - r / 2, std::max(3, r / 4), std::max(4, r / 3));

    // Specular shine
    painter.setBrush(QColor(255, 255, 255, 180));
    painter.drawEllipse(cx - r / 3, cy - r / 2, std::max(2, r / 4), std::max(2, r / 4));

    // Iconic Red Ribbon Bow on head
    int bowSize = std::max(6, r * 3 / 5);
    int bx = cx + r / 4;
    int by = cy - r;
    painter.setBrush(QColor(235, 25, 60));
    painter.setPen(QPen(QColor(180, 10, 40), 1));
    painter.drawEllipse(bx - bowSize, by, bowSize * 3 / 4, bowSize / 2);
    painter.drawEllipse(bx, by, bowSize * 3 / 4, bowSize / 2);
    painter.setBrush(QColor(255, 210, 220));
    painter.drawEllipse(bx - bowSize / 5, by + bowSize / 8, bowSize * 2 / 5, bowSize * 2 / 5);

    // Little "P2" banner badge above Ms. Pacman
    painter.setFont(QFont("Arial", std::max(7, size / 4), QFont::Bold));
    painter.setBrush(QColor(230, 40, 120));
    painter.setPen(QPen(QColor(255, 220, 240), 1));
    int bW = std::max(32, size);
    int bH = 16;
    painter.drawRoundedRect(cx - bW / 2, cy - r - bH - 3, bW, bH, 4, 4);

    painter.setPen(Qt::white);
    painter.drawText(cx - bW / 2, cy - r - bH - 3, bW, bH, Qt::AlignCenter, "P2");

    painter.restore();
}

void MainWindow::drawLevelNode(QPainter &painter, int lvl, int cx, int cy, int radius)
{
    painter.save();
    bool isCompleted = (lvl < highestUnlockedLevel);
    bool isCurrent = (lvl == highestUnlockedLevel);
    bool isSelected = (lvl == selectedMapLevel);
    bool isLocked = (lvl > highestUnlockedLevel);
    bool isHovered = (lvl == hoveredMapLevel);

    int r = radius;
    if (isHovered && !isLocked) r += 3;

    // 1. Aura / Pulsing ring for current/selected node
    if (isCurrent || isSelected) {
        float pulse = (std::sin(elapsedTimer.elapsed() * 0.007f) + 1.0f) * 0.5f;
        int ringR = r + 8 + int(8 * pulse);
        QRadialGradient ringGrad(cx, cy, ringR);
        ringGrad.setColorAt(0.0, QColor(0, 230, 255, 120));
        ringGrad.setColorAt(0.7, QColor(0, 180, 255, 60));
        ringGrad.setColorAt(1.0, QColor(0, 0, 0, 0));
        painter.fillRect(cx - ringR, cy - ringR, ringR * 2, ringR * 2, ringGrad);

        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(0, 240, 255, 160 + int(95 * pulse)), 3, Qt::DashLine));
        painter.drawEllipse(cx - ringR + 3, cy - ringR + 3, (ringR - 3) * 2, (ringR - 3) * 2);
    }

    // 2. Drop shadow
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 120));
    painter.drawEllipse(cx - r + 3, cy - r + 5, r * 2, r * 2);

    // 3. 3D Spherical Candy Button
    QRadialGradient grad(cx - r / 3, cy - r / 3, r * 1.3);
    if (isCompleted) {
        // Candy Emerald
        grad.setColorAt(0.0, QColor(110, 255, 150));
        grad.setColorAt(0.4, QColor(35, 200, 75));
        grad.setColorAt(0.85, QColor(15, 130, 45));
        grad.setColorAt(1.0, QColor(8, 70, 25));
        painter.setBrush(grad);
        painter.setPen(QPen(QColor(180, 255, 200), 2));
    } else if (isCurrent || isSelected) {
        // Candy Cyan / Royal Blue
        grad.setColorAt(0.0, QColor(140, 240, 255));
        grad.setColorAt(0.4, QColor(0, 160, 245));
        grad.setColorAt(0.85, QColor(0, 90, 190));
        grad.setColorAt(1.0, QColor(0, 40, 110));
        painter.setBrush(grad);
        painter.setPen(QPen(QColor(200, 245, 255), 2.5));
    } else {
        // Frosted Slate / Locked
        grad.setColorAt(0.0, QColor(85, 75, 105));
        grad.setColorAt(0.6, QColor(48, 40, 65));
        grad.setColorAt(1.0, QColor(25, 20, 36));
        painter.setBrush(grad);
        painter.setPen(QPen(QColor(70, 60, 85), 1.5));
    }
    painter.drawEllipse(cx - r, cy - r, r * 2, r * 2);

    // 4. Glossy Specular Highlight on top
    painter.setBrush(QColor(255, 255, 255, 75));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(cx - r * 0.5, cy - r * 0.75, int(r * 1.0), int(r * 0.55));

    // 5. Node Contents: Number or Padlock
    if (isLocked) {
        int lW = r * 0.6;
        int lH = r * 0.5;
        int lx = cx - lW / 2;
        int ly = cy - lH / 4;

        // Shackle
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(200, 190, 160), 2.5));
        painter.drawArc(lx + lW / 4, ly - lH / 2, lW / 2, lH, 0, 180 * 16);

        // Body
        painter.setBrush(QColor(180, 150, 60));
        painter.setPen(QPen(QColor(230, 200, 100), 1));
        painter.drawRoundedRect(lx, ly, lW, lH, 3, 3);
    } else {
        // Bold Level Number
        painter.setFont(QFont("Arial", std::max(12, r * 3 / 4), QFont::Bold));
        painter.setPen(QColor(0, 0, 0, 120));
        painter.drawText(cx - r + 1, cy - r + 2, r * 2, r * 2, Qt::AlignCenter, QString::number(lvl));
        painter.setPen(Qt::white);
        painter.drawText(cx - r, cy - r, r * 2, r * 2, Qt::AlignCenter, QString::number(lvl));

        // Golden Stars for completed levels
        if (isCompleted) {
            painter.setFont(QFont("Arial", std::max(8, r / 3)));
            painter.setPen(QColor(255, 215, 0));
            painter.drawText(cx - r, cy + r - 3, r * 2, 16, Qt::AlignCenter, "⭐⭐⭐");
        }
    }

    // 6. Level 8 Crown Trophy
    if (lvl == TOTAL_LEVELS) {
        int cw = 28, ch = 18;
        int cx0 = cx - cw / 2;
        int cy0 = cy - r - ch - 4;
        painter.setBrush(QColor(255, 210, 30));
        painter.setPen(QPen(QColor(255, 240, 120), 1));
        QPolygon crown;
        crown << QPoint(cx0, cy0 + ch)
              << QPoint(cx0 + cw, cy0 + ch)
              << QPoint(cx0 + cw, cy0 + ch / 3)
              << QPoint(cx0 + cw * 3 / 4, cy0 + ch * 2 / 3)
              << QPoint(cx0 + cw / 2, cy0)
              << QPoint(cx0 + cw / 4, cy0 + ch * 2 / 3)
              << QPoint(cx0, cy0 + ch / 3);
        painter.drawPolygon(crown);
    }

    // 7. Pacman Avatar on the Current Level
    if (lvl == highestUnlockedLevel) {
        int bob = int(4 * std::sin(elapsedTimer.elapsed() * 0.007f));
        if (isMultiplayer) {
            drawPacmanAvatar(painter, cx - 18, cy - r - 22 + bob, std::max(20, r - 3));
            drawMsPacmanAvatar(painter, cx + 18, cy - r - 22 - bob, std::max(20, r - 3));
        } else if (singlePlayerHero == 2) {
            drawMsPacmanAvatar(painter, cx, cy - r - 22 + bob, std::max(22, r));
        } else {
            drawPacmanAvatar(painter, cx, cy - r - 22 + bob, std::max(22, r));
        }
    }

    painter.restore();
}

void MainWindow::drawCandyCrushMap(QPainter &painter)
{
    int w = width();
    int h = height();

    // 1. Rich Candy Twilight Fantasy Gradient Background
    QLinearGradient bg(0, 0, 0, h);
    bg.setColorAt(0.0, QColor(20, 10, 42));   // Cosmic night
    bg.setColorAt(0.35, QColor(48, 16, 70));  // Deep plum
    bg.setColorAt(0.70, QColor(90, 24, 85));  // Warm raspberry
    bg.setColorAt(1.0, QColor(36, 12, 50));   // Dark violet
    painter.fillRect(rect(), bg);

    // Gentle candy hills rolling in bottom background
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(60, 16, 68, 140));
    QPainterPath hill1;
    hill1.moveTo(0, h * 0.85);
    hill1.quadTo(w * 0.35, h * 0.72, w * 0.7, h * 0.88);
    hill1.quadTo(w * 0.88, h * 0.94, w, h * 0.86);
    hill1.lineTo(w, h);
    hill1.lineTo(0, h);
    painter.drawPath(hill1);

    painter.setBrush(QColor(42, 12, 54, 180));
    QPainterPath hill2;
    hill2.moveTo(0, h * 0.90);
    hill2.quadTo(w * 0.45, h * 0.82, w, h * 0.92);
    hill2.lineTo(w, h);
    hill2.lineTo(0, h);
    painter.drawPath(hill2);

    // Twinkling stars / sugar sprinkles
    painter.setPen(Qt::NoPen);
    static const int starCoords[14][2] = {
        {12, 18}, {28, 10}, {84, 14}, {92, 26}, {15, 45}, {88, 52},
        {8, 70}, {94, 68}, {45, 12}, {62, 38}, {22, 60}, {76, 80},
        {50, 70}, {38, 90}
    };
    for (int i = 0; i < 14; ++i) {
        int sx = int(starCoords[i][0] * 0.01 * w);
        int sy = int(starCoords[i][1] * 0.01 * h);
        float pulse = (std::sin(elapsedTimer.elapsed() * 0.005f + i * 1.3f) + 1.0f) * 0.5f;
        int starA = 80 + int(140 * pulse);
        painter.setBrush(QColor(255, 240, 160, starA));
        int starD = 3 + (i % 3);
        painter.drawEllipse(sx, sy, starD, starD);
    }

    // 2. The Winding Serpentine Candy Road
    QPainterPath roadPath;
    roadPath.moveTo(getLevelNodePos(1, w, h));
    for (int lvl = 2; lvl <= TOTAL_LEVELS; ++lvl) {
        QPoint p0 = getLevelNodePos(lvl - 1, w, h);
        QPoint p1 = getLevelNodePos(lvl, w, h);
        QPoint cp1(p0.x(), (p0.y() + p1.y()) / 2);
        QPoint cp2(p1.x(), (p0.y() + p1.y()) / 2);
        roadPath.cubicTo(cp1, cp2, p1);
    }

    // Layer 1: Road Glow Aura
    painter.strokePath(roadPath, QPen(QColor(255, 110, 190, 45), 32, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    // Layer 2: Road Bed (Chocolate / Caramel)
    painter.strokePath(roadPath, QPen(QColor(135, 60, 25, 230), 20, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    // Layer 3: Dashed Yellow Sugar Road Line
    painter.strokePath(roadPath, QPen(QColor(255, 230, 110, 240), 5, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));

    // Decorative Power Dots along the road
    for (int lvl = 1; lvl < TOTAL_LEVELS; ++lvl) {
        QPoint p0 = getLevelNodePos(lvl, w, h);
        QPoint p1 = getLevelNodePos(lvl + 1, w, h);
        for (int step = 1; step <= 3; ++step) {
            double t = step / 4.0;
            double it = 1.0 - t;
            double cy0 = (p0.y() + p1.y()) / 2.0;
            double bx = it * it * it * p0.x() + 3 * it * it * t * p0.x() + 3 * it * t * t * p1.x() + t * t * t * p1.x();
            double by = it * it * it * p0.y() + 3 * it * it * t * cy0 + 3 * it * t * t * cy0 + t * t * t * p1.y();

            painter.setBrush(QColor(255, 230, 160));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(int(bx) - 3, int(by) - 3, 6, 6);
        }
    }

    // 3. The Level Nodes
    int radius = std::min(w, h) / 22;
    if (radius < 24) radius = 24;
    if (radius > 36) radius = 36;

    for (int lvl = 1; lvl <= TOTAL_LEVELS; ++lvl) {
        QPoint np = getLevelNodePos(lvl, w, h);
        drawLevelNode(painter, lvl, np.x(), np.y(), radius);
    }

    // 4. Header Ribbon Banner
    int headerW = std::min(580, int(w * 0.8));
    int headerH = 64;
    int hx = (w - headerW) / 2;
    int hy = 16;

    // Drop shadow
    painter.setBrush(QColor(0, 0, 0, 140));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(hx + 3, hy + 5, headerW, headerH, 14, 14);

    // Banner gradient
    QLinearGradient bGrad(hx, hy, hx, hy + headerH);
    bGrad.setColorAt(0.0, QColor(255, 75, 120));
    bGrad.setColorAt(0.5, QColor(220, 30, 80));
    bGrad.setColorAt(1.0, QColor(160, 15, 55));
    painter.setBrush(bGrad);
    painter.setPen(QPen(QColor(255, 215, 100), 3));
    painter.drawRoundedRect(hx, hy, headerW, headerH, 14, 14);

    // Title text
    painter.setFont(QFont("Arial", std::max(15, headerH * 3 / 8), QFont::Bold));
    painter.setPen(QColor(60, 10, 20));
    painter.drawText(hx + 1, hy + 2, headerW, headerH * 5 / 8, Qt::AlignCenter, "PAC-MAN LABYRINTH SAGA");
    painter.setPen(QColor(255, 235, 120));
    painter.drawText(hx, hy, headerW, headerH * 5 / 8, Qt::AlignCenter, "PAC-MAN LABYRINTH SAGA");

    painter.setFont(QFont("Arial", std::max(9, headerH / 6)));
    painter.setPen(QColor(255, 220, 230));
    painter.drawText(hx, hy + headerH * 4 / 8, headerW, headerH * 3 / 8, Qt::AlignCenter,
                     "Explore the Unfolding Multi-Window Labyrinths!");

    // Progress Badge on top-right
    int pbW = 160, pbH = 34;
    int pbx = w - pbW - 20;
    int pby = 22;
    painter.setBrush(QColor(15, 20, 45, 200));
    painter.setPen(QPen(QColor(0, 220, 255), 1.5));
    painter.drawRoundedRect(pbx, pby, pbW, pbH, 8, 8);

    painter.setFont(QFont("Arial", 9, QFont::Bold));
    painter.setPen(QColor(255, 215, 0));
    painter.drawText(pbx, pby, pbW, pbH, Qt::AlignCenter,
                     QString("⭐ LEVEL %1 / %2").arg(highestUnlockedLevel).arg(TOTAL_LEVELS));

    // 5. Mode & Hero Selector: [ 🟡 YELLOW (1P) ]  [ 🎀 PINK (1P) ]  [ 👥 CO-OP (2P) ]
    QRect selRect = getModeSelectorRect();
    QRect pYellow = getModeYellowRect().adjusted(2, 2, -2, -2);
    QRect pPink = getModePinkRect().adjusted(2, 2, -2, -2);
    QRect pCoop = getModeCoopRect().adjusted(2, 2, -2, -2);
    QPoint mousePos = mapFromGlobal(QCursor::pos());
    bool yellowHover = pYellow.contains(mousePos);
    bool pinkHover = pPink.contains(mousePos);
    bool coopHover = pCoop.contains(mousePos);

    // Pill container shadow & background
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 140));
    painter.drawRoundedRect(selRect.adjusted(2, 3, 2, 3), selRect.height() / 2, selRect.height() / 2);

    painter.setBrush(QColor(24, 12, 42, 235));
    painter.setPen(QPen(QColor(130, 80, 180), 2));
    painter.drawRoundedRect(selRect, selRect.height() / 2, selRect.height() / 2);

    // Button 1: Yellow Solo
    bool yellowActive = (!isMultiplayer && singlePlayerHero == 1);
    if (yellowActive) {
        QLinearGradient g(pYellow.left(), pYellow.top(), pYellow.left(), pYellow.bottom());
        g.setColorAt(0.0, QColor(255, 225, 50));
        g.setColorAt(0.5, QColor(240, 160, 20));
        g.setColorAt(1.0, QColor(200, 110, 10));
        painter.setBrush(g);
        painter.setPen(QPen(QColor(255, 250, 200), yellowHover ? 2.5 : 1.5));
        painter.drawRoundedRect(pYellow, pYellow.height() / 2, pYellow.height() / 2);

        painter.setFont(QFont("Arial", std::max(9, pYellow.height() * 3 / 8), QFont::Bold));
        painter.setPen(QColor(40, 20, 5));
        painter.drawText(pYellow, Qt::AlignCenter, "🟡 YELLOW (1P)");
    } else {
        if (yellowHover) {
            painter.setBrush(QColor(70, 40, 100, 180));
            painter.setPen(QPen(QColor(180, 140, 220), 1));
            painter.drawRoundedRect(pYellow, pYellow.height() / 2, pYellow.height() / 2);
        }
        painter.setFont(QFont("Arial", std::max(9, pYellow.height() * 3 / 8)));
        painter.setPen(QColor(220, 200, 140));
        painter.drawText(pYellow, Qt::AlignCenter, "🟡 YELLOW");
    }

    // Button 2: Pink Solo
    bool pinkActive = (!isMultiplayer && singlePlayerHero == 2);
    if (pinkActive) {
        QLinearGradient g(pPink.left(), pPink.top(), pPink.left(), pPink.bottom());
        g.setColorAt(0.0, QColor(255, 110, 185));
        g.setColorAt(0.5, QColor(235, 45, 135));
        g.setColorAt(1.0, QColor(180, 15, 90));
        painter.setBrush(g);
        painter.setPen(QPen(QColor(255, 230, 245), pinkHover ? 2.5 : 1.5));
        painter.drawRoundedRect(pPink, pPink.height() / 2, pPink.height() / 2);

        painter.setFont(QFont("Arial", std::max(9, pPink.height() * 3 / 8), QFont::Bold));
        painter.setPen(Qt::white);
        painter.drawText(pPink, Qt::AlignCenter, "🎀 PINK (1P)");
    } else {
        if (pinkHover) {
            painter.setBrush(QColor(90, 30, 80, 180));
            painter.setPen(QPen(QColor(220, 120, 190), 1));
            painter.drawRoundedRect(pPink, pPink.height() / 2, pPink.height() / 2);
        }
        painter.setFont(QFont("Arial", std::max(9, pPink.height() * 3 / 8)));
        painter.setPen(QColor(230, 160, 200));
        painter.drawText(pPink, Qt::AlignCenter, "🎀 PINK");
    }

    // Button 3: 2-Player Co-op
    if (isMultiplayer) {
        QLinearGradient g(pCoop.left(), pCoop.top(), pCoop.left(), pCoop.bottom());
        g.setColorAt(0.0, QColor(40, 210, 255));
        g.setColorAt(0.5, QColor(10, 140, 240));
        g.setColorAt(1.0, QColor(5, 80, 180));
        painter.setBrush(g);
        painter.setPen(QPen(QColor(200, 245, 255), coopHover ? 2.5 : 1.5));
        painter.drawRoundedRect(pCoop, pCoop.height() / 2, pCoop.height() / 2);

        painter.setFont(QFont("Arial", std::max(9, pCoop.height() * 3 / 8), QFont::Bold));
        painter.setPen(Qt::white);
        painter.drawText(pCoop, Qt::AlignCenter, "👥 CO-OP (2P)");
    } else {
        if (coopHover) {
            painter.setBrush(QColor(30, 50, 100, 180));
            painter.setPen(QPen(QColor(100, 180, 240), 1));
            painter.drawRoundedRect(pCoop, pCoop.height() / 2, pCoop.height() / 2);
        }
        painter.setFont(QFont("Arial", std::max(9, pCoop.height() * 3 / 8)));
        painter.setPen(QColor(160, 210, 245));
        painter.drawText(pCoop, Qt::AlignCenter, "👥 CO-OP");
    }

    // 6. Big PLAY Button at the Bottom
    QRect btnRect = getPlayButtonRect();
    bool btnHovered = btnRect.contains(mousePos);

    // Drop shadow
    painter.setBrush(QColor(0, 0, 0, 150));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(btnRect.adjusted(3, 4, 3, 4), btnRect.height() / 2, btnRect.height() / 2);

    // Button body
    QLinearGradient pGrad(btnRect.left(), btnRect.top(), btnRect.left(), btnRect.bottom());
    if (btnHovered) {
        pGrad.setColorAt(0.0, QColor(70, 240, 120));
        pGrad.setColorAt(0.5, QColor(30, 200, 80));
        pGrad.setColorAt(1.0, QColor(10, 150, 50));
    } else {
        pGrad.setColorAt(0.0, QColor(255, 175, 40));
        pGrad.setColorAt(0.5, QColor(235, 120, 20));
        pGrad.setColorAt(1.0, QColor(190, 70, 10));
    }
    painter.setBrush(pGrad);
    painter.setPen(QPen(QColor(255, 245, 180), btnHovered ? 3.5 : 2.5));
    painter.drawRoundedRect(btnRect, btnRect.height() / 2, btnRect.height() / 2);

    // Button text
    QString playText;
    if (isMultiplayer) {
        playText = QString("▶ PLAY 2-PLAYER CO-OP (LVL %1)").arg(selectedMapLevel);
    } else if (singlePlayerHero == 2) {
        playText = QString("▶ PLAY AS PINK MS. PACMAN (LVL %1)").arg(selectedMapLevel);
    } else {
        playText = QString("▶ PLAY AS YELLOW PACMAN (LVL %1)").arg(selectedMapLevel);
    }

    painter.setFont(QFont("Arial", std::max(12, btnRect.height() * 3 / 8), QFont::Bold));
    painter.setPen(QColor(40, 20, 5));
    painter.drawText(btnRect.adjusted(1, 2, 1, 2), Qt::AlignCenter, playText);
    painter.setPen(Qt::white);
    painter.drawText(btnRect, Qt::AlignCenter, playText);

    // Subtitle below button
    QString subText;
    if (isMultiplayer) {
        subText = QString("%1  •  [Arrows] Steer Active Hero  •  [Tab] Swap Arrows to Partner!").arg(getLevelTitle(selectedMapLevel));
    } else if (singlePlayerHero == 2) {
        subText = QString("%1  •  Steer Pink Ms. Pacman with [Arrows] or [WASD]!").arg(getLevelTitle(selectedMapLevel));
    } else {
        subText = QString("%1  •  Steer Yellow Pacman with [Arrows] or [WASD]!").arg(getLevelTitle(selectedMapLevel));
    }

    painter.setFont(QFont("Arial", std::max(8, btnRect.height() / 4)));
    painter.setPen(QColor(230, 215, 245));
    painter.drawText(0, btnRect.bottom() + 3, w, 16, Qt::AlignCenter, subText);
}
