#include "mainwindow.h"
#include <QRandomGenerator>
#include <QVector>
#include <QPair>
#include <cmath>
#include <cstdlib>
#include <algorithm>

void MainWindow::updateFogOfWar()
{
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            double dist1 = (p1Alive ? std::hypot(c - pacmanX, r - pacmanY) : 999.0);
            double dist2 = (isMultiplayer && p2Alive ? std::hypot(c - p2X, r - p2Y) : 999.0);
            if (dist1 <= currentVisionRadius || dist2 <= currentVisionRadius) {
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
    return (dist1 <= currentVisionRadius || dist2 <= currentVisionRadius);
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
            scoreP1 += 10 + (currentLevel - 1) * 2;
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
            scoreP2 += 10 + (currentLevel - 1) * 2;
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
                frightenedTicksLeft = currentFrightenDuration;

                // Eating fruit grants +1 Fog Peek Charge!
                fogPeeksRemaining++;
                peekToastMessage = QString("🍒 FRUIT EATEN! +1 FOG PEEK EARNED! (Charges: %1 👁️)").arg(fogPeeksRemaining);
                peekToastTimer.start();
            }
        }
    }
}

