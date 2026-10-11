#include "mainwindow.h"
#include <QPainter>
#include <QPainterPath>
#include <QTime>
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
    // Window layout: Grid + bottom HUD area (85 px)
    setFixedSize(COLS * CELL_SIZE, ROWS * CELL_SIZE + 85);
    setStyleSheet("background-color: #0b0b12;");
    setWindowTitle("Pacman Maze Solver - Strategy Pattern (BFS / A* / DFS)");

    // Initialize Strategy Pattern Solvers
    solvers.push_back(std::make_shared<BFSSolver>());
    solvers.push_back(std::make_shared<AStarSolver>());
    solvers.push_back(std::make_shared<DFSSolver>());
    currentSolverIndex = 0; // Default to BFS (Shortest Path)

    // Goal at (1, 1) by default
    goalPoint = {1, 1};
    autoSolveMode = false;
    showExplored = true;
    ghostsActive = true;

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

    recalculatePath();
}

void MainWindow::recalculatePath()
{
    if (currentSolverIndex >= 0 && currentSolverIndex < solvers.size()) {
        currentResult = solvers[currentSolverIndex]->solve(board, {pacmanX, pacmanY}, goalPoint);
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    int c = event->pos().x() / CELL_SIZE;
    int r = event->pos().y() / CELL_SIZE;

    // Check if clicked inside board
    if (r >= 0 && r < ROWS && c >= 0 && c < COLS) {
        if (board[r][c] != 1) { // Cannot set a wall as goal
            goalPoint = {c, r};
            recalculatePath();
            update();
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (gameOver || gameWon) {
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_R) {
            initGame();
        }
        return;
    }

    // Toggle Auto-Solve mode
    if (event->key() == Qt::Key_S) {
        autoSolveMode = !autoSolveMode;
        recalculatePath();
        update();
        return;
    }

    // Cycle through Maze Solver Strategies (BFS -> A* -> DFS)
    if (event->key() == Qt::Key_A) {
        currentSolverIndex = (currentSolverIndex + 1) % solvers.size();
        recalculatePath();
        update();
        return;
    }

    // Toggle Explored Nodes Visualization
    if (event->key() == Qt::Key_E) {
        showExplored = !showExplored;
        update();
        return;
    }

    // Toggle Ghosts Active / Frozen
    if (event->key() == Qt::Key_G) {
        ghostsActive = !ghostsActive;
        update();
        return;
    }

    // Reset board
    if (event->key() == Qt::Key_R) {
        initGame();
        update();
        return;
    }

    // Manual Arrow movement (switches Auto-Solve off to give player control)
    Direction attemptDir = STOP;
    switch (event->key()) {
        case Qt::Key_Up:    attemptDir = UP; break;
        case Qt::Key_Down:  attemptDir = DOWN; break;
        case Qt::Key_Left:  attemptDir = LEFT; break;
        case Qt::Key_Right: attemptDir = RIGHT; break;
        default: QWidget::keyPressEvent(event); return;
    }

    if (attemptDir != STOP) {
        autoSolveMode = false;
        pacmanNextDir = attemptDir;
        movePacman();
        recalculatePath();

        // Check collisions immediately after Pacman moves
        for (const Ghost &g : ghosts) {
            if (g.x == pacmanX && g.y == pacmanY && ghostsActive) {
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

void MainWindow::stepAutoSolve()
{
    if (!currentResult.found || currentResult.path.size() <= 1) {
        // Reached goal or no path
        return;
    }

    // Next step along solved path
    Point next = currentResult.path[1];

    // Determine direction for sprite rotation
    int dx = next.x - pacmanX;
    int dy = next.y - pacmanY;

    if (dx == 1 || (pacmanX == COLS - 1 && next.x == 0)) {
        pacmanDir = RIGHT;
    } else if (dx == -1 || (pacmanX == 0 && next.x == COLS - 1)) {
        pacmanDir = LEFT;
    } else if (dy == 1) {
        pacmanDir = DOWN;
    } else if (dy == -1) {
        pacmanDir = UP;
    }

    pacmanX = next.x;
    pacmanY = next.y;

    // Eat dot if present
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

    recalculatePath();
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

void MainWindow::gameLoop()
{
    if (gameOver || gameWon) return;

    if (autoSolveMode) {
        stepAutoSolve();
    }

    if (ghostsActive) {
        moveGhosts();
    }

    // Check collisions
    for (const Ghost &g : ghosts) {
        if (g.x == pacmanX && g.y == pacmanY && ghostsActive) {
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

    // 2. Draw Explored Nodes (Search Tree Exploration visualization)
    if (showExplored && !currentResult.exploredOrder.isEmpty()) {
        painter.setBrush(QColor(0, 180, 220, 45));
        painter.setPen(Qt::NoPen);
        for (const Point &pt : currentResult.exploredOrder) {
            if (board[pt.y][pt.x] != 1 && (pt.x != pacmanX || pt.y != pacmanY)) {
                painter.drawRect(pt.x * CELL_SIZE + 2, pt.y * CELL_SIZE + 2, CELL_SIZE - 4, CELL_SIZE - 4);
            }
        }
    }

    // 3. Draw Solved Path
    if (currentResult.found && currentResult.path.size() > 1) {
        // Glowing path line
        QPen pathGlowPen(QColor(0, 255, 140, 120), 8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        QPen pathCorePen(QColor(180, 255, 220), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

        for (int i = 0; i < currentResult.path.size() - 1; ++i) {
            Point p1 = currentResult.path[i];
            Point p2 = currentResult.path[i+1];

            // Avoid drawing path lines across horizontal wrapping tunnels
            if (std::abs(p1.x - p2.x) > 1) continue;

            int x1 = p1.x * CELL_SIZE + CELL_SIZE / 2;
            int y1 = p1.y * CELL_SIZE + CELL_SIZE / 2;
            int x2 = p2.x * CELL_SIZE + CELL_SIZE / 2;
            int y2 = p2.y * CELL_SIZE + CELL_SIZE / 2;

            painter.setPen(pathGlowPen);
            painter.drawLine(x1, y1, x2, y2);
            painter.setPen(pathCorePen);
            painter.drawLine(x1, y1, x2, y2);
        }

        // Draw small waypoint nodes along the path
        painter.setBrush(QColor(255, 255, 100, 200));
        painter.setPen(Qt::NoPen);
        for (const Point &pt : currentResult.path) {
            if (pt != goalPoint && (pt.x != pacmanX || pt.y != pacmanY)) {
                int cx = pt.x * CELL_SIZE + CELL_SIZE / 2;
                int cy = pt.y * CELL_SIZE + CELL_SIZE / 2;
                painter.drawEllipse(cx - 3, cy - 3, 6, 6);
            }
        }
    }

    // 4. Draw Goal Marker (Target Destination)
    {
        int gx = goalPoint.x * CELL_SIZE;
        int gy = goalPoint.y * CELL_SIZE;

        // Outer pulsing ring
        painter.setBrush(QColor(255, 50, 80, 80));
        painter.setPen(QPen(QColor(255, 60, 100), 2));
        painter.drawEllipse(gx + 3, gy + 3, CELL_SIZE - 6, CELL_SIZE - 6);

        // Inner bullseye
        painter.setBrush(QColor(255, 220, 0));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(gx + CELL_SIZE/2 - 4, gy + CELL_SIZE/2 - 4, 8, 8);

        // Flag / Target text
        painter.setPen(QColor(255, 255, 255));
        painter.setFont(QFont("Arial", 8, QFont::Bold));
        painter.drawText(gx + 2, gy + 11, "GOAL");
    }

    // 5. Draw Pacman (Maze Solver Agent)
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

    // 6. Draw Ghosts
    QColor ghostColors[] = {
        QColor(255, 50, 50),   // Blinky (Red)
        QColor(255, 180, 220), // Pinky (Pink)
        QColor(50, 220, 255),  // Inky (Cyan)
        QColor(255, 160, 50)   // Clyde (Orange)
    };

    for (const Ghost &g : ghosts) {
        int gx = g.x * CELL_SIZE;
        int gy = g.y * CELL_SIZE;

        if (!ghostsActive) {
            // Frozen ghost visual indicator
            painter.setBrush(QColor(100, 120, 160, 160));
        } else {
            painter.setBrush(ghostColors[g.colorIndex]);
        }
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

    // 7. Draw HUD (Bottom Control & Strategy Pattern Information Panel)
    int hudY = ROWS * CELL_SIZE;
    painter.fillRect(0, hudY, width(), height() - hudY, QColor(14, 16, 26));

    // Divider line
    painter.setPen(QPen(QColor(40, 60, 100), 2));
    painter.drawLine(0, hudY, width(), hudY);

    // Line 1: Algorithm & Mode
    painter.setFont(QFont("Arial", 11, QFont::Bold));
    painter.setPen(Qt::white);
    painter.drawText(12, hudY + 22, QString("SCORE: %1").arg(score));

    // Active strategy name & description
    QString algoName = solvers[currentSolverIndex]->name();
    QString algoDesc = solvers[currentSolverIndex]->description();
    painter.setPen(QColor(0, 230, 160));
    painter.drawText(120, hudY + 22, QString("STRATEGY: %1 (%2)").arg(algoName).arg(algoDesc));

    // Status pill
    if (autoSolveMode) {
        painter.setPen(QColor(255, 210, 0));
        painter.drawText(width() - 175, hudY + 22, "[AUTO-SOLVE: ON]");
    } else {
        painter.setPen(QColor(160, 170, 190));
        painter.drawText(width() - 175, hudY + 22, "[MANUAL MODE]");
    }

    // Line 2: Solver stats
    painter.setFont(QFont("Arial", 10));
    painter.setPen(QColor(190, 210, 235));
    int pathSteps = currentResult.found ? currentResult.path.size() - 1 : 0;
    int exploredCount = currentResult.exploredOrder.size();
    QString ghostStatus = ghostsActive ? "Active" : "Frozen";
    painter.drawText(12, hudY + 45,
                     QString("Path Length: %1 steps   |   Visited Nodes: %2   |   Ghosts: %3   |   Explored Vis: %4")
                     .arg(pathSteps)
                     .arg(exploredCount)
                     .arg(ghostStatus)
                     .arg(showExplored ? "ON" : "OFF"));

    // Line 3: Controls Guide
    painter.setFont(QFont("Arial", 9));
    painter.setPen(QColor(130, 150, 180));
    painter.drawText(12, hudY + 68,
                     "Controls: [Click Maze] Set Goal  |  [S] Auto-Solve  |  [A] Switch Strategy  |  [G] Ghosts  |  [E] Explored  |  [Arrows] Move  |  [R] Reset");

    // 8. Game Over / Win Banner
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
    } else if (pacmanX == goalPoint.x && pacmanY == goalPoint.y && autoSolveMode) {
        // Goal reached banner (brief indicator)
        painter.setPen(QColor(255, 220, 0));
        painter.setFont(QFont("Arial", 10, QFont::Bold));
        painter.drawText(width() - 320, hudY + 22, "★ GOAL REACHED!");
    }
}
