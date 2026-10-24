#include "mainwindow.h"
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>
#include <cmath>

// 1 = Wall, 0 = Empty, 2 = Dot
int initialBoard[21][21] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,1},
    {1,2,1,1,1,2,1,1,1,2,1,2,1,1,1,2,1,1,1,2,1},
    {1,2,1,1,1,2,1,1,1,2,1,2,1,1,1,2,1,1,1,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,1,1,1,2,1,2,1,1,1,1,1,2,1,2,1,1,1,2,1},
    {1,2,2,2,2,2,1,2,2,2,1,2,2,2,1,2,2,2,2,2,1},
    {1,1,1,1,1,2,1,1,1,0,1,0,1,1,1,2,1,1,1,1,1},
    {0,0,0,0,1,2,1,0,0,0,0,0,0,0,1,2,1,0,0,0,0},
    {1,1,1,1,1,2,1,0,1,1,0,1,1,0,1,2,1,1,1,1,1},
    {0,0,0,0,0,2,0,0,1,0,0,0,1,0,0,2,0,0,0,0,0},
    {1,1,1,1,1,2,1,0,1,1,1,1,1,0,1,2,1,1,1,1,1},
    {0,0,0,0,1,2,1,0,0,0,0,0,0,0,1,2,1,0,0,0,0},
    {1,1,1,1,1,2,1,2,1,1,1,1,1,2,1,2,1,1,1,1,1},
    {1,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,1},
    {1,2,1,1,1,2,1,1,1,2,1,2,1,1,1,2,1,1,1,2,1},
    {1,2,2,2,1,2,2,2,2,2,0,2,2,2,2,2,1,2,2,2,1},
    {1,1,1,2,1,2,1,2,1,1,1,1,1,2,1,2,1,2,1,1,1},
    {1,2,2,2,2,2,1,2,2,2,1,2,2,2,1,2,2,2,2,2,1},
    {1,2,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(COLS * CELL_SIZE, ROWS * CELL_SIZE + 55);
    setStyleSheet("background-color: black;");
    setWindowTitle("Pacman Maze Escape");

    initGame();

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::gameLoop);
    timer->start(130);
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

void MainWindow::initLevel()
{
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            board[r][c] = initialBoard[r][c];
        }
    }

    // Pacman starts at bottom-center
    pacmanX = 10;
    pacmanY = 16;
    pacmanDir = LEFT;
    pacmanNextDir = LEFT;
    pacmanMouthAngle = 45;
    pacmanMouthOpening = false;

    levelComplete = false;
    gameOver = false;

    // Exit at top-left open cell
    exitX = 1;
    exitY = 1;
    board[exitY][exitX] = 0; // make sure exit cell is clear

    // Ghosts
    ghosts.clear();
    ghosts.push_back({10, 10, UP, 0});    // Blinky (Red)
    ghosts.push_back({9, 10, LEFT, 1});   // Pinky (Pink)
    ghosts.push_back({11, 10, RIGHT, 2}); // Inky (Cyan)
    ghosts.push_back({10, 9, DOWN, 3});   // Clyde (Orange)

    // Frightened mode off
    ghostsFrightened = false;
    frightenedTicksLeft = 0;

    // Ghost speed: move every N ticks. Decreases with level (faster ghosts).
    // Level 1: every 3 ticks, Level 2: every 2, Level 3+: every 1
    ghostMoveEveryN = std::max(1, 4 - currentLevel);
    ghostTickCounter = 0;

    // Fruits
    activeFruits.clear();
    fruitSpawnCounter = QRandomGenerator::global()->bounded(15, 40);

    // Reset timer
    elapsedTimer.start();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
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

        // Check if reached exit
        if (pacmanX == exitX && pacmanY == exitY) {
            levelComplete = true;
            score += 500; // level completion bonus
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

    // Tunnel wrapping
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

        // Wrap around tunnels
        if (pacmanX < 0) pacmanX = COLS - 1;
        if (pacmanX >= COLS) pacmanX = 0;

        // Eat dot
        if (board[pacmanY][pacmanX] == 2) {
            board[pacmanY][pacmanX] = 0;
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
        // Frightened ghosts move slower (skip every other move)
        if (ghostsFrightened && QRandomGenerator::global()->bounded(3) == 0) {
            continue; // skip this ghost's move ~33% of the time when frightened
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
            // Level 3+: Blinky (red ghost, index 0) actively chases Pacman
            if (!ghostsFrightened && g.colorIndex == 0 && currentLevel >= 3) {
                // Chase mode: pick direction that gets closest to Pacman
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
                // Frightened: run AWAY from Pacman
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
                // Eat the ghost! +200 points, respawn it far away
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
    // Find the open cell farthest from Pacman
    int bestDist = -1;
    int bestX = 10, bestY = 10;

    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            if (board[r][c] == 1) continue;
            // Not on Pacman
            if (c == pacmanX && r == pacmanY) continue;
            // Not on exit
            if (c == exitX && r == exitY) continue;
            // Not on another ghost
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

            // POWER-UP: Eating a fruit makes ghosts frightened!
            ghostsFrightened = true;
            frightenedTicksLeft = FRIGHTEN_DURATION;
        }
    }
}

// ========================================================
// Fruit Drawing Helpers
// ========================================================

void MainWindow::drawCherry(QPainter &painter, int cx, int cy)
{
    int s = CELL_SIZE;
    painter.setPen(QPen(QColor(0, 160, 0), 2));
    painter.drawLine(cx + s/2, cy + 4, cx + s/2 - 4, cy + s/2 - 2);
    painter.drawLine(cx + s/2, cy + 4, cx + s/2 + 4, cy + s/2 - 2);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(220, 20, 20));
    painter.drawEllipse(cx + s/2 - 9, cy + s/2 - 3, 10, 10);
    painter.setBrush(QColor(200, 10, 10));
    painter.drawEllipse(cx + s/2 + 1, cy + s/2 - 3, 10, 10);
    painter.setBrush(QColor(255, 180, 180, 180));
    painter.drawEllipse(cx + s/2 - 7, cy + s/2, 3, 3);
    painter.drawEllipse(cx + s/2 + 3, cy + s/2, 3, 3);
}

void MainWindow::drawApple(QPainter &painter, int cx, int cy)
{
    int s = CELL_SIZE;
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(50, 200, 50));
    painter.drawEllipse(cx + 5, cy + 7, s - 10, s - 10);
    painter.setBrush(QColor(120, 255, 120, 150));
    painter.drawEllipse(cx + 8, cy + 9, 5, 5);
    painter.setPen(QPen(QColor(100, 60, 20), 2));
    painter.drawLine(cx + s/2, cy + 7, cx + s/2, cy + 3);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(30, 160, 30));
    painter.drawEllipse(cx + s/2, cy + 2, 6, 4);
}

void MainWindow::drawStrawberry(QPainter &painter, int cx, int cy)
{
    int s = CELL_SIZE;
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(220, 30, 50));
    QPolygon strawberry;
    strawberry << QPoint(cx + s/2, cy + s - 5)
               << QPoint(cx + 5, cy + 8)
               << QPoint(cx + s - 5, cy + 8);
    painter.drawPolygon(strawberry);
    painter.drawEllipse(cx + 5, cy + 6, s - 10, 8);
    painter.setBrush(QColor(255, 255, 100));
    painter.drawEllipse(cx + s/2 - 3, cy + 12, 2, 2);
    painter.drawEllipse(cx + s/2 + 2, cy + 14, 2, 2);
    painter.drawEllipse(cx + s/2 - 1, cy + 17, 2, 2);
    painter.setBrush(QColor(30, 160, 30));
    painter.drawEllipse(cx + s/2 - 5, cy + 4, 5, 4);
    painter.drawEllipse(cx + s/2 + 1, cy + 4, 5, 4);
}

void MainWindow::drawOrangeFruit(QPainter &painter, int cx, int cy)
{
    int s = CELL_SIZE;
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 160, 30));
    painter.drawEllipse(cx + 5, cy + 6, s - 10, s - 10);
    painter.setBrush(QColor(255, 220, 120, 160));
    painter.drawEllipse(cx + 8, cy + 9, 5, 5);
    painter.setPen(QPen(QColor(100, 60, 20), 2));
    painter.drawLine(cx + s/2, cy + 6, cx + s/2, cy + 3);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(30, 160, 30));
    painter.drawEllipse(cx + s/2, cy + 1, 6, 4);
}

// ========================================================
// Game Loop
// ========================================================

void MainWindow::gameLoop()
{
    if (gameOver || levelComplete) return;

    // Ghost movement governed by speed control
    ghostTickCounter++;
    if (ghostTickCounter >= ghostMoveEveryN) {
        ghostTickCounter = 0;
        moveGhosts();

        // Check ghost collisions after they move
        checkGhostCollisions();
    }

    // Frightened mode countdown
    if (ghostsFrightened) {
        frightenedTicksLeft--;
        if (frightenedTicksLeft <= 0) {
            ghostsFrightened = false;
        }
    }

    // Fruit spawning
    trySpawnFruit();

    // Tick down fruit lifespans
    for (int i = activeFruits.size() - 1; i >= 0; --i) {
        activeFruits[i].ticksRemaining--;
        if (activeFruits[i].ticksRemaining <= 0) {
            activeFruits.removeAt(i);
        }
    }

    update();
}

// ========================================================
// Rendering
// ========================================================

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 1. Draw Maze Tiles & Pellets
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            int cx = c * CELL_SIZE;
            int cy = r * CELL_SIZE;

            if (board[r][c] == 1) {
                painter.setBrush(QColor(25, 45, 130));
                painter.setPen(QPen(QColor(40, 80, 200), 1));
                painter.drawRoundedRect(cx + 1, cy + 1, CELL_SIZE - 2, CELL_SIZE - 2, 4, 4);
            } else {
                painter.fillRect(cx, cy, CELL_SIZE, CELL_SIZE, QColor(8, 8, 16));

                if (board[r][c] == 2) {
                    painter.setBrush(QColor(255, 230, 180));
                    painter.setPen(Qt::NoPen);
                    painter.drawEllipse(cx + CELL_SIZE/2 - 2, cy + CELL_SIZE/2 - 2, 5, 5);
                }
            }
        }
    }

    // 2. Draw EXIT portal
    {
        int ex = exitX * CELL_SIZE;
        int ey = exitY * CELL_SIZE;

        // Outer glow
        painter.setBrush(QColor(0, 255, 100, 50));
        painter.setPen(QPen(QColor(0, 255, 120, 180), 2));
        painter.drawRoundedRect(ex + 2, ey + 2, CELL_SIZE - 4, CELL_SIZE - 4, 6, 6);

        // Inner portal
        painter.setBrush(QColor(0, 255, 100, 120));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(ex + 5, ey + 5, CELL_SIZE - 10, CELL_SIZE - 10);

        // "EXIT" text
        painter.setPen(QColor(255, 255, 255));
        painter.setFont(QFont("Arial", 7, QFont::Bold));
        painter.drawText(ex + 3, ey + CELL_SIZE/2 + 4, "EXIT");
    }

    // 3. Draw Bonus Fruits
    for (const BonusFruit &fruit : activeFruits) {
        int fx = fruit.x * CELL_SIZE;
        int fy = fruit.y * CELL_SIZE;

        switch (fruit.type) {
            case CHERRY:       drawCherry(painter, fx, fy); break;
            case APPLE:        drawApple(painter, fx, fy); break;
            case STRAWBERRY:   drawStrawberry(painter, fx, fy); break;
            case ORANGE_FRUIT: drawOrangeFruit(painter, fx, fy); break;
        }

        // Point value label
        painter.setPen(QColor(255, 255, 100));
        painter.setFont(QFont("Arial", 7, QFont::Bold));
        painter.drawText(fx + 4, fy + CELL_SIZE - 1, "100");
    }

    // 4. Draw Pacman
    {
        painter.setBrush(QColor(255, 220, 0));
        painter.setPen(Qt::NoPen);
        int startAngle = 0;
        if (pacmanDir == RIGHT) startAngle = pacmanMouthAngle;
        else if (pacmanDir == DOWN) startAngle = 270 + pacmanMouthAngle;
        else if (pacmanDir == LEFT) startAngle = 180 + pacmanMouthAngle;
        else if (pacmanDir == UP) startAngle = 90 + pacmanMouthAngle;

        int spanAngle = 360 - 2 * pacmanMouthAngle;
        painter.drawPie(pacmanX * CELL_SIZE + 2, pacmanY * CELL_SIZE + 2,
                        CELL_SIZE - 4, CELL_SIZE - 4, startAngle * 16, spanAngle * 16);
    }

    // 5. Draw Ghosts
    QColor ghostNormalColors[] = {
        QColor(255, 50, 50),   // Blinky (Red)
        QColor(255, 180, 220), // Pinky (Pink)
        QColor(50, 220, 255),  // Inky (Cyan)
        QColor(255, 160, 50)   // Clyde (Orange)
    };

    for (const Ghost &g : ghosts) {
        int gx = g.x * CELL_SIZE;
        int gy = g.y * CELL_SIZE;

        if (ghostsFrightened) {
            // Frightened appearance: dark blue, flashing if almost over
            if (frightenedTicksLeft <= 8 && (frightenedTicksLeft % 2 == 0)) {
                painter.setBrush(QColor(255, 255, 255)); // white flash
            } else {
                painter.setBrush(QColor(30, 30, 200)); // dark blue
            }
        } else {
            painter.setBrush(ghostNormalColors[g.colorIndex]);
        }
        painter.setPen(Qt::NoPen);

        // Ghost head & body
        painter.drawEllipse(gx + 2, gy + 2, CELL_SIZE - 4, CELL_SIZE - 4);
        painter.drawRect(gx + 2, gy + CELL_SIZE/2, CELL_SIZE - 4, (CELL_SIZE - 4)/2);

        // Wavy bottom edge
        for (int w = 0; w < 3; ++w) {
            int wx = gx + 2 + w * ((CELL_SIZE - 4) / 3);
            int wy = gy + CELL_SIZE - 5;
            painter.drawEllipse(wx, wy, (CELL_SIZE - 4) / 3, 6);
        }

        if (ghostsFrightened) {
            // Scared face
            painter.setBrush(QColor(255, 200, 200));
            painter.drawEllipse(gx + 8, gy + 10, 4, 4);
            painter.drawEllipse(gx + 18, gy + 10, 4, 4);
            // Wavy mouth
            painter.setPen(QPen(QColor(255, 200, 200), 1));
            for (int m = 0; m < 4; ++m) {
                int mx = gx + 6 + m * 5;
                int my = gy + 18 + (m % 2 == 0 ? 0 : 2);
                painter.drawLine(mx, my, mx + 4, my + (m % 2 == 0 ? 2 : -2));
            }
            painter.setPen(Qt::NoPen);
        } else {
            // Normal eyes
            painter.setBrush(Qt::white);
            painter.drawEllipse(gx + 6, gy + 8, 6, 6);
            painter.drawEllipse(gx + 16, gy + 8, 6, 6);
            painter.setBrush(Qt::blue);
            painter.drawEllipse(gx + 8, gy + 10, 3, 3);
            painter.drawEllipse(gx + 18, gy + 10, 3, 3);
        }
    }

    // 6. Draw HUD
    int hudY = ROWS * CELL_SIZE;
    painter.fillRect(0, hudY, width(), height() - hudY, QColor(14, 16, 26));
    painter.setPen(QPen(QColor(40, 60, 100), 2));
    painter.drawLine(0, hudY, width(), hudY);

    // Line 1: Score, Level, Moves, Time
    painter.setFont(QFont("Arial", 11, QFont::Bold));
    painter.setPen(Qt::white);
    painter.drawText(12, hudY + 20, QString("SCORE: %1").arg(score));

    painter.setPen(QColor(0, 230, 160));
    painter.drawText(130, hudY + 20, QString("LEVEL: %1").arg(currentLevel));

    painter.setPen(QColor(180, 200, 255));
    painter.drawText(240, hudY + 20, QString("MOVES: %1").arg(moveCount));

    // Elapsed time
    qint64 elapsedSec = elapsedTimer.elapsed() / 1000;
    int mins = elapsedSec / 60;
    int secs = elapsedSec % 60;
    painter.setPen(QColor(255, 200, 100));
    painter.drawText(370, hudY + 20, QString("TIME: %1:%2")
                     .arg(mins, 2, 10, QChar('0'))
                     .arg(secs, 2, 10, QChar('0')));

    // Frightened indicator
    if (ghostsFrightened) {
        painter.setPen(QColor(100, 100, 255));
        painter.drawText(490, hudY + 20, QString("⚡ POWER! %1").arg(frightenedTicksLeft));
    }

    // Line 2: Controls
    painter.setFont(QFont("Arial", 9));
    painter.setPen(QColor(130, 150, 180));
    painter.drawText(12, hudY + 42,
        "[Arrows] Move  |  [R] Reset  |  Eat Fruit = Ghosts Edible!  |  Reach EXIT to clear level!");

    // 7. Game Over / Level Complete Banner
    if (gameOver) {
        painter.fillRect(0, ROWS * CELL_SIZE / 2 - 30, width(), 60, QColor(0, 0, 0, 210));
        painter.setPen(QColor(255, 60, 60));
        painter.setFont(QFont("Arial", 18, QFont::Bold));
        painter.drawText(rect(), Qt::AlignCenter, "GAME OVER! Press [R] to Restart");
    } else if (levelComplete) {
        painter.fillRect(0, ROWS * CELL_SIZE / 2 - 35, width(), 70, QColor(0, 0, 0, 210));
        painter.setPen(QColor(60, 255, 120));
        painter.setFont(QFont("Arial", 16, QFont::Bold));
        QString levelMsg = QString("LEVEL %1 COMPLETE! +500 Bonus").arg(currentLevel);
        painter.drawText(0, ROWS * CELL_SIZE / 2 - 10, width(), 20, Qt::AlignCenter, levelMsg);
        painter.setPen(QColor(255, 220, 100));
        painter.setFont(QFont("Arial", 12));
        painter.drawText(0, ROWS * CELL_SIZE / 2 + 12, width(), 20, Qt::AlignCenter,
                         "Press [Space] for Next Level (Ghosts get faster!)");
    }
}
