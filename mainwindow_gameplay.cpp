#include "mainwindow.h"
#include <QRandomGenerator>
#include <QVector>
#include <QPair>
#include <cmath>
#include <cstdlib>
#include <algorithm>

void MainWindow::updateFogOfWar()
{
    int hostGIdx = isHostGhost() ? getHostGhostIndex() : -1;
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            double dist1 = (isP1Active() && p1Alive ? std::hypot(c - pacmanX, r - pacmanY) : 999.0);
            double dist2 = (isP2Active() && p2Alive ? std::hypot(c - p2X, r - p2Y) : 999.0);
            double dist3 = (isP3Active() && hero3.alive ? std::hypot(c - hero3.x, r - hero3.y) : 999.0);
            double dist4 = (isP4Active() && hero4.alive ? std::hypot(c - hero4.x, r - hero4.y) : 999.0);
            double distG = (hostGIdx >= 0 && hostGIdx < ghosts.size() ? std::hypot(c - ghosts[hostGIdx].x, r - ghosts[hostGIdx].y) : 999.0);
            if (dist1 <= currentVisionRadius || dist2 <= currentVisionRadius ||
                dist3 <= currentVisionRadius || dist4 <= currentVisionRadius || distG <= (currentVisionRadius + 0.8)) {
                explored[currentSectorY][currentSectorX][r][c] = true;
            }
        }
    }
}

bool MainWindow::isCellVisible(int sx, int sy, int x, int y) const
{
    if (!fogOfWarEnabled) return true;
    if (sx != currentSectorX || sy != currentSectorY) return false;
    int hostGIdx = isHostGhost() ? getHostGhostIndex() : -1;
    double dist1 = (isP1Active() && p1Alive ? std::hypot(x - pacmanX, y - pacmanY) : 999.0);
    double dist2 = (isP2Active() && p2Alive ? std::hypot(x - p2X, y - p2Y) : 999.0);
    double dist3 = (isP3Active() && hero3.alive ? std::hypot(x - hero3.x, y - hero3.y) : 999.0);
    double dist4 = (isP4Active() && hero4.alive ? std::hypot(x - hero4.x, y - hero4.y) : 999.0);
    double distG = (hostGIdx >= 0 && hostGIdx < ghosts.size() ? std::hypot(x - ghosts[hostGIdx].x, y - ghosts[hostGIdx].y) : 999.0);
    return (dist1 <= currentVisionRadius || dist2 <= currentVisionRadius ||
            dist3 <= currentVisionRadius || dist4 <= currentVisionRadius || distG <= (currentVisionRadius + 0.8));
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
            if (isP2Active()) {
                p2X = 0; p2Y = (pacmanY == 9 ? 10 : 9);
            }
            if (hero3.clientId != -1) { hero3.x = 0; hero3.y = 8; }
            if (hero4.clientId != -1) { hero4.x = 0; hero4.y = 11; }
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
            if (isP2Active()) {
                p2X = COLS - 1; p2Y = (pacmanY == 9 ? 10 : 9);
            }
            if (hero3.clientId != -1) { hero3.x = COLS - 1; hero3.y = 8; }
            if (hero4.clientId != -1) { hero4.x = COLS - 1; hero4.y = 11; }
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
            if (isP2Active()) {
                p2Y = 0; p2X = (pacmanX == 9 ? 10 : 9);
            }
            if (hero3.clientId != -1) { hero3.y = 0; hero3.x = 8; }
            if (hero4.clientId != -1) { hero4.y = 0; hero4.x = 11; }
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
            if (isP2Active()) {
                p2Y = ROWS - 1; p2X = (pacmanX == 9 ? 10 : 9);
            }
            if (hero3.clientId != -1) { hero3.y = ROWS - 1; hero3.x = 8; }
            if (hero4.clientId != -1) { hero4.y = ROWS - 1; hero4.x = 11; }
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
            score = scoreP1 + scoreP2 + hero3.score + hero4.score;
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

        if (nextX >= COLS) {
            fromSectorX = currentSectorX; fromSectorY = currentSectorY;
            toSectorX = currentSectorX + 1; toSectorY = currentSectorY;
            transitionDirX = 1; transitionDirY = 0;
            currentSectorX = toSectorX;
            p2X = 0; pacmanX = 0; pacmanY = (p2Y == 9 ? 10 : 9);
            if (hero3.clientId != -1) { hero3.x = 0; hero3.y = 8; }
            if (hero4.clientId != -1) { hero4.x = 0; hero4.y = 11; }
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
            p2X = COLS - 1; pacmanX = COLS - 1; pacmanY = (p2Y == 9 ? 10 : 9);
            if (hero3.clientId != -1) { hero3.x = COLS - 1; hero3.y = 8; }
            if (hero4.clientId != -1) { hero4.x = COLS - 1; hero4.y = 11; }
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
            p2Y = 0; pacmanY = 0; pacmanX = (p2X == 9 ? 10 : 9);
            if (hero3.clientId != -1) { hero3.y = 0; hero3.x = 8; }
            if (hero4.clientId != -1) { hero4.y = 0; hero4.x = 11; }
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
            p2Y = ROWS - 1; pacmanY = ROWS - 1; pacmanX = (p2X == 9 ? 10 : 9);
            if (hero3.clientId != -1) { hero3.y = ROWS - 1; hero3.x = 8; }
            if (hero4.clientId != -1) { hero4.y = ROWS - 1; hero4.x = 11; }
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
            score = scoreP1 + scoreP2 + hero3.score + hero4.score;
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

void MainWindow::moveHero(ExtraHero &hero, int heroId)
{
    Q_UNUSED(heroId);
    if (!hero.alive || hero.clientId == -1) return;

    if (canMove(hero.x, hero.y, hero.nextDir)) {
        hero.dir = hero.nextDir;
    }

    if (canMove(hero.x, hero.y, hero.dir)) {
        int nextX = hero.x, nextY = hero.y;
        if (hero.dir == UP) nextY--;
        else if (hero.dir == DOWN) nextY++;
        else if (hero.dir == LEFT) nextX--;
        else if (hero.dir == RIGHT) nextX++;

        if (nextX >= COLS) {
            fromSectorX = currentSectorX; fromSectorY = currentSectorY;
            toSectorX = currentSectorX + 1; toSectorY = currentSectorY;
            transitionDirX = 1; transitionDirY = 0;
            currentSectorX = toSectorX;
            hero.x = 0; pacmanX = 0; p2X = 0;
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
            hero.x = COLS - 1; pacmanX = COLS - 1; p2X = COLS - 1;
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
            hero.y = 0; pacmanY = 0; p2Y = 0;
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
            hero.y = ROWS - 1; pacmanY = ROWS - 1; p2Y = ROWS - 1;
            isTransitioning = true;
            transitionTimer.start();
            if (!sectorDiscovered[currentSectorY][currentSectorX]) {
                newSectorJustDiscovered = true;
                newSectorTimer.start();
            }
            sectorDiscovered[currentSectorY][currentSectorX] = true;
        } else {
            hero.x = nextX;
            hero.y = nextY;
        }

        updateFogOfWar();

        // Eat dot
        if (board[currentSectorY][currentSectorX][hero.y][hero.x] == 2) {
            board[currentSectorY][currentSectorX][hero.y][hero.x] = 0;
            hero.score += 10 + (currentLevel - 1) * 2;
            score = scoreP1 + scoreP2 + hero3.score + hero4.score;
        }

        // Mouth animation
        if (hero.mouthOpening) {
            hero.mouthAngle += 15;
            if (hero.mouthAngle >= 45) hero.mouthOpening = false;
        } else {
            hero.mouthAngle -= 15;
            if (hero.mouthAngle <= 0) hero.mouthOpening = true;
        }
    }
}

void MainWindow::moveGhosts()
{
    int hostGIdx = isHostGhost() ? getHostGhostIndex() : -1;
    for (int gi = 0; gi < ghosts.size(); ++gi) {
        Ghost &g = ghosts[gi];
        if (ghostsFrightened && QRandomGenerator::global()->bounded(3) == 0) {
            continue;
        }

        // Mode B: Ghost controlled by human player via phone/laptop or host PC
        if (g.clientId >= 0 || (hostGIdx == gi)) {
            continue; // controlled directly by player input!
        }

        // Mode A / AI: Ghost controlled by smart navigation algorithm
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
            // Target nearest living hero
            int targetX = pacmanX;
            int targetY = pacmanY;
            int bestDistToHero = 9999;

            auto checkHeroDist = [&](bool alive, int hx, int hy) {
                if (alive) {
                    int d = std::abs(g.x - hx) + std::abs(g.y - hy);
                    if (d < bestDistToHero) {
                        bestDistToHero = d;
                        targetX = hx;
                        targetY = hy;
                    }
                }
            };

            checkHeroDist(p1Alive, pacmanX, pacmanY);
            checkHeroDist(isP2Active() && p2Alive, p2X, p2Y);
            checkHeroDist(hero3.clientId != -1 && hero3.alive, hero3.x, hero3.y);
            checkHeroDist(hero4.clientId != -1 && hero4.alive, hero4.x, hero4.y);

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

        // Player 1 (Yellow Pac-Man)
        if (isP1Active() && p1Alive && g.x == pacmanX && g.y == pacmanY) {
            if (ghostsFrightened) {
                scoreP1 += 200;
                score = scoreP1 + scoreP2 + hero3.score + hero4.score;
                respawnGhostFarAway(g);
            } else {
                p1Alive = false;
                p1RespawnTicks = 30;
            }
        }

        // Player 2 (Ms. Pac-Man)
        if (isP2Active() && p2Alive && g.x == p2X && g.y == p2Y) {
            if (ghostsFrightened) {
                scoreP2 += 200;
                score = scoreP1 + scoreP2 + hero3.score + hero4.score;
                respawnGhostFarAway(g);
            } else {
                p2Alive = false;
                p2RespawnTicks = 30;
            }
        }

        // Hero 3 (Jr. Pac-Man)
        if (isP3Active() && hero3.alive && g.x == hero3.x && g.y == hero3.y) {
            if (ghostsFrightened) {
                hero3.score += 200;
                score = scoreP1 + scoreP2 + hero3.score + hero4.score;
                respawnGhostFarAway(g);
            } else {
                hero3.alive = false;
                hero3.respawnTicks = 30;
            }
        }

        // Hero 4 (Miracle Pac-Man)
        if (isP4Active() && hero4.alive && g.x == hero4.x && g.y == hero4.y) {
            if (ghostsFrightened) {
                hero4.score += 200;
                score = scoreP1 + scoreP2 + hero3.score + hero4.score;
                respawnGhostFarAway(g);
            } else {
                hero4.alive = false;
                hero4.respawnTicks = 30;
            }
        }
    }

    // Check if entire active hero team has been defeated
    bool anyHeroConfigured = isP1Active() || isP2Active() || isP3Active() || isP4Active();

    bool anyHeroAlive = (isP1Active() && p1Alive) ||
                        (isP2Active() && p2Alive) ||
                        (isP3Active() && hero3.alive) ||
                        (isP4Active() && hero4.alive);

    bool anyHeroSpawning = (isP1Active() && p1RespawnTicks > 0) ||
                           (isP2Active() && p2RespawnTicks > 0) ||
                           (isP3Active() && hero3.respawnTicks > 0) ||
                           (isP4Active() && hero4.respawnTicks > 0);

    if (anyHeroConfigured && !anyHeroAlive && !anyHeroSpawning) {
        gameOver = true;
    }
}

void MainWindow::respawnGhostFarAway(Ghost &g)
{
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

void MainWindow::trySpawnFruit()
{
    fruitSpawnCounter--;
    if (fruitSpawnCounter > 0) return;

    fruitSpawnCounter = QRandomGenerator::global()->bounded(20, 50);
    if (activeFruits.size() >= MAX_FRUITS) return;

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
            bool p2Picked = isP2Active() && p2Alive && (activeFruits[i].x == p2X && activeFruits[i].y == p2Y);
            bool p3Picked = (hero3.clientId != -1 && hero3.alive && activeFruits[i].x == hero3.x && activeFruits[i].y == hero3.y);
            bool p4Picked = (hero4.clientId != -1 && hero4.alive && activeFruits[i].x == hero4.x && activeFruits[i].y == hero4.y);

            if (p1Picked || p2Picked || p3Picked || p4Picked) {
                QString eaterName = "Hero";
                if (p1Picked) {
                    scoreP1 += 100;
                    p1FogPeeks++;
                    fogPeeksRemaining = p1FogPeeks;
                    eaterName = "Pac-Man (Host)";
                }
                if (p2Picked) {
                    scoreP2 += 100;
                    p2FogPeeks++;
                    eaterName = p2PlayerName.isEmpty() ? "Ms. Pac-Man" : p2PlayerName;
                }
                if (p3Picked) {
                    hero3.score += 100;
                    hero3FogPeeks++;
                    eaterName = hero3.playerName.isEmpty() ? "Jr. Pac-Man" : hero3.playerName;
                }
                if (p4Picked) {
                    hero4.score += 100;
                    hero4FogPeeks++;
                    eaterName = hero4.playerName.isEmpty() ? "Miracle Pac-Man" : hero4.playerName;
                }

                score = scoreP1 + scoreP2 + hero3.score + hero4.score;
                activeFruits.removeAt(i);

                ghostsFrightened = true;
                frightenedTicksLeft = currentFrightenDuration;

                peekToastMessage = QString("🍒 %1 ATE FRUIT! +1 FOG PEEK EARNED!").arg(eaterName);
                peekToastTimer.start();
                broadcastCurrentGameState();
            }
        }
    }
}
