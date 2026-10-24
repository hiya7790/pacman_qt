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
    setWindowTitle("Pacman: Dark Maze Escape (Fullscreen)");

    fogOfWarEnabled = true;

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

void MainWindow::generateMaze()
{
    // 1. Fill entire board with walls and reset explored fog
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            board[r][c] = 1;
            explored[r][c] = false;
        }
    }

    // 2. Randomized Depth-First Search (DFS) carving on odd grid nodes
    bool visited[ROWS][COLS] = {};
    QVector<QPoint> stack;

    visited[1][1] = true;
    board[1][1] = 0;
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
            board[wallR][wallC] = 0;

            board[nr][nc] = 0;
            visited[nr][nc] = true;
            stack.push_back(QPoint(nc, nr));
        } else {
            stack.pop_back();
        }
    }

    // 3. Central Ghost House room (rows 9..11, cols 8..12)
    for (int r = 9; r <= 11; ++r) {
        for (int c = 8; c <= 12; ++c) {
            board[r][c] = 0;
        }
    }
    board[8][10] = 0;
    board[7][10] = 0;
    board[12][10] = 0;
    board[13][10] = 0;

    // 4. Side Warp Tunnels on Row 10
    board[10][0] = 0;
    board[10][1] = 0;
    board[10][COLS - 2] = 0;
    board[10][COLS - 1] = 0;

    // 5. Braid the maze: Randomly remove ~28% of internal dividing walls
    for (int r = 2; r < ROWS - 2; ++r) {
        for (int c = 2; c < COLS - 2; ++c) {
            if (board[r][c] == 1) {
                if (board[r-1][c] == 0 && board[r+1][c] == 0 && board[r][c-1] == 1 && board[r][c+1] == 1) {
                    if (QRandomGenerator::global()->bounded(100) < 28) {
                        board[r][c] = 0;
                    }
                }
                else if (board[r][c-1] == 0 && board[r][c+1] == 0 && board[r-1][c] == 1 && board[r+1][c] == 1) {
                    if (QRandomGenerator::global()->bounded(100) < 28) {
                        board[r][c] = 0;
                    }
                }
            }
        }
    }

    // 6. Set EXIT point at top-left corner (1, 1)
    exitX = 1;
    exitY = 1;
    board[exitY][exitX] = 0;

    // 7. Find Pacman start position in bottom corridor near center
    pacmanX = 10;
    pacmanY = ROWS - 2;
    bool foundStart = false;
    for (int r = ROWS - 2; r >= ROWS - 5; --r) {
        for (int offset = 0; offset < COLS / 2; ++offset) {
            for (int sign : {0, -1, 1}) {
                int c = 10 + sign * offset;
                if (c >= 1 && c < COLS - 1 && board[r][c] == 0) {
                    pacmanX = c;
                    pacmanY = r;
                    foundStart = true;
                    break;
                }
            }
            if (foundStart) break;
        }
        if (foundStart) break;
    }
    board[pacmanY][pacmanX] = 0;

    // 8. Populate Dots (2)
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            if (board[r][c] == 0) {
                if (c == exitX && r == exitY) continue;
                if (c == pacmanX && r == pacmanY) continue;
                if (r >= 9 && r <= 11 && c >= 8 && c <= 12) continue;
                if ((c == 0 || c == COLS - 1) && r == 10) continue;

                board[r][c] = 2; // Dot
            }
        }
    }
}

void MainWindow::initLevel()
{
    generateMaze();

    pacmanDir = LEFT;
    pacmanNextDir = LEFT;
    pacmanMouthAngle = 45;
    pacmanMouthOpening = false;

    levelComplete = false;
    gameOver = false;

    ghosts.clear();
    ghosts.push_back({10, 9, UP, 0});     // Blinky (Red)
    ghosts.push_back({9, 10, LEFT, 1});   // Pinky (Pink)
    ghosts.push_back({11, 10, RIGHT, 2}); // Inky (Cyan)
    ghosts.push_back({10, 10, DOWN, 3});  // Clyde (Orange)

    ghostsFrightened = false;
    frightenedTicksLeft = 0;

    ghostMoveEveryN = std::max(1, 4 - currentLevel);
    ghostTickCounter = 0;

    activeFruits.clear();
    fruitSpawnCounter = QRandomGenerator::global()->bounded(15, 40);

    updateFogOfWar();
    elapsedTimer.start();
}

void MainWindow::updateFogOfWar()
{
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            double dist = std::hypot(c - pacmanX, r - pacmanY);
            if (dist <= VISION_RADIUS) {
                explored[r][c] = true;
            }
        }
    }
}

bool MainWindow::isCellVisible(int x, int y) const
{
    if (!fogOfWarEnabled) return true;
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

    // Toggle Fog of War
    if (event->key() == Qt::Key_F) {
        fogOfWarEnabled = !fogOfWarEnabled;
        update();
        return;
    }

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
        updateFogOfWar();
        checkFruitPickup();
        checkGhostCollisions();

        if (pacmanX == exitX && pacmanY == exitY) {
            levelComplete = true;
            score += 500;
        }

        update();
    }
}

bool MainWindow::canMove(int x, int y, Direction dir)
{
    int nextX = x;
    int nextY = y;

    if (dir == UP) nextY--;
    else if (dir == DOWN) nextY++;
    else if (dir == LEFT) nextX--;
    else if (dir == RIGHT) nextX++;

    if (nextX < 0 || nextX >= COLS) return true;

    if (nextY >= 0 && nextY < ROWS) {
        if (board[nextY][nextX] != 1) return true;
    }
    return false;
}

void MainWindow::movePacman()
{
    if (canMove(pacmanX, pacmanY, pacmanNextDir)) {
        pacmanDir = pacmanNextDir;
    }

    if (canMove(pacmanX, pacmanY, pacmanDir)) {
        if (pacmanDir == UP) pacmanY--;
        else if (pacmanDir == DOWN) pacmanY++;
        else if (pacmanDir == LEFT) pacmanX--;
        else if (pacmanDir == RIGHT) pacmanX++;

        if (pacmanX < 0) pacmanX = COLS - 1;
        if (pacmanX >= COLS) pacmanX = 0;

        if (board[pacmanY][pacmanX] == 2) {
            board[pacmanY][pacmanX] = 0;
            score += 10;
        }

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
            if (canMove(g.x, g.y, d)) {
                if (d != reverseDir) possibleDirs.push_back(d);
            }
        }

        if (possibleDirs.isEmpty()) {
            if (canMove(g.x, g.y, reverseDir)) g.dir = reverseDir;
        } else {
            if (!ghostsFrightened && g.colorIndex == 0 && currentLevel >= 3) {
                int bestDist = 999;
                Direction bestDir = possibleDirs[0];
                for (Direction d : possibleDirs) {
                    int nx = g.x, ny = g.y;
                    if (d == UP) ny--;
                    else if (d == DOWN) ny++;
                    else if (d == LEFT) nx--;
                    else if (d == RIGHT) nx++;
                    if (nx < 0) nx = COLS - 1;
                    if (nx >= COLS) nx = 0;
                    int dist = std::abs(nx - pacmanX) + std::abs(ny - pacmanY);
                    if (dist < bestDist) {
                        bestDist = dist;
                        bestDir = d;
                    }
                }
                g.dir = bestDir;
            } else if (ghostsFrightened) {
                int bestDist = -1;
                Direction bestDir = possibleDirs[0];
                for (Direction d : possibleDirs) {
                    int nx = g.x, ny = g.y;
                    if (d == UP) ny--;
                    else if (d == DOWN) ny++;
                    else if (d == LEFT) nx--;
                    else if (d == RIGHT) nx++;
                    if (nx < 0) nx = COLS - 1;
                    if (nx >= COLS) nx = 0;
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

        if (g.x < 0) g.x = COLS - 1;
        if (g.x >= COLS) g.x = 0;
    }
}

void MainWindow::checkGhostCollisions()
{
    for (Ghost &g : ghosts) {
        if (g.x == pacmanX && g.y == pacmanY) {
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
    int bestDist = -1;
    int bestX = 10, bestY = 10;

    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            if (board[r][c] == 1) continue;
            if (c == pacmanX && r == pacmanY) continue;
            if (c == exitX && r == exitY) continue;

            bool onGhost = false;
            for (const Ghost &other : ghosts) {
                if (&other != &g && other.x == c && other.y == r) { onGhost = true; break; }
            }
            if (onGhost) continue;

            int dist = std::abs(c - pacmanX) + std::abs(r - pacmanY);
            if (dist > bestDist) {
                bestDist = dist;
                bestX = c;
                bestY = r;
            }
        }
    }

    g.x = bestX;
    g.y = bestY;
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

    QVector<QPair<int,int>> candidates;
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            if (board[r][c] == 0) {
                if (c == pacmanX && r == pacmanY) continue;
                if (c == exitX && r == exitY) continue;
                bool occupied = false;
                for (const Ghost &g : ghosts) {
                    if (g.x == c && g.y == r) { occupied = true; break; }
                }
                for (const BonusFruit &f : activeFruits) {
                    if (f.x == c && f.y == r) { occupied = true; break; }
                }
                if (occupied) continue;
                candidates.push_back({c, r});
            }
        }
    }

    if (candidates.isEmpty()) return;

    int idx = QRandomGenerator::global()->bounded(candidates.size());
    FruitType type = static_cast<FruitType>(QRandomGenerator::global()->bounded(4));
    int lifespan = QRandomGenerator::global()->bounded(40, 80);

    activeFruits.push_back({candidates[idx].first, candidates[idx].second, type, lifespan});
}

void MainWindow::checkFruitPickup()
{
    for (int i = activeFruits.size() - 1; i >= 0; --i) {
        if (activeFruits[i].x == pacmanX && activeFruits[i].y == pacmanY) {
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
// Game Loop
// ========================================================

void MainWindow::gameLoop()
{
    if (gameOver || levelComplete) return;

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
// Dynamic Fullscreen Rendering
// ========================================================

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Compute responsive cell size and center offset
    int hudHeight = 58;
    int availW = width();
    int availH = height() - hudHeight;

    int cellSize = std::min(availW / COLS, availH / ROWS);
    if (cellSize < 12) cellSize = 12;

    int mazeW = COLS * cellSize;
    int mazeH = ROWS * cellSize;
    int startX = (availW - mazeW) / 2;
    int startY = (availH - mazeH) / 2;

    // Fill background letterbox with deep black
    painter.fillRect(rect(), QColor(5, 5, 8));

    // 1. Draw Maze Tiles & Pellets (with Fog of War)
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            int cx = startX + c * cellSize;
            int cy = startY + r * cellSize;

            bool visible = isCellVisible(c, r);
            bool wasExplored = explored[r][c];

            if (fogOfWarEnabled && !visible && !wasExplored) {
                painter.fillRect(cx, cy, cellSize, cellSize, QColor(0, 0, 0));
                continue;
            }

            if (board[r][c] == 1) {
                // Wall tile
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
                    if (board[r][c] == 2) {
                        painter.setBrush(QColor(255, 230, 180));
                        painter.setPen(Qt::NoPen);
                        int dotSize = std::max(4, cellSize * 5 / 30);
                        painter.drawEllipse(cx + cellSize/2 - dotSize/2, cy + cellSize/2 - dotSize/2, dotSize, dotSize);
                    }
                } else {
                    painter.fillRect(cx, cy, cellSize, cellSize, QColor(4, 4, 10));
                    if (board[r][c] == 2) {
                        painter.setBrush(QColor(100, 95, 80, 75));
                        painter.setPen(Qt::NoPen);
                        int dotSize = std::max(2, cellSize * 3 / 30);
                        painter.drawEllipse(cx + cellSize/2 - dotSize/2, cy + cellSize/2 - dotSize/2, dotSize, dotSize);
                    }
                }
            }
        }
    }

    // 2. Draw EXIT portal
    {
        bool exitVisible = isCellVisible(exitX, exitY);
        bool exitExplored = explored[exitY][exitX];

        if (!fogOfWarEnabled || exitVisible || exitExplored) {
            int ex = startX + exitX * cellSize;
            int ey = startY + exitY * cellSize;

            if (exitVisible) {
                painter.setBrush(QColor(0, 255, 100, 50));
                painter.setPen(QPen(QColor(0, 255, 120, 180), std::max(2, cellSize / 15)));
                painter.drawRoundedRect(ex + 2, ey + 2, cellSize - 4, cellSize - 4, 6, 6);

                painter.setBrush(QColor(0, 255, 100, 120));
                painter.setPen(Qt::NoPen);
                painter.drawEllipse(ex + 5, ey + 5, cellSize - 10, cellSize - 10);

                painter.setPen(QColor(255, 255, 255));
                painter.setFont(QFont("Arial", std::max(7, cellSize / 4), QFont::Bold));
                painter.drawText(ex + 2, ey + cellSize/2 + cellSize/8, "EXIT");
            } else {
                painter.setBrush(QColor(0, 120, 50, 30));
                painter.setPen(QPen(QColor(0, 140, 60, 90), 1));
                painter.drawRoundedRect(ex + 2, ey + 2, cellSize - 4, cellSize - 4, 6, 6);

                painter.setPen(QColor(140, 200, 140, 140));
                painter.setFont(QFont("Arial", std::max(6, cellSize / 5), QFont::Bold));
                painter.drawText(ex + 3, ey + cellSize/2 + cellSize/8, "EXIT");
            }
        }
    }

    // 3. Draw Bonus Fruits
    for (const BonusFruit &fruit : activeFruits) {
        if (!isCellVisible(fruit.x, fruit.y)) continue;

        int fx = startX + fruit.x * cellSize;
        int fy = startY + fruit.y * cellSize;

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

    // 4. Draw Pacman
    {
        int px = startX + pacmanX * cellSize;
        int py = startY + pacmanY * cellSize;

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

    // 5. Draw Ghosts
    QColor ghostNormalColors[] = {
        QColor(255, 50, 50),   // Blinky (Red)
        QColor(255, 180, 220), // Pinky (Pink)
        QColor(50, 220, 255),  // Inky (Cyan)
        QColor(255, 160, 50)   // Clyde (Orange)
    };

    for (const Ghost &g : ghosts) {
        if (!isCellVisible(g.x, g.y)) continue;

        int gx = startX + g.x * cellSize;
        int gy = startY + g.y * cellSize;

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

        // Ghost body
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

    // 6. Lantern Light Spotlight / Ambient Vignette
    if (fogOfWarEnabled) {
        int pxCenter = startX + pacmanX * cellSize + cellSize / 2;
        int pyCenter = startY + pacmanY * cellSize + cellSize / 2;

        QRadialGradient lantern(pxCenter, pyCenter, (VISION_RADIUS + 0.6) * cellSize);
        lantern.setColorAt(0.0, QColor(255, 235, 170, 25));
        lantern.setColorAt(0.45, QColor(255, 200, 100, 10));
        lantern.setColorAt(0.75, QColor(0, 0, 0, 60));
        lantern.setColorAt(1.0, QColor(0, 0, 0, 220));

        painter.fillRect(startX, startY, mazeW, mazeH, lantern);
    }

    // 7. Fullscreen Bottom HUD Bar
    int hudY = height() - hudHeight;
    painter.fillRect(0, hudY, width(), hudHeight, QColor(14, 16, 26));
    painter.setPen(QPen(QColor(40, 60, 100), 2));
    painter.drawLine(0, hudY, width(), hudY);

    int baseFont1 = std::max(11, width() / 110);
    int baseFont2 = std::max(9, width() / 135);

    // Line 1: Metrics
    painter.setFont(QFont("Arial", baseFont1, QFont::Bold));
    painter.setPen(Qt::white);
    painter.drawText(20, hudY + 22, QString("SCORE: %1").arg(score));

    painter.setPen(QColor(0, 230, 160));
    painter.drawText(width() * 0.18, hudY + 22, QString("LEVEL: %1").arg(currentLevel));

    painter.setPen(QColor(180, 200, 255));
    painter.drawText(width() * 0.32, hudY + 22, QString("MOVES: %1").arg(moveCount));

    qint64 elapsedSec = elapsedTimer.elapsed() / 1000;
    int mins = elapsedSec / 60;
    int secs = elapsedSec % 60;
    painter.setPen(QColor(255, 200, 100));
    painter.drawText(width() * 0.48, hudY + 22, QString("TIME: %1:%2")
                     .arg(mins, 2, 10, QChar('0'))
                     .arg(secs, 2, 10, QChar('0')));

    if (fogOfWarEnabled) {
        painter.setPen(QColor(120, 220, 255));
        painter.drawText(width() * 0.65, hudY + 22, "[FOG: ON 🌑]");
    } else {
        painter.setPen(QColor(150, 150, 160));
        painter.drawText(width() * 0.65, hudY + 22, "[FOG: OFF ☀️]");
    }

    if (ghostsFrightened) {
        painter.setPen(QColor(255, 100, 140));
        painter.drawText(width() * 0.82, hudY + 22, QString("⚡ POWER! %1").arg(frightenedTicksLeft));
    }

    // Line 2: Controls
    painter.setFont(QFont("Arial", baseFont2));
    painter.setPen(QColor(130, 150, 180));
    painter.drawText(20, hudY + 45,
        "[Arrows] Move  |  [F] Toggle Fog  |  [F11 / Esc] Fullscreen  |  [R] New Maze/Reset  |  Reach EXIT to Win!");

    // 8. Game Over / Level Complete Banner
    if (gameOver) {
        painter.fillRect(0, height() / 2 - 40, width(), 80, QColor(0, 0, 0, 225));
        painter.setPen(QColor(255, 60, 60));
        painter.setFont(QFont("Arial", std::max(18, width() / 55), QFont::Bold));
        painter.drawText(rect(), Qt::AlignCenter, "GAME OVER! Press [R] for New Maze");
    } else if (levelComplete) {
        painter.fillRect(0, height() / 2 - 45, width(), 90, QColor(0, 0, 0, 225));
        painter.setPen(QColor(60, 255, 120));
        painter.setFont(QFont("Arial", std::max(16, width() / 60), QFont::Bold));
        QString levelMsg = QString("LEVEL %1 COMPLETE! +500 Bonus").arg(currentLevel);
        painter.drawText(0, height() / 2 - 12, width(), 25, Qt::AlignCenter, levelMsg);
        painter.setPen(QColor(255, 220, 100));
        painter.setFont(QFont("Arial", std::max(12, width() / 75)));
        painter.drawText(0, height() / 2 + 16, width(), 25, Qt::AlignCenter,
                         "Press [Space] for Next Level (Brand New Maze!)");
    }
}
