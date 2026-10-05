#include "mainwindow.h"
#include <QPainter>
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
    setFixedSize(COLS * CELL_SIZE, ROWS * CELL_SIZE + 40); // 40 for score
    setStyleSheet("background-color: black;");
    
    initGame();
    
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::gameLoop);
    timer->start(150); // Speed of game
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
    ghosts.push_back({10, 10, UP, 0});   // Red
    ghosts.push_back({9, 10, LEFT, 1});  // Pink
    ghosts.push_back({11, 10, RIGHT, 2});// Cyan
    ghosts.push_back({10, 9, DOWN, 3});  // Orange
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (gameOver || gameWon) {
        if (event->key() == Qt::Key_Space) {
            initGame();
        }
        return;
    }

    Direction attemptDir = STOP;
    switch (event->key()) {
        case Qt::Key_Up:    attemptDir = UP; break;
        case Qt::Key_Down:  attemptDir = DOWN; break;
        case Qt::Key_Left:  attemptDir = LEFT; break;
        case Qt::Key_Right: attemptDir = RIGHT; break;
        default: QWidget::keyPressEvent(event); return;
    }
    
    if (attemptDir != STOP) {
        pacmanNextDir = attemptDir;
        movePacman();
        
        // Check collisions immediately after Pacman moves
        for (const Ghost &g : ghosts) {
            if (g.x == pacmanX && g.y == pacmanY) {
                gameOver = true;
            }
        }
        
        // Check win immediately
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
    // Try to move in nextDir
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
        
        // Don't reverse direction unless hitting a wall
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
            // Must reverse
            if (canMove(g.x, g.y, reverseDir)) g.dir = reverseDir;
        } else {
            // Pick random valid direction (not reverse)
            int r = QRandomGenerator::global()->bounded(possibleDirs.size());
            g.dir = possibleDirs[r];
        }
        
        // Actually move
        if (g.dir == UP) g.y--;
        else if (g.dir == DOWN) g.y++;
        else if (g.dir == LEFT) g.x--;
        else if (g.dir == RIGHT) g.x++;
        
        // Tunnels
        if (g.x < 0) g.x = COLS - 1;
        if (g.x >= COLS) g.x = 0;
    }
}

void MainWindow::gameLoop()
{
    if (gameOver || gameWon) return;
    
    // Pacman is moved manually via keyPressEvent now
    moveGhosts();
    
    // Check collisions after ghosts move
    for (const Ghost &g : ghosts) {
        if (g.x == pacmanX && g.y == pacmanY) {
            gameOver = true;
        }
    }
    
    update(); // Trigger paintEvent to animate ghosts
}

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw Maze
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            if (board[r][c] == 1) {
                painter.setBrush(Qt::blue);
                painter.setPen(Qt::NoPen);
                painter.drawRect(c * CELL_SIZE, r * CELL_SIZE, CELL_SIZE, CELL_SIZE);
            } else if (board[r][c] == 2) {
                painter.setBrush(Qt::white);
                painter.setPen(Qt::NoPen);
                painter.drawEllipse(c * CELL_SIZE + CELL_SIZE/2 - 3, r * CELL_SIZE + CELL_SIZE/2 - 3, 6, 6);
            }
        }
    }
    
    // Draw Pacman
    painter.setBrush(Qt::yellow);
    painter.setPen(Qt::NoPen);
    int startAngle = 0;
    if (pacmanDir == RIGHT) startAngle = pacmanMouthAngle;
    else if (pacmanDir == DOWN) startAngle = 270 + pacmanMouthAngle;
    else if (pacmanDir == LEFT) startAngle = 180 + pacmanMouthAngle;
    else if (pacmanDir == UP) startAngle = 90 + pacmanMouthAngle;
    
    int spanAngle = 360 - 2 * pacmanMouthAngle;
    painter.drawPie(pacmanX * CELL_SIZE, pacmanY * CELL_SIZE, CELL_SIZE, CELL_SIZE, startAngle * 16, spanAngle * 16);
    
    // Draw Ghosts
    QColor ghostColors[] = {Qt::red, QColor(255, 184, 255), Qt::cyan, QColor(255, 184, 82)};
    for (const Ghost &g : ghosts) {
        painter.setBrush(ghostColors[g.colorIndex]);
        painter.setPen(Qt::NoPen);
        
        int gx = g.x * CELL_SIZE;
        int gy = g.y * CELL_SIZE;
        
        // Ghost body
        painter.drawEllipse(gx, gy, CELL_SIZE, CELL_SIZE);
        painter.drawRect(gx, gy + CELL_SIZE/2, CELL_SIZE, CELL_SIZE/2);
        
        // Eyes
        painter.setBrush(Qt::white);
        painter.drawEllipse(gx + 6, gy + 8, 8, 8);
        painter.drawEllipse(gx + 16, gy + 8, 8, 8);
        painter.setBrush(Qt::blue);
        painter.drawEllipse(gx + 8, gy + 10, 4, 4);
        painter.drawEllipse(gx + 18, gy + 10, 4, 4);
    }
    
    // Score
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 16, QFont::Bold));
    painter.drawText(10, ROWS * CELL_SIZE + 25, QString("SCORE: %1").arg(score));
    
    if (gameOver) {
        painter.setPen(Qt::red);
        painter.drawText(150, ROWS * CELL_SIZE + 25, "GAME OVER! (Space to Restart)");
    } else if (gameWon) {
        painter.setPen(Qt::green);
        painter.drawText(150, ROWS * CELL_SIZE + 25, "YOU WIN! (Space to Restart)");
    }
}
