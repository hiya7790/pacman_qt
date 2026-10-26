#include "mainwindow.h"
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QCursor>
#include <cmath>
#include <algorithm>

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
    for (const BonusFruit &fruit : std::as_const(activeFruits)) {
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

    // 5. Draw Primary Hero / Player 1 (if in this sector)
    if (isCurrent && (p1Alive || (p1RespawnTicks > 0 && p1RespawnTicks % 4 < 2))) {
        int px = originX + pacmanX * cellSize;
        int py = originY + pacmanY * cellSize;

        bool isPinkSolo = (!isMultiplayer && singlePlayerHero == 2);

        if (isPinkSolo) {
            // Drawn as Pink Ms. Pacman in solo mode
            painter.setBrush(p1Alive ? QColor(255, 105, 180) : QColor(255, 150, 200, 110));
        } else {
            painter.setBrush(p1Alive ? QColor(255, 220, 0) : QColor(255, 240, 100, 110));
        }
        painter.setPen(Qt::NoPen);
        int startAngle = 0;
        if (pacmanDir == RIGHT) startAngle = pacmanMouthAngle;
        else if (pacmanDir == DOWN) startAngle = 270 + pacmanMouthAngle;
        else if (pacmanDir == LEFT) startAngle = 180 + pacmanMouthAngle;
        else if (pacmanDir == UP) startAngle = 90 + pacmanMouthAngle;

        int spanAngle = 360 - 2 * pacmanMouthAngle;
        painter.drawPie(px + 2, py + 2, cellSize - 4, cellSize - 4, startAngle * 16, spanAngle * 16);

        if (isPinkSolo) {
            // Cute Red Ribbon Bow on head
            painter.setBrush(QColor(235, 20, 60));
            int bowSize = std::max(4, cellSize / 4);
            int bx = px + cellSize / 2;
            int by = py + 2;
            painter.drawEllipse(bx - bowSize, by, bowSize * 3 / 4, bowSize / 2);
            painter.drawEllipse(bx, by, bowSize * 3 / 4, bowSize / 2);
            painter.setBrush(QColor(255, 220, 230));
            painter.drawEllipse(bx - bowSize / 4, by + bowSize / 6, bowSize / 2, bowSize / 2);

            // Eye beauty mark
            painter.setBrush(QColor(20, 20, 60));
            painter.drawEllipse(px + cellSize * 3 / 5, py + cellSize / 3, std::max(2, cellSize / 7), std::max(2, cellSize / 7));
        }

        // Active control indicator badge
        painter.setFont(QFont("Arial", std::max(6, cellSize / 4), QFont::Bold));
        if (isMultiplayer) {
            if (arrowPlayer == 1) {
                painter.setPen(QColor(255, 240, 0));
                painter.drawText(px - 10, py - 4, cellSize + 20, cellSize / 2, Qt::AlignCenter, "P1 [ARROWS]");
            } else {
                painter.setPen(QColor(200, 200, 140));
                painter.drawText(px - 10, py - 4, cellSize + 20, cellSize / 2, Qt::AlignCenter, "P1 [WASD]");
            }
        }
    }

    // Draw Player 2 (Ms. Pacman - Pink with bow) (if multiplayer and in this sector)
    if (isMultiplayer && isCurrent && (p2Alive || (p2RespawnTicks > 0 && p2RespawnTicks % 4 < 2))) {
        int px = originX + p2X * cellSize;
        int py = originY + p2Y * cellSize;

        painter.setBrush(p2Alive ? QColor(255, 105, 180) : QColor(255, 150, 200, 110));
        painter.setPen(Qt::NoPen);
        int startAngle = 0;
        if (p2Dir == RIGHT) startAngle = p2MouthAngle;
        else if (p2Dir == DOWN) startAngle = 270 + p2MouthAngle;
        else if (p2Dir == LEFT) startAngle = 180 + p2MouthAngle;
        else if (p2Dir == UP) startAngle = 90 + p2MouthAngle;

        int spanAngle = 360 - 2 * p2MouthAngle;
        painter.drawPie(px + 2, py + 2, cellSize - 4, cellSize - 4, startAngle * 16, spanAngle * 16);

        // Cute Red Ribbon Bow on head
        painter.setBrush(QColor(235, 20, 60));
        int bowSize = std::max(4, cellSize / 4);
        int bx = px + cellSize / 2;
        int by = py + 2;
        painter.drawEllipse(bx - bowSize, by, bowSize * 3 / 4, bowSize / 2);
        painter.drawEllipse(bx, by, bowSize * 3 / 4, bowSize / 2);
        painter.setBrush(QColor(255, 220, 230));
        painter.drawEllipse(bx - bowSize / 4, by + bowSize / 6, bowSize / 2, bowSize / 2);

        // Eye beauty mark
        painter.setBrush(QColor(20, 20, 60));
        painter.drawEllipse(px + cellSize * 3 / 5, py + cellSize / 3, std::max(2, cellSize / 7), std::max(2, cellSize / 7));

        // Active control indicator badge
        painter.setFont(QFont("Arial", std::max(6, cellSize / 4), QFont::Bold));
        if (arrowPlayer == 2) {
            painter.setPen(QColor(255, 130, 200));
            painter.drawText(px - 10, py - 4, cellSize + 20, cellSize / 2, Qt::AlignCenter, "P2 [ARROWS]");
        } else {
            painter.setPen(QColor(220, 160, 200));
            painter.drawText(px - 10, py - 4, cellSize + 20, cellSize / 2, Qt::AlignCenter, "P2 [WASD]");
        }
    }

    // 6. Draw Ghosts in this sector
    QColor ghostNormalColors[] = {
        QColor(255, 50, 50),   // Blinky (Red)
        QColor(255, 180, 220), // Pinky (Pink)
        QColor(50, 220, 255),  // Inky (Cyan)
        QColor(255, 160, 50)   // Clyde (Orange)
    };

    for (const Ghost &g : std::as_const(ghosts)) {
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
        if (p1Alive) {
            int pxCenter = originX + pacmanX * cellSize + cellSize / 2;
            int pyCenter = originY + pacmanY * cellSize + cellSize / 2;

            QRadialGradient lantern(pxCenter, pyCenter, (currentVisionRadius + 0.6) * cellSize);
            lantern.setColorAt(0.0, QColor(255, 235, 170, 25));
            lantern.setColorAt(0.45, QColor(255, 200, 100, 10));
            lantern.setColorAt(0.75, QColor(0, 0, 0, 60));
            lantern.setColorAt(1.0, QColor(0, 0, 0, 220));

            painter.fillRect(originX, originY, COLS * cellSize, ROWS * cellSize, lantern);
        }
        if (isMultiplayer && p2Alive) {
            int p2xCenter = originX + p2X * cellSize + cellSize / 2;
            int p2yCenter = originY + p2Y * cellSize + cellSize / 2;

            QRadialGradient lantern2(p2xCenter, p2yCenter, (currentVisionRadius + 0.6) * cellSize);
            lantern2.setColorAt(0.0, QColor(255, 190, 230, 25));
            lantern2.setColorAt(0.45, QColor(255, 150, 200, 10));
            lantern2.setColorAt(0.75, QColor(0, 0, 0, 60));
            lantern2.setColorAt(1.0, QColor(0, 0, 0, 220));

            painter.fillRect(originX, originY, COLS * cellSize, ROWS * cellSize, lantern2);
        }
    }

    // 6. Glowing Sci-Fi Gateway Portals (Cuts through darkness & visible across fog!)
    float pulseTime = elapsedTimer.elapsed() * 0.007f;
    float pulseVal = (std::sin(pulseTime) + 1.0f) * 0.5f;
    int gateAlpha = 190 + int(65 * pulseVal);

    // East Gateway (Connects to sector sx + 1)
    if (sx < sectorCols - 1 && board[sy][sx][9][COLS - 1] == 0) {
        bool showGate = (!fogOfWarEnabled) ||
                        isCellVisible(sx, sy, COLS - 1, 9) ||
                        isCellVisible(sx, sy, COLS - 1, 10);
        if (showGate) {
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
            bool isEastDiscovered = (sx + 1 < sectorCols && sectorDiscovered[sy][sx + 1]);
            QString eastLabel = isEastDiscovered ? QString("SECTOR (%1,%2) ▶").arg(sx + 2).arg(sy + 1) : "EXPLORE ▶";
            painter.drawText(gx - cellSize * 3 - 6, gy + cellSize * 1.1, eastLabel);
        }
    }

    // West Gateway (Connects to sector sx - 1)
    if (sx > 0 && board[sy][sx][9][0] == 0) {
        bool showGate = (!fogOfWarEnabled) ||
                        isCellVisible(sx, sy, 0, 9) ||
                        isCellVisible(sx, sy, 0, 10);
        if (showGate) {
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
            bool isWestDiscovered = (sx - 1 >= 0 && sectorDiscovered[sy][sx - 1]);
            QString westLabel = isWestDiscovered ? QString("◀ SECTOR (%1,%2)").arg(sx).arg(sy + 1) : "◀ EXPLORE";
            painter.drawText(gx + gw + 4, gy + cellSize * 1.1, westLabel);
        }
    }

    // South Gateway (Connects to sector sy + 1)
    if (sy < sectorRows - 1 && board[sy][sx][ROWS - 1][9] == 0) {
        bool showGate = (!fogOfWarEnabled) ||
                        isCellVisible(sx, sy, 9, ROWS - 1) ||
                        isCellVisible(sx, sy, 10, ROWS - 1);
        if (showGate) {
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
            bool isSouthDiscovered = (sy + 1 < sectorRows && sectorDiscovered[sy + 1][sx]);
            QString southLabel = isSouthDiscovered ? QString("▼ SECTOR (%1,%2) ▼").arg(sx + 1).arg(sy + 2) : "▼ EXPLORE ▼";
            painter.drawText(gx - 4, gy - 6, southLabel);
        }
    }

    // North Gateway (Connects to sector sy - 1)
    if (sy > 0 && board[sy][sx][0][9] == 0) {
        bool showGate = (!fogOfWarEnabled) ||
                        isCellVisible(sx, sy, 9, 0) ||
                        isCellVisible(sx, sy, 10, 0);
        if (showGate) {
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
            bool isNorthDiscovered = (sy - 1 >= 0 && sectorDiscovered[sy - 1][sx]);
            QString northLabel = isNorthDiscovered ? QString("▲ SECTOR (%1,%2) ▲").arg(sx + 1).arg(sy) : "▲ EXPLORE ▲";
            painter.drawText(gx - 4, gy + gh + 14, northLabel);
        }
    }

    // 7. Glowing EXIT Portal (Hidden until Pacman is close or Fog Peek is triggered)
    if (sx == exitSectorX && sy == exitSectorY) {
        bool exitVisible = isCellVisible(sx, sy, exitX, exitY);

        if (!fogOfWarEnabled || exitVisible) {
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

void MainWindow::drawGameScreen(QPainter &painter)
{
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

    // Top-Left Saga Map Button
    QRect mapBtn = getMapButtonRect();
    bool mapBtnHovered = mapBtn.contains(mapFromGlobal(QCursor::pos()));
    painter.setBrush(mapBtnHovered ? QColor(25, 45, 90, 230) : QColor(14, 25, 55, 200));
    painter.setPen(QPen(mapBtnHovered ? QColor(0, 255, 240) : QColor(0, 180, 255), 1.5));
    painter.drawRoundedRect(mapBtn, 6, 6);

    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.setPen(mapBtnHovered ? Qt::white : QColor(220, 240, 255));
    painter.drawText(mapBtn, Qt::AlignCenter, "🗺️ SAGA MAP [M]");

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
    // Toast Notification Banner when a new sector is revealed/unfolded
    // ========================================================
    if (newSectorJustDiscovered) {
        qint64 ms = newSectorTimer.elapsed();
        if (ms < 1800) {
            float alpha = 1.0f;
            if (ms > 1200) alpha = (1800 - ms) / 600.0f;
            int a = int(220 * alpha);

            int bannerW = std::min(420, mazeW - 20);
            int bannerH = 34;
            int bx = startX + (mazeW - bannerW) / 2;
            int by = startY + 16;

            painter.setBrush(QColor(8, 22, 50, a));
            painter.setPen(QPen(QColor(0, 240, 255, a), 2));
            painter.drawRoundedRect(bx, by, bannerW, bannerH, 8, 8);

            painter.setPen(QColor(255, 235, 100, int(255 * alpha)));
            painter.setFont(QFont("Arial", std::max(11, bannerH / 3), QFont::Bold));
            painter.drawText(bx, by, bannerW, bannerH, Qt::AlignCenter,
                             "✨ UNCHARTED SECTOR UNFOLDED! ✨");
        } else {
            newSectorJustDiscovered = false;
        }
    }

    // ========================================================
    // Dynamic Unfolding Sector Mini-Map (Top-Right HUD Corner)
    // ========================================================
    {
        // 1. Calculate bounding box of all discovered sectors
        int minSx = currentSectorX, maxSx = currentSectorX;
        int minSy = currentSectorY, maxSy = currentSectorY;
        int discCount = 0;

        for (int sy = 0; sy < sectorRows; ++sy) {
            for (int sx = 0; sx < sectorCols; ++sx) {
                if (sectorDiscovered[sy][sx]) {
                    discCount++;
                    minSx = std::min(minSx, sx);
                    maxSx = std::max(maxSx, sx);
                    minSy = std::min(minSy, sy);
                    maxSy = std::max(maxSy, sy);
                }
            }
        }

        int unfoldCols = maxSx - minSx + 1;
        int unfoldRows = maxSy - minSy + 1;
        int maxDim = std::max(unfoldCols, unfoldRows);
        int mapBoxSize = std::clamp(int((cellSize * 11) / std::max(4, maxDim)), 9, 24);
        int mapGap = std::max(2, mapBoxSize / 5);
        int totalMapW = unfoldCols * mapBoxSize + (unfoldCols - 1) * mapGap;
        int totalMapH = unfoldRows * mapBoxSize + (unfoldRows - 1) * mapGap;
        int mapX = startX + mazeW - totalMapW - 16;
        int mapY = startY + 16;

        // Map container frame
        painter.fillRect(mapX - 8, mapY - 20, totalMapW + 16, totalMapH + 28, QColor(6, 10, 20, 215));
        painter.setPen(QPen(QColor(0, 180, 255, 120), 1));
        painter.drawRoundedRect(mapX - 8, mapY - 20, totalMapW + 16, totalMapH + 28, 6, 6);

        // Map title: strictly reveals ONLY discovered count, no totals!
        painter.setFont(QFont("Arial", std::max(7, mapBoxSize / 3), QFont::Bold));
        painter.setPen(QColor(0, 220, 255));
        QString mapTitle = (discCount == 1) ? "MAP (1 SECTOR)" : QString("MAP (%1 SECTORS)").arg(discCount);
        painter.drawText(mapX - 2, mapY - 6, mapTitle);

        // Render ONLY discovered sectors as they unfold
        for (int sy = minSy; sy <= maxSy; ++sy) {
            for (int sx = minSx; sx <= maxSx; ++sx) {
                if (!sectorDiscovered[sy][sx]) continue;

                int rx = mapX + (sx - minSx) * (mapBoxSize + mapGap);
                int ry = mapY + (sy - minSy) * (mapBoxSize + mapGap);

                // Sector base tile
                painter.fillRect(rx, ry, mapBoxSize, mapBoxSize, QColor(18, 42, 85));
                painter.setPen(QPen(QColor(50, 130, 220), 1));
                painter.drawRect(rx, ry, mapBoxSize, mapBoxSize);

                // Mark EXIT only if this discovered sector contains the exit AND (peek active or player sees it)
                if (sx == exitSectorX && sy == exitSectorY && (!fogOfWarEnabled || isCellVisible(exitSectorX, exitSectorY, exitX, exitY))) {
                    painter.setPen(QColor(0, 255, 120));
                    painter.setFont(QFont("Arial", std::max(7, mapBoxSize / 3), QFont::Bold));
                    painter.drawText(rx, ry, mapBoxSize, mapBoxSize, Qt::AlignCenter, "EXIT");
                } else {
                    // Small sector coordinate label
                    painter.setFont(QFont("Arial", std::max(6, mapBoxSize / 4)));
                    painter.setPen(QColor(130, 180, 240, 160));
                    painter.drawText(rx + 2, ry + mapBoxSize - 3, QString("%1,%2").arg(sx + 1).arg(sy + 1));
                }

                // Current window cursor (Golden pulsing outline + Pacman dot)
                if (sx == currentSectorX && sy == currentSectorY) {
                    painter.setPen(QPen(QColor(255, 220, 0), 2));
                    painter.drawRect(rx - 1, ry - 1, mapBoxSize + 2, mapBoxSize + 2);

                    painter.setBrush(QColor(255, 220, 0));
                    painter.setPen(Qt::NoPen);
                    painter.drawEllipse(rx + mapBoxSize/2 - 2, ry + mapBoxSize/2 - 2, 4, 4);

                    // Show small exploration chevrons on edges only during Fog Peek!
                    if (!fogOfWarEnabled) {
                        float chevPulse = (std::sin(elapsedTimer.elapsed() * 0.008f) + 1.0f) * 0.5f;
                        int chevAlpha = 140 + int(115 * chevPulse);
                        painter.setPen(QColor(0, 255, 240, chevAlpha));
                        painter.setFont(QFont("Arial", std::max(7, mapBoxSize / 3), QFont::Bold));

                        if (sx + 1 < sectorCols && !sectorDiscovered[sy][sx + 1]) {
                            painter.drawText(rx + mapBoxSize - 1, ry + mapBoxSize * 3 / 4, "▶");
                        }
                        if (sx - 1 >= 0 && !sectorDiscovered[sy][sx - 1]) {
                            painter.drawText(rx - 7, ry + mapBoxSize * 3 / 4, "◀");
                        }
                        if (sy + 1 < sectorRows && !sectorDiscovered[sy + 1][sx]) {
                            painter.drawText(rx + mapBoxSize / 3, ry + mapBoxSize + 7, "▼");
                        }
                        if (sy - 1 >= 0 && !sectorDiscovered[sy - 1][sx]) {
                            painter.drawText(rx + mapBoxSize / 3, ry - 1, "▲");
                        }
                    }
                }

                // Pathway bridge connectors between adjacent DISCOVERED sectors
                if (sx < maxSx && sectorDiscovered[sy][sx + 1]) {
                    painter.setPen(QPen(QColor(0, 230, 255, 200), 2));
                    int cy = ry + mapBoxSize / 2;
                    painter.drawLine(rx + mapBoxSize, cy, rx + mapBoxSize + mapGap, cy);
                }
                if (sy < maxSy && sectorDiscovered[sy + 1][sx]) {
                    painter.setPen(QPen(QColor(0, 230, 255, 200), 2));
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

    // Count discovered sectors for HUD
    int discCount = 0;
    for (int sy = 0; sy < sectorRows; ++sy) {
        for (int sx = 0; sx < sectorCols; ++sx) {
            if (sectorDiscovered[sy][sx]) discCount++;
        }
    }

    // Line 1: Stats & Window location (hudY + 20)
    painter.setFont(QFont("Arial", baseFont1, QFont::Bold));
    int worldNum = (currentLevel - 1) / LEVELS_PER_WORLD + 1;
    int stageNum = (currentLevel - 1) % LEVELS_PER_WORLD + 1;
    if (!isMultiplayer) {
        painter.setPen(Qt::white);
        painter.drawText(20, hudY + 20, QString("SCORE: %1").arg(score));

        painter.setPen(QColor(0, 230, 160));
        painter.drawText(width() * 0.16, hudY + 20, QString("W%1-S%2 (LVL %3)").arg(worldNum).arg(stageNum).arg(currentLevel));

        painter.setPen(QColor(0, 210, 255));
        painter.drawText(width() * 0.30, hudY + 20,
                         QString("SECTOR: (%1, %2)").arg(currentSectorX + 1).arg(currentSectorY + 1));

        painter.setPen(QColor(255, 180, 0));
        painter.drawText(width() * 0.44, hudY + 20,
                         QString("EXPLORED: %1").arg(discCount));

        painter.setPen(QColor(180, 200, 255));
        painter.drawText(width() * 0.56, hudY + 20, QString("MOVES: %1").arg(moveCount));
    } else {
        // P1 Status (Yellow)
        painter.setPen(p1Alive ? QColor(255, 230, 30) : QColor(160, 160, 80));
        QString p1Str = p1Alive ? QString("P1 (🟡): %1").arg(scoreP1)
                                : QString("P1: RESPAWN (%1s)").arg((p1RespawnTicks * 130 + 999) / 1000);
        painter.drawText(20, hudY + 20, p1Str);

        // P2 Status (Pink)
        painter.setPen(p2Alive ? QColor(255, 120, 190) : QColor(160, 80, 120));
        QString p2Str = p2Alive ? QString("P2 (🎀): %1").arg(scoreP2)
                                : QString("P2: RESPAWN (%1s)").arg((p2RespawnTicks * 130 + 999) / 1000);
        painter.drawText(width() * 0.18, hudY + 20, p2Str);

        // Team Total Score
        painter.setPen(Qt::white);
        painter.drawText(width() * 0.36, hudY + 20, QString("TEAM: %1").arg(score));

        painter.setPen(QColor(0, 230, 160));
        painter.drawText(width() * 0.48, hudY + 20, QString("W%1-S%2 (LVL %3)").arg(worldNum).arg(stageNum).arg(currentLevel));

        painter.setPen(QColor(0, 210, 255));
        painter.drawText(width() * 0.58, hudY + 20,
                         QString("SECTOR: (%1, %2)").arg(currentSectorX + 1).arg(currentSectorY + 1));
    }

    qint64 elapsedSec = elapsedTimer.elapsed() / 1000;
    int mins = elapsedSec / 60;
    int secs = elapsedSec % 60;
    painter.setPen(QColor(255, 200, 100));
    painter.drawText(width() * 0.65, hudY + 20, QString("TIME: %1:%2")
                     .arg(mins, 2, 10, QChar('0'))
                     .arg(secs, 2, 10, QChar('0')));

    if (fogPeekActive) {
        int msLeft = std::max(0, FOG_PEEK_DURATION_MS - (int)fogPeekTimer.elapsed());
        double secLeft = msLeft / 1000.0;
        painter.setPen(QColor(255, 225, 50));
        painter.drawText(width() * 0.74, hudY + 20, QString("[PEEKING: %1s 💡 (%2 left)]").arg(secLeft, 0, 'f', 1).arg(fogPeeksRemaining));
    } else if (fogPeeksRemaining > 0) {
        painter.setPen(QColor(100, 230, 255));
        painter.drawText(width() * 0.74, hudY + 20, QString("[PEEK (F): %1 left 👁️]").arg(fogPeeksRemaining));
    } else {
        painter.setPen(QColor(255, 110, 110));
        painter.drawText(width() * 0.74, hudY + 20, "[PEEK: 0 left 🍒 (Eat Fruit!)]");
    }

    if (ghostsFrightened) {
        painter.setPen(QColor(255, 100, 140));
        painter.drawText(width() * 0.89, hudY + 20, QString("⚡ POWER! %1").arg(frightenedTicksLeft));
    }

    // Floating Notification / Toast Banner (Fog Peek activation, Out of peeks warning, Fruit bonus)
    if (peekToastTimer.isValid() && peekToastTimer.elapsed() < 2400) {
        painter.save();
        int tW = std::clamp(int(width() * 0.46), 340, 520);
        int tH = 34;
        int tX = (width() - tW) / 2;
        int tY = hudY - 44;

        // Shadow
        painter.setBrush(QColor(0, 0, 0, 160));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(tX + 2, tY + 2, tW, tH, 10, 10);

        // Body
        painter.setBrush(QColor(12, 16, 32, 230));
        QColor borderCol = (fogPeeksRemaining > 0 ? QColor(80, 240, 140) : QColor(255, 110, 110));
        painter.setPen(QPen(borderCol, 2));
        painter.drawRoundedRect(tX, tY, tW, tH, 10, 10);

        painter.setFont(QFont("Arial", baseFont1, QFont::Bold));
        painter.setPen(borderCol);
        painter.drawText(tX, tY, tW, tH, Qt::AlignCenter, peekToastMessage);
        painter.restore();
    }

    // Line 2: Prominent Mission Objective Guidance (hudY + 42)
    int curWorld = (currentLevel - 1) / LEVELS_PER_WORLD + 1;
    int curStage = (currentLevel - 1) % LEVELS_PER_WORLD + 1;
    QString objectiveText;
    if (currentSectorX == exitSectorX && currentSectorY == exitSectorY) {
        bool exitNear = isCellVisible(currentSectorX, currentSectorY, exitX, exitY) || (!fogOfWarEnabled);
        if (exitNear) {
            objectiveText = QString("🏁 OBJECTIVE: Enter the green [EXIT] portal to clear Stage %1/20 (%2)!").arg(curStage).arg(getWorldName(curWorld));
            painter.setPen(QColor(60, 255, 140));
        } else {
            objectiveText = QString("🧭 OBJECTIVE: Search the dark labyrinth with your lantern to locate the hidden [EXIT] portal! (%1)").arg(getWorldName(curWorld));
            painter.setPen(QColor(255, 215, 60));
        }
    } else {
        if (!fogOfWarEnabled) {
            QString dirStr;
            if (currentSectorX < exitSectorX) dirStr = "RIGHT wall [▶ GATEWAY]";
            else if (currentSectorX > exitSectorX) dirStr = "LEFT wall [◀ GATEWAY]";
            else if (currentSectorY < exitSectorY) dirStr = "BOTTOM wall [▼ GATEWAY]";
            else if (currentSectorY > exitSectorY) dirStr = "TOP wall [▲ GATEWAY]";
            objectiveText = QString("💡 PEEK REVEAL: Head toward the %1 to reach the exit sector! (%2)").arg(dirStr).arg(getWorldName(curWorld));
            painter.setPen(QColor(0, 255, 240));
        } else {
            objectiveText = QString("🧭 OBJECTIVE: Explore the dark corridors with your lantern to discover gateways! (%1)").arg(getWorldName(curWorld));
            painter.setPen(QColor(255, 220, 80));
        }
    }
    painter.setFont(QFont("Arial", baseFont1, QFont::Bold));
    painter.drawText(20, hudY + 42, objectiveText);

    // Line 3: Controls (hudY + 62)
    painter.setFont(QFont("Arial", baseFont2));
    painter.setPen(QColor(140, 160, 190));
    if (!isMultiplayer) {
        if (singlePlayerHero == 2) {
            painter.drawText(20, hudY + 62,
                "HERO: [🎀 Pink Ms. Pacman]  |  [Arrows] or [WASD] Move  |  [Tab] Switch to Yellow  |  [F] Fog Peek (1.0s, 3x/lvl + Fruits)  |  [M] Saga Map  |  [F11] Fullscreen");
        } else {
            painter.drawText(20, hudY + 62,
                "HERO: [🟡 Yellow Pacman]  |  [Arrows] or [WASD] Move  |  [Tab] Switch to Pink  |  [F] Fog Peek (1.0s, 3x/lvl + Fruits)  |  [M] Saga Map  |  [F11] Fullscreen");
        }
    } else {
        if (arrowPlayer == 2) {
            painter.drawText(20, hudY + 62,
                "ARROWS STEER: [🎀 Pink P2]  |  WASD: [🟡 Yellow P1]  |  [Tab] Swap Arrows to Yellow!  |  [F] Fog Peek (1.0s, 3x/lvl + Fruits)  |  [M] Saga Map");
        } else {
            painter.drawText(20, hudY + 62,
                "ARROWS STEER: [🟡 Yellow P1]  |  WASD: [🎀 Pink P2]  |  [Tab] Swap Arrows to Pink!  |  [F] Fog Peek (1.0s, 3x/lvl + Fruits)  |  [M] Saga Map");
        }
    }

    // Game Over / Level Complete Banner
    if (gameOver) {
        painter.fillRect(0, height() / 2 - 45, width(), 95, QColor(0, 0, 0, 235));
        painter.setPen(QColor(255, 60, 60));
        painter.setFont(QFont("Arial", std::max(18, width() / 50), QFont::Bold));
        QString goMsg = isMultiplayer
            ? QString("GAME OVER! Both Players Defeated (Stage %1/20 • %2)").arg(curStage).arg(getWorldName(curWorld))
            : QString("GAME OVER! (Stage %1/20 • %2)").arg(curStage).arg(getWorldName(curWorld));
        painter.drawText(0, height() / 2 - 18, width(), 30, Qt::AlignCenter, goMsg);

        painter.setPen(QColor(255, 220, 100));
        painter.setFont(QFont("Arial", std::max(12, width() / 70)));
        painter.drawText(0, height() / 2 + 16, width(), 25, Qt::AlignCenter,
                         QString("Press [R] or [Space] to Retry Stage %1  |  Press [M] for Saga Map").arg(curStage));
    } else if (levelComplete) {
        painter.fillRect(0, height() / 2 - 50, width(), 100, QColor(0, 0, 0, 235));
        painter.setPen(QColor(60, 255, 120));
        painter.setFont(QFont("Arial", std::max(16, width() / 55), QFont::Bold));
        QString levelMsg = isMultiplayer
            ? QString("STAGE %1/20 COMPLETE!  (%2) • Team Escaped!").arg(curStage).arg(getWorldName(curWorld))
            : QString("STAGE %1/20 COMPLETE!  (%2) • Labyrinth Escaped!").arg(curStage).arg(getWorldName(curWorld));
        painter.drawText(0, height() / 2 - 18, width(), 30, Qt::AlignCenter, levelMsg);

        painter.setPen(QColor(255, 220, 100));
        painter.setFont(QFont("Arial", std::max(12, width() / 70)));
        painter.drawText(0, height() / 2 + 16, width(), 25, Qt::AlignCenter,
                         "Press [Space] for Next Level  |  Press [M] for Saga Map");
    }
}
