#include "mainwindow.h"
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>

// 1 = Wall, 0 = Empty, 2 = Dot
int initialBoard[21][21] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,1},
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
    setFixedSize(COLS * CELL_SIZE, ROWS * CELL_SIZE + 45);
    setStyleSheet("background-color: black;");
    setWindowTitle("Pacman Maze");

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
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            board[r][c] = initialBoard[r][c];
        }
    }

    pacmanX = 10;
    pacmanY = 16;
    pacmanDir = LEFT;
    pacmanNextDir = LEFT;
    pacmanMouthAngle = 45;
    pacmanMouthOpening = false;

    score = 0;
    gameOver = false;
    gameWon = false;

    ghosts.clear();
    ghosts.push_back({10, 10, UP, 0});    // Blinky (Red)
    ghosts.push_back({9, 10, LEFT, 1});   // Pinky (Pink)
    ghosts.push_back({11, 10, RIGHT, 2}); // Inky (Cyan)
    ghosts.push_back({10, 9, DOWN, 3});   // Clyde (Orange)

    activeFruits.clear();
    fruitSpawnCounter = QRandomGenerator::global()->bounded(15, 40); // first fruit after 15-40 ticks
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (gameOver || gameWon) {
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_R) {
            initGame();
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
        checkFruitPickup();

        // Check ghost collisions
        for (const Ghost &g : ghosts) {
            if (g.x == pacmanX && g.y == pacmanY) {
                gameOver = true;
            }
        }

        // Check win
        bool dotsLeft = false;
        for (int r = 0; r < ROWS; ++r) {
            for (int c = 0; c < COLS; ++c) {
                if (board[r][c] == 2) dotsLeft = true;
            }
        }
        if (!dotsLeft) gameWon = true;

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
            int r = QRandomGenerator::global()->bounded(possibleDirs.size());
            g.dir = possibleDirs[r];
        }

        if (g.dir == UP) g.y--;
        else if (g.dir == DOWN) g.y++;
        else if (g.dir == LEFT) g.x--;
        else if (g.dir == RIGHT) g.x++;

        if (g.x < 0) g.x = COLS - 1;
        if (g.x >= COLS) g.x = 0;
    }
}

// ========================================================
// Bonus Fruit System
// ========================================================

void MainWindow::trySpawnFruit()
{
    fruitSpawnCounter--;
    if (fruitSpawnCounter > 0) return;

    // Reset timer for next spawn attempt (20-50 ticks)
    fruitSpawnCounter = QRandomGenerator::global()->bounded(20, 50);

    // Don't exceed max fruits on screen
    if (activeFruits.size() >= MAX_FRUITS) return;

    // Collect all empty, non-wall, non-dot cells as candidates
    QVector<QPair<int,int>> candidates;
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            if (board[r][c] == 0) {
                // Not on pacman
                if (c == pacmanX && r == pacmanY) continue;
                // Not on a ghost
                bool onGhost = false;
                for (const Ghost &g : ghosts) {
                    if (g.x == c && g.y == r) { onGhost = true; break; }
                }
                if (onGhost) continue;
                // Not on existing fruit
                bool onFruit = false;
                for (const BonusFruit &f : activeFruits) {
                    if (f.x == c && f.y == r) { onFruit = true; break; }
                }
                if (onFruit) continue;
                candidates.push_back({c, r});
            }
        }
    }

    if (candidates.isEmpty()) return;

    int idx = QRandomGenerator::global()->bounded(candidates.size());
    FruitType type = static_cast<FruitType>(QRandomGenerator::global()->bounded(4));
    int lifespan = QRandomGenerator::global()->bounded(40, 80); // fruit disappears after 40-80 ticks

    activeFruits.push_back({candidates[idx].first, candidates[idx].second, type, lifespan});
}

void MainWindow::checkFruitPickup()
{
    for (int i = activeFruits.size() - 1; i >= 0; --i) {
        if (activeFruits[i].x == pacmanX && activeFruits[i].y == pacmanY) {
            score += 100;
            activeFruits.removeAt(i);
        }
    }
}

// ========================================================
// Fruit Drawing Helpers (QPainter art)
// ========================================================

void MainWindow::drawCherry(QPainter &painter, int cx, int cy)
{
    int s = CELL_SIZE;
    // Stem
    painter.setPen(QPen(QColor(0, 160, 0), 2));
    painter.drawLine(cx + s/2, cy + 4, cx + s/2 - 4, cy + s/2 - 2);
    painter.drawLine(cx + s/2, cy + 4, cx + s/2 + 4, cy + s/2 - 2);
    // Left cherry
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(220, 20, 20));
    painter.drawEllipse(cx + s/2 - 9, cy + s/2 - 3, 10, 10);
    // Right cherry
    painter.setBrush(QColor(200, 10, 10));
    painter.drawEllipse(cx + s/2 + 1, cy + s/2 - 3, 10, 10);
    // Highlight
    painter.setBrush(QColor(255, 180, 180, 180));
    painter.drawEllipse(cx + s/2 - 7, cy + s/2, 3, 3);
    painter.drawEllipse(cx + s/2 + 3, cy + s/2, 3, 3);
}

void MainWindow::drawApple(QPainter &painter, int cx, int cy)
{
    int s = CELL_SIZE;
    // Body
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(50, 200, 50));
    painter.drawEllipse(cx + 5, cy + 7, s - 10, s - 10);
    // Highlight
    painter.setBrush(QColor(120, 255, 120, 150));
    painter.drawEllipse(cx + 8, cy + 9, 5, 5);
    // Stem
    painter.setPen(QPen(QColor(100, 60, 20), 2));
    painter.drawLine(cx + s/2, cy + 7, cx + s/2, cy + 3);
    // Leaf
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(30, 160, 30));
    painter.drawEllipse(cx + s/2, cy + 2, 6, 4);
}

void MainWindow::drawStrawberry(QPainter &painter, int cx, int cy)
{
    int s = CELL_SIZE;
    // Body (triangle-ish via polygon)
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(220, 30, 50));
    QPolygon strawberry;
    strawberry << QPoint(cx + s/2, cy + s - 5)
               << QPoint(cx + 5, cy + 8)
               << QPoint(cx + s - 5, cy + 8);
    painter.drawPolygon(strawberry);
    // Round top
    painter.drawEllipse(cx + 5, cy + 6, s - 10, 8);
    // Seeds
    painter.setBrush(QColor(255, 255, 100));
    painter.drawEllipse(cx + s/2 - 3, cy + 12, 2, 2);
    painter.drawEllipse(cx + s/2 + 2, cy + 14, 2, 2);
    painter.drawEllipse(cx + s/2 - 1, cy + 17, 2, 2);
    // Leaves
    painter.setBrush(QColor(30, 160, 30));
    painter.drawEllipse(cx + s/2 - 5, cy + 4, 5, 4);
    painter.drawEllipse(cx + s/2 + 1, cy + 4, 5, 4);
}

void MainWindow::drawOrangeFruit(QPainter &painter, int cx, int cy)
{
    int s = CELL_SIZE;
    // Body
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 160, 30));
    painter.drawEllipse(cx + 5, cy + 6, s - 10, s - 10);
    // Highlight
    painter.setBrush(QColor(255, 220, 120, 160));
    painter.drawEllipse(cx + 8, cy + 9, 5, 5);
    // Stem
    painter.setPen(QPen(QColor(100, 60, 20), 2));
    painter.drawLine(cx + s/2, cy + 6, cx + s/2, cy + 3);
    // Leaf
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(30, 160, 30));
    painter.drawEllipse(cx + s/2, cy + 1, 6, 4);
}

// ========================================================
// Game Loop
// ========================================================

void MainWindow::gameLoop()
{
    if (gameOver || gameWon) return;

    moveGhosts();

    // Check ghost collisions
    for (const Ghost &g : ghosts) {
        if (g.x == pacmanX && g.y == pacmanY) {
            gameOver = true;
        }
    }

    // Fruit spawning
    trySpawnFruit();

    // Tick down fruit lifespans and remove expired ones
    for (int i = activeFruits.size() - 1; i >= 0; --i) {
        activeFruits[i].ticksRemaining--;
        if (activeFruits[i].ticksRemaining <= 0) {
            activeFruits.removeAt(i);
        }
    }

    // Check win
    bool dotsLeft = false;
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            if (board[r][c] == 2) dotsLeft = true;
        }
    }
    if (!dotsLeft) gameWon = true;

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
                // Wall
                painter.setBrush(QColor(25, 45, 130));
                painter.setPen(QPen(QColor(40, 80, 200), 1));
                painter.drawRoundedRect(cx + 1, cy + 1, CELL_SIZE - 2, CELL_SIZE - 2, 4, 4);
            } else {
                // Background floor
                painter.fillRect(cx, cy, CELL_SIZE, CELL_SIZE, QColor(8, 8, 16));

                // Dot
                if (board[r][c] == 2) {
                    painter.setBrush(QColor(255, 230, 180));
                    painter.setPen(Qt::NoPen);
                    painter.drawEllipse(cx + CELL_SIZE/2 - 2, cy + CELL_SIZE/2 - 2, 5, 5);
                }
            }
        }
    }

    // 2. Draw Bonus Fruits
    for (const BonusFruit &fruit : activeFruits) {
        int fx = fruit.x * CELL_SIZE;
        int fy = fruit.y * CELL_SIZE;

        switch (fruit.type) {
            case CHERRY:       drawCherry(painter, fx, fy); break;
            case APPLE:        drawApple(painter, fx, fy); break;
            case STRAWBERRY:   drawStrawberry(painter, fx, fy); break;
            case ORANGE_FRUIT: drawOrangeFruit(painter, fx, fy); break;
        }

        // Draw "100" label below/above the fruit for clarity
        painter.setPen(QColor(255, 255, 100));
        painter.setFont(QFont("Arial", 7, QFont::Bold));
        painter.drawText(fx + 4, fy + CELL_SIZE - 1, "100");
    }

    // 3. Draw Pacman
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

    // 4. Draw Ghosts
    QColor ghostColors[] = {
        QColor(255, 50, 50),   // Blinky (Red)
        QColor(255, 180, 220), // Pinky (Pink)
        QColor(50, 220, 255),  // Inky (Cyan)
        QColor(255, 160, 50)   // Clyde (Orange)
    };

    for (const Ghost &g : ghosts) {
        int gx = g.x * CELL_SIZE;
        int gy = g.y * CELL_SIZE;

        painter.setBrush(ghostColors[g.colorIndex]);
        painter.setPen(Qt::NoPen);

        // Ghost head & body
        painter.drawEllipse(gx + 2, gy + 2, CELL_SIZE - 4, CELL_SIZE - 4);
        painter.drawRect(gx + 2, gy + CELL_SIZE/2, CELL_SIZE - 4, (CELL_SIZE - 4)/2);

        // Eyes
        painter.setBrush(Qt::white);
        painter.drawEllipse(gx + 6, gy + 8, 6, 6);
        painter.drawEllipse(gx + 16, gy + 8, 6, 6);
        painter.setBrush(Qt::blue);
        painter.drawEllipse(gx + 8, gy + 10, 3, 3);
        painter.drawEllipse(gx + 18, gy + 10, 3, 3);
    }

    // 5. Draw HUD
    int hudY = ROWS * CELL_SIZE;
    painter.fillRect(0, hudY, width(), height() - hudY, QColor(14, 16, 26));

    // Divider line
    painter.setPen(QPen(QColor(40, 60, 100), 2));
    painter.drawLine(0, hudY, width(), hudY);

    // Score
    painter.setFont(QFont("Arial", 14, QFont::Bold));
    painter.setPen(Qt::white);
    painter.drawText(12, hudY + 28, QString("SCORE: %1").arg(score));

    // Controls hint
    painter.setFont(QFont("Arial", 9));
    painter.setPen(QColor(130, 150, 180));
    painter.drawText(200, hudY + 28, "[Arrow Keys] Move   |   [R] Reset");

    // Fruit legend
    painter.setPen(QColor(255, 200, 100));
    painter.setFont(QFont("Arial", 9));
    int legendX = width() - 145;
    painter.drawText(legendX, hudY + 28, "Fruits = 100 pts!");

    // 6. Game Over / Win Banner
    if (gameOver) {
        painter.fillRect(0, ROWS * CELL_SIZE / 2 - 30, width(), 60, QColor(0, 0, 0, 200));
        painter.setPen(QColor(255, 60, 60));
        painter.setFont(QFont("Arial", 18, QFont::Bold));
        painter.drawText(rect(), Qt::AlignCenter, "GAME OVER! Press [R] to Restart");
    } else if (gameWon) {
        painter.fillRect(0, ROWS * CELL_SIZE / 2 - 30, width(), 60, QColor(0, 0, 0, 200));
        painter.setPen(QColor(60, 255, 120));
        painter.setFont(QFont("Arial", 18, QFont::Bold));
        painter.drawText(rect(), Qt::AlignCenter, "ALL PELLETS CLEARED! Press [R] to Restart");
    }
}
