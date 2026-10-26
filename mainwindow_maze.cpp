#include "mainwindow.h"
#include <QRandomGenerator>
#include <QPoint>
#include <QVector>
#include <algorithm>

void MainWindow::getSectorDimsForLevel(int lvl, int &cols, int &rows) const
{
    int stageInWorld = (lvl - 1) % LEVELS_PER_WORLD + 1; // 1 to 20
    int world = (lvl - 1) / LEVELS_PER_WORLD + 1;

    if (world == 1) {
        if (stageInWorld == 1) { cols = 2; rows = 1; }      // Stage 1: 2 windows
        else if (stageInWorld == 2) { cols = 2; rows = 2; } // Stage 2: 4 windows (2x2)
        else if (stageInWorld == 3) { cols = 3; rows = 2; } // Stage 3: 6 windows (3x2)
        else if (stageInWorld == 4) { cols = 2; rows = 3; } // Stage 4: 6 vertical windows (2x3)
        else if (stageInWorld <= 6) { cols = 3; rows = 3; } // Stages 5-6: 9 windows (3x3)
        else if (stageInWorld <= 8) { cols = 4; rows = 3; } // Stages 7-8: 12 windows (4x3)
        else if (stageInWorld <= 10) { cols = 3; rows = 4; } // Stages 9-10: 12 windows (3x4)
        else if (stageInWorld <= 12) { cols = 4; rows = 4; } // Stages 11-12: 16 windows (4x4)
        else if (stageInWorld <= 14) { cols = 5; rows = 4; } // Stages 13-14: 20 windows (5x4)
        else if (stageInWorld <= 16) { cols = 4; rows = 5; } // Stages 15-16: 20 windows (4x5)
        else if (stageInWorld <= 18) { cols = 5; rows = 5; } // Stages 17-18: 25 windows (5x5)
        else if (stageInWorld == 19) { cols = 6; rows = 5; } // Stage 19: 30 windows (6x5)
        else { cols = 6; rows = 6; }                         // Stage 20: 36 windows (Summit Grand Vault!)
    } else {
        // Worlds 2, 3, etc. maintain grand layouts
        int cycle = stageInWorld % 4;
        if (cycle == 1) { cols = 5; rows = 5; }
        else if (cycle == 2) { cols = 6; rows = 5; }
        else if (cycle == 3) { cols = 5; rows = 6; }
        else { cols = 6; rows = 6; }
    }
    cols = std::clamp(cols, 1, MAX_SECTORS_X);
    rows = std::clamp(rows, 1, MAX_SECTORS_Y);
}

QString MainWindow::getWorldName(int world) const
{
    static const QString worldNames[] = {
        "Sweet Peppermint Plains",
        "Caramel Lava Caverns",
        "Gummy Bear Galaxy",
        "Cotton Candy Clouds",
        "Chocolate Chip Chasm",
        "Neon Licorice Labyrinth",
        "Frosted Marshmallow Mountain",
        "Sugar Plum Palace",
        "Bubblegum Bay",
        "Rainbow Lollipop Lagoon"
    };
    int count = sizeof(worldNames) / sizeof(worldNames[0]);
    if (world >= 1 && world <= count) {
        return QString("WORLD %1: %2").arg(world).arg(worldNames[world - 1]);
    }
    int idx = (world - 1) % count;
    return QString("WORLD %1: %2 (Ch.%3)").arg(world).arg(worldNames[idx]).arg((world - 1) / count + 1);
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

    // 4. Loop braiding: Dynamic wall removal (reduces at higher levels for tricky labyrinth turns!)
    int braidChance = std::clamp(32 - currentLevel, 14, 32);
    for (int r = 2; r < ROWS - 2; ++r) {
        for (int c = 2; c < COLS - 2; ++c) {
            if (board[sy][sx][r][c] == 1) {
                if (board[sy][sx][r-1][c] == 0 && board[sy][sx][r+1][c] == 0 &&
                    board[sy][sx][r][c-1] == 1 && board[sy][sx][r][c+1] == 1) {
                    if (QRandomGenerator::global()->bounded(100) < braidChance) {
                        board[sy][sx][r][c] = 0;
                    }
                } else if (board[sy][sx][r][c-1] == 0 && board[sy][sx][r][c+1] == 0 &&
                           board[sy][sx][r-1][c] == 1 && board[sy][sx][r+1][c] == 1) {
                    if (QRandomGenerator::global()->bounded(100) < braidChance) {
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

void MainWindow::checkLevelExit()
{
    bool p1Won = p1Alive && (pacmanX == exitX && pacmanY == exitY);
    bool p2Won = isMultiplayer && p2Alive && (p2X == exitX && p2Y == exitY);

    if (currentSectorX == exitSectorX && currentSectorY == exitSectorY && (p1Won || p2Won)) {
        levelComplete = true;
        int clearBonus = (300 + currentLevel * 100) * (sectorCols * sectorRows);
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
            highestUnlockedLevel = currentLevel + 1;
        }
        selectedMapLevel = currentLevel + 1;
        viewedMapWorld = (selectedMapLevel - 1) / LEVELS_PER_WORLD + 1;
        scrollToLevel(selectedMapLevel, false);
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

