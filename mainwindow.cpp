#include "mainwindow.h"
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QRandomGenerator>
#include <QPoint>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(700, 750);
    setStyleSheet("background-color: black;");
    setWindowTitle("Pacman: Multi-Window Maze Escape (Fullscreen)");

    fogOfWarEnabled = true;
    fogPeekActive = false;
    isTransitioning = false;

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
    currentLevel = 1;
    moveCount = 0;
    gameOver = false;
    levelComplete = false;

    initLevel();
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
    } else {
        sectorCols = 3; sectorRows = 3; // 3x3 = 9 windows (mega labyrinth!)
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
    currentSectorY = sectorRows - 1;
    sectorDiscovered[currentSectorY][currentSectorX] = true;

    // Exit at top-right sector (furthest from start!)
    exitSectorX = sectorCols - 1;
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

    levelComplete = false;
    gameOver = false;
    isTransitioning = false;

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

    ghostsFrightened = false;
    frightenedTicksLeft = 0;

    ghostMoveEveryN = std::max(1, 4 - currentLevel);
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
            double dist = std::hypot(c - pacmanX, r - pacmanY);
            if (dist <= VISION_RADIUS) {
                explored[currentSectorY][currentSectorX][r][c] = true;
            }
        }
    }
}

bool MainWindow::isCellVisible(int sx, int sy, int x, int y) const
{
    if (!fogOfWarEnabled) return true;
    if (sx != currentSectorX || sy != currentSectorY) return false;
    return std::hypot(x - pacmanX, y - pacmanY) <= VISION_RADIUS;
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

    if (gameOver) {
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_R) {
            initGame();
            update();
        }
        return;
    }

    if (levelComplete) {
        if (event->key() == Qt::Key_Space) {
            currentLevel++;
            initLevel();
            update();
        }
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

    // Prevent input during active camera window slide transition
    if (isTransitioning) return;

    Direction attemptDir = STOP;
    switch (event->key()) {
        case Qt::Key_Up:    attemptDir = UP; break;
        case Qt::Key_Down:  attemptDir = DOWN; break;
        case Qt::Key_Left:  attemptDir = LEFT; break;
        case Qt::Key_Right: attemptDir = RIGHT; break;
        case Qt::Key_R:     initGame(); update(); return;
        default: QWidget::keyPressEvent(event); return;
    }

    if (attemptDir != STOP) {
        pacmanNextDir = attemptDir;
        movePacman();
        moveCount++;
        checkFruitPickup();
        checkGhostCollisions();

        // Check if reached EXIT in the Exit window
        if (currentSectorX == exitSectorX && currentSectorY == exitSectorY &&
            pacmanX == exitX && pacmanY == exitY) {
            levelComplete = true;
            score += 500 * (sectorCols * sectorRows);
        }

        update();
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
            isTransitioning = true;
            transitionTimer.start();
            sectorDiscovered[currentSectorY][currentSectorX] = true;
        } else if (nextX < 0) {
            fromSectorX = currentSectorX; fromSectorY = currentSectorY;
            toSectorX = currentSectorX - 1; toSectorY = currentSectorY;
            transitionDirX = -1; transitionDirY = 0;
            currentSectorX = toSectorX;
            pacmanX = COLS - 1;
            isTransitioning = true;
            transitionTimer.start();
            sectorDiscovered[currentSectorY][currentSectorX] = true;
        } else if (nextY >= ROWS) {
            fromSectorX = currentSectorX; fromSectorY = currentSectorY;
            toSectorX = currentSectorX; toSectorY = currentSectorY + 1;
            transitionDirX = 0; transitionDirY = 1;
            currentSectorY = toSectorY;
            pacmanY = 0;
            isTransitioning = true;
            transitionTimer.start();
            sectorDiscovered[currentSectorY][currentSectorX] = true;
        } else if (nextY < 0) {
            fromSectorX = currentSectorX; fromSectorY = currentSectorY;
            toSectorX = currentSectorX; toSectorY = currentSectorY - 1;
            transitionDirX = 0; transitionDirY = -1;
            currentSectorY = toSectorY;
            pacmanY = ROWS - 1;
            isTransitioning = true;
            transitionTimer.start();
            sectorDiscovered[currentSectorY][currentSectorX] = true;
        } else {
            pacmanX = nextX;
            pacmanY = nextY;
        }

        updateFogOfWar();

        // Eat dot
        if (board[currentSectorY][currentSectorX][pacmanY][pacmanX] == 2) {
            board[currentSectorY][currentSectorX][pacmanY][pacmanX] = 0;
            score += 10;
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
            // If in the same sector as Pacman, actively hunt Pacman
            if (g.sectorX == currentSectorX && g.sectorY == currentSectorY && !ghostsFrightened) {
                int bestDist = 999;
                Direction bestDir = possibleDirs[0];
                for (Direction d : possibleDirs) {
                    int nx = g.x, ny = g.y;
                    if (d == UP) ny--;
                    else if (d == DOWN) ny++;
                    else if (d == LEFT) nx--;
                    else if (d == RIGHT) nx++;
                    int dist = std::abs(nx - pacmanX) + std::abs(ny - pacmanY);
                    if (dist < bestDist) {
                        bestDist = dist;
                        bestDir = d;
                    }
                }
                g.dir = bestDir;
            } else if (g.sectorX == currentSectorX && g.sectorY == currentSectorY && ghostsFrightened) {
                // Frightened: run away from Pacman
                int bestDist = -1;
                Direction bestDir = possibleDirs[0];
                for (Direction d : possibleDirs) {
                    int nx = g.x, ny = g.y;
                    if (d == UP) ny--;
                    else if (d == DOWN) ny++;
                    else if (d == LEFT) nx--;
                    else if (d == RIGHT) nx++;
                    int dist = std::abs(nx - pacmanX) + std::abs(ny - pacmanY);
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
        if (g.sectorX == currentSectorX && g.sectorY == currentSectorY &&
            g.x == pacmanX && g.y == pacmanY) {
            if (ghostsFrightened) {
                score += 200;
                respawnGhostFarAway(g);
            } else {
                gameOver = true;
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
        if (activeFruits[i].sectorX == currentSectorX && activeFruits[i].sectorY == currentSectorY &&
            activeFruits[i].x == pacmanX && activeFruits[i].y == pacmanY) {
            score += 100;
            activeFruits.removeAt(i);

            ghostsFrightened = true;
            frightenedTicksLeft = FRIGHTEN_DURATION;
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
    for (const BonusFruit &fruit : activeFruits) {
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

    // 5. Draw Pacman (if in this sector)
    if (isCurrent) {
        int px = originX + pacmanX * cellSize;
        int py = originY + pacmanY * cellSize;

        painter.setBrush(QColor(255, 220, 0));
        painter.setPen(Qt::NoPen);
        int startAngle = 0;
        if (pacmanDir == RIGHT) startAngle = pacmanMouthAngle;
        else if (pacmanDir == DOWN) startAngle = 270 + pacmanMouthAngle;
        else if (pacmanDir == LEFT) startAngle = 180 + pacmanMouthAngle;
        else if (pacmanDir == UP) startAngle = 90 + pacmanMouthAngle;

        int spanAngle = 360 - 2 * pacmanMouthAngle;
        painter.drawPie(px + 2, py + 2, cellSize - 4, cellSize - 4, startAngle * 16, spanAngle * 16);
    }

    // 6. Draw Ghosts in this sector
    QColor ghostNormalColors[] = {
        QColor(255, 50, 50),   // Blinky (Red)
        QColor(255, 180, 220), // Pinky (Pink)
        QColor(50, 220, 255),  // Inky (Cyan)
        QColor(255, 160, 50)   // Clyde (Orange)
    };

    for (const Ghost &g : ghosts) {
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
        int pxCenter = originX + pacmanX * cellSize + cellSize / 2;
        int pyCenter = originY + pacmanY * cellSize + cellSize / 2;

        QRadialGradient lantern(pxCenter, pyCenter, (VISION_RADIUS + 0.6) * cellSize);
        lantern.setColorAt(0.0, QColor(255, 235, 170, 25));
        lantern.setColorAt(0.45, QColor(255, 200, 100, 10));
        lantern.setColorAt(0.75, QColor(0, 0, 0, 60));
        lantern.setColorAt(1.0, QColor(0, 0, 0, 220));

        painter.fillRect(originX, originY, COLS * cellSize, ROWS * cellSize, lantern);
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
        painter.drawText(gx - cellSize * 3 - 6, gy + cellSize * 1.1, QString("WINDOW %1 ▶").arg(sx + 2));
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
        painter.drawText(gx + gw + 4, gy + cellSize * 1.1, QString("◀ WINDOW %1").arg(sx));
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
        painter.drawText(gx - 4, gy - 6, QString("▼ WINDOW (ROW %1) ▼").arg(sy + 2));
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
        painter.drawText(gx - 4, gy + gh + 14, QString("▲ WINDOW (ROW %1) ▲").arg(sy));
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
    if (gameOver || levelComplete) return;

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
    // Sector Mini-Map (Top-Right HUD Corner)
    // ========================================================
    {
        int mapBoxSize = std::max(16, cellSize / 2);
        int mapGap = 3;
        int totalMapW = sectorCols * mapBoxSize + (sectorCols - 1) * mapGap;
        int totalMapH = sectorRows * mapBoxSize + (sectorRows - 1) * mapGap;
        int mapX = startX + mazeW - totalMapW - 14;
        int mapY = startY + 14;

        int discCount = 0;
        for (int sy = 0; sy < sectorRows; ++sy) {
            for (int sx = 0; sx < sectorCols; ++sx) {
                if (sectorDiscovered[sy][sx]) discCount++;
            }
        }

        // Map container background
        painter.fillRect(mapX - 6, mapY - 18, totalMapW + 12, totalMapH + 24, QColor(0, 0, 0, 200));
        painter.setPen(QPen(QColor(40, 80, 140, 180), 1));
        painter.drawRect(mapX - 6, mapY - 18, totalMapW + 12, totalMapH + 24);

        painter.setFont(QFont("Arial", std::max(7, mapBoxSize / 2), QFont::Bold));
        painter.setPen(QColor(180, 220, 255));
        painter.drawText(mapX - 2, mapY - 5, QString("WINDOWS (%1/%2)").arg(discCount).arg(sectorCols * sectorRows));

        for (int sy = 0; sy < sectorRows; ++sy) {
            for (int sx = 0; sx < sectorCols; ++sx) {
                int rx = mapX + sx * (mapBoxSize + mapGap);
                int ry = mapY + sy * (mapBoxSize + mapGap);

                if (!sectorDiscovered[sy][sx]) {
                    // Undiscovered window
                    painter.fillRect(rx, ry, mapBoxSize, mapBoxSize, QColor(14, 18, 25));
                    painter.setPen(QColor(50, 60, 75));
                    painter.drawRect(rx, ry, mapBoxSize, mapBoxSize);
                    painter.drawText(rx + mapBoxSize/3, ry + mapBoxSize * 2/3, "?");
                } else {
                    // Discovered window
                    painter.fillRect(rx, ry, mapBoxSize, mapBoxSize, QColor(20, 45, 90));
                    painter.setPen(QColor(60, 140, 220));
                    painter.drawRect(rx, ry, mapBoxSize, mapBoxSize);

                    // Mark Exit Window
                    if (sx == exitSectorX && sy == exitSectorY) {
                        painter.setPen(QColor(0, 255, 120));
                        painter.setFont(QFont("Arial", std::max(8, mapBoxSize / 2), QFont::Bold));
                        painter.drawText(rx + mapBoxSize/4, ry + mapBoxSize * 3/4, "E");
                    }
                }

                // Current window cursor (Yellow outline + Pacman dot)
                if (sx == currentSectorX && sy == currentSectorY) {
                    painter.setPen(QPen(QColor(255, 220, 0), 2));
                    painter.drawRect(rx - 1, ry - 1, mapBoxSize + 2, mapBoxSize + 2);

                    painter.setBrush(QColor(255, 220, 0));
                    painter.setPen(Qt::NoPen);
                    painter.drawEllipse(rx + mapBoxSize/2 - 2, ry + mapBoxSize/2 - 2, 4, 4);
                }

                // Inter-window doorway connectors
                if (sx < sectorCols - 1) {
                    painter.setPen(QPen(QColor(0, 220, 255, 180), 2));
                    int cy = ry + mapBoxSize / 2;
                    painter.drawLine(rx + mapBoxSize, cy, rx + mapBoxSize + mapGap, cy);
                }
                if (sy < sectorRows - 1) {
                    painter.setPen(QPen(QColor(0, 220, 255, 180), 2));
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

    // Line 1: Stats & Window location (hudY + 20)
    painter.setFont(QFont("Arial", baseFont1, QFont::Bold));
    painter.setPen(Qt::white);
    painter.drawText(20, hudY + 20, QString("SCORE: %1").arg(score));

    painter.setPen(QColor(0, 230, 160));
    painter.drawText(width() * 0.16, hudY + 20, QString("LEVEL: %1").arg(currentLevel));

    painter.setPen(QColor(0, 210, 255));
    painter.drawText(width() * 0.28, hudY + 20,
                     QString("WINDOW: [%1, %2] of [%3x%4]")
                     .arg(currentSectorX + 1).arg(currentSectorY + 1)
                     .arg(sectorCols).arg(sectorRows));

    painter.setPen(QColor(180, 200, 255));
    painter.drawText(width() * 0.48, hudY + 20, QString("MOVES: %1").arg(moveCount));

    qint64 elapsedSec = elapsedTimer.elapsed() / 1000;
    int mins = elapsedSec / 60;
    int secs = elapsedSec % 60;
    painter.setPen(QColor(255, 200, 100));
    painter.drawText(width() * 0.60, hudY + 20, QString("TIME: %1:%2")
                     .arg(mins, 2, 10, QChar('0'))
                     .arg(secs, 2, 10, QChar('0')));

    if (fogPeekActive) {
        int msLeft = std::max(0, FOG_PEEK_DURATION_MS - (int)fogPeekTimer.elapsed());
        double secLeft = msLeft / 1000.0;
        painter.setPen(QColor(255, 215, 0));
        painter.drawText(width() * 0.74, hudY + 20, QString("[PEEK: %1s ☀️]").arg(secLeft, 0, 'f', 1));
    } else if (fogOfWarEnabled) {
        painter.setPen(QColor(120, 220, 255));
        painter.drawText(width() * 0.74, hudY + 20, "[FOG: ON 🌑 (F)]");
    } else {
        painter.setPen(QColor(150, 150, 160));
        painter.drawText(width() * 0.74, hudY + 20, "[FOG: OFF ☀️]");
    }

    if (ghostsFrightened) {
        painter.setPen(QColor(255, 100, 140));
        painter.drawText(width() * 0.88, hudY + 20, QString("⚡ POWER! %1").arg(frightenedTicksLeft));
    }

    // Line 2: Prominent Mission Objective Guidance (hudY + 42)
    int discCount = 0;
    for (int sy = 0; sy < sectorRows; ++sy) {
        for (int sx = 0; sx < sectorCols; ++sx) {
            if (sectorDiscovered[sy][sx]) discCount++;
        }
    }

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

        objectiveText = QString("🚪 OBJECTIVE: Walk into the glowing cyan %1 to transition into the next window! (%2/%3 windows discovered)")
            .arg(dirStr).arg(discCount).arg(sectorCols * sectorRows);
        painter.setPen(QColor(255, 220, 80));
    }
    painter.setFont(QFont("Arial", baseFont1, QFont::Bold));
    painter.drawText(20, hudY + 42, objectiveText);

    // Line 3: Controls (hudY + 62)
    painter.setFont(QFont("Arial", baseFont2));
    painter.setPen(QColor(140, 160, 190));
    painter.drawText(20, hudY + 62,
        "[Arrows] Move Pacman  |  [F] 1.5s Fog Peek  |  [F11] Fullscreen  |  [R] Reset  |  Follow glowing Gateways to next windows!");

    // Game Over / Level Complete Banner
    if (gameOver) {
        painter.fillRect(0, height() / 2 - 40, width(), 80, QColor(0, 0, 0, 225));
        painter.setPen(QColor(255, 60, 60));
        painter.setFont(QFont("Arial", std::max(18, width() / 55), QFont::Bold));
        painter.drawText(rect(), Qt::AlignCenter, "GAME OVER! Press [R] to Restart");
    } else if (levelComplete) {
        painter.fillRect(0, height() / 2 - 45, width(), 90, QColor(0, 0, 0, 225));
        painter.setPen(QColor(60, 255, 120));
        painter.setFont(QFont("Arial", std::max(16, width() / 60), QFont::Bold));
        QString levelMsg = QString("LEVEL %1 COMPLETE! All Windows Cleared!").arg(currentLevel);
        painter.drawText(0, height() / 2 - 12, width(), 25, Qt::AlignCenter, levelMsg);
        painter.setPen(QColor(255, 220, 100));
        painter.setFont(QFont("Arial", std::max(12, width() / 75)));
        painter.drawText(0, height() / 2 + 16, width(), 25, Qt::AlignCenter,
                         "Press [Space] for Next Level (Bigger Multi-Window World!)");
    }
}
