import re

with open('mainwindow.cpp', 'r') as f:
    content = f.read()

# 1. Update keyPressEvent
old_keypress = """void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (gameOver || gameWon) {
        if (event->key() == Qt::Key_Space) {
            initGame();
        }
        return;
    }

    switch (event->key()) {
        case Qt::Key_Up:    pacmanNextDir = UP; break;
        case Qt::Key_Down:  pacmanNextDir = DOWN; break;
        case Qt::Key_Left:  pacmanNextDir = LEFT; break;
        case Qt::Key_Right: pacmanNextDir = RIGHT; break;
        default: QWidget::keyPressEvent(event);
    }
}"""

new_keypress = """void MainWindow::keyPressEvent(QKeyEvent *event)
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
}"""

content = content.replace(old_keypress, new_keypress)

# 2. Update gameLoop to remove movePacman()
old_gameloop = """void MainWindow::gameLoop()
{
    if (gameOver || gameWon) return;
    
    movePacman();
    moveGhosts();
    
    // Check collisions
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
    
    update(); // Trigger paintEvent
}"""

new_gameloop = """void MainWindow::gameLoop()
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
}"""

content = content.replace(old_gameloop, new_gameloop)

with open('mainwindow.cpp', 'w') as f:
    f.write(content)
