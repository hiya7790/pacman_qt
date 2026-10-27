#include "mainwindow.h"
#include "gameserver.h"
#include "soundmanager.h"
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

struct MazeTheme {
    QColor wallBase;
    QColor wallBorder;
    QColor wallGlow;
    QColor corridor;
    QColor dotCore;
    QColor dotGlow;
};

static MazeTheme getMazeThemeForWorld(int world)
{
    switch ((world - 1) % 6) {
        case 0: // World 1: Neon Sapphire & Electric Cyan (Cyber-Matrix)
            return { QColor(16, 26, 68), QColor(0, 210, 255), QColor(140, 245, 255), QColor(6, 8, 20), QColor(255, 240, 180), QColor(255, 220, 100, 75) };
        case 1: // World 2: Molten Caramel & Amber Caverns
            return { QColor(48, 18, 10), QColor(255, 120, 30), QColor(255, 190, 90), QColor(14, 6, 4), QColor(255, 210, 120), QColor(255, 160, 40, 75) };
        case 2: // World 3: Gummy Galaxy & Cosmic Amethyst
            return { QColor(28, 14, 58), QColor(190, 80, 255), QColor(230, 160, 255), QColor(10, 5, 20), QColor(220, 240, 255), QColor(180, 120, 255, 75) };
        case 3: // World 4: Cotton Candy Clouds & Strawberry Velvet
            return { QColor(46, 16, 52), QColor(255, 80, 175), QColor(255, 180, 225), QColor(16, 6, 18), QColor(255, 235, 210), QColor(255, 130, 190, 75) };
        case 4: // World 5: Chocolate Chip Chasm & Sunstone Gold
            return { QColor(32, 18, 10), QColor(235, 175, 35), QColor(255, 230, 110), QColor(12, 7, 5), QColor(255, 225, 80), QColor(235, 180, 40, 75) };
        case 5: // World 6: Emerald Lagoon & Jade Sanctum
        default:
            return { QColor(10, 34, 38), QColor(0, 230, 150), QColor(130, 255, 210), QColor(4, 14, 16), QColor(235, 255, 245), QColor(0, 230, 150, 75) };
    }
}

void MainWindow::drawSector(QPainter &painter, int sx, int sy, int originX, int originY, int cellSize)
{
    bool isCurrent = (sx == currentSectorX && sy == currentSectorY);
    int worldNum = (currentLevel - 1) / LEVELS_PER_WORLD + 1;
    MazeTheme theme = getMazeThemeForWorld(worldNum);

    // 1. Draw Maze Tiles & Glowing Candy Pellets
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
                // 3D Beveled Acrylic Wall
                int cornerR = std::max(4, cellSize / 5);
                if (visible) {
                    painter.setBrush(theme.wallBase);
                    painter.setPen(QPen(theme.wallBorder, std::max(1.5, cellSize * 0.08)));
                    painter.drawRoundedRect(cx + 1, cy + 1, cellSize - 2, cellSize - 2, cornerR, cornerR);

                    // Specular gleam highlight on top and left edges
                    painter.setPen(QPen(theme.wallGlow, 1.2));
                    painter.drawLine(cx + cornerR, cy + 2, cx + cellSize - cornerR, cy + 2);
                    painter.drawLine(cx + 2, cy + cornerR, cx + 2, cy + cellSize - cornerR);
                } else {
                    painter.setBrush(theme.wallBase.darker(240));
                    painter.setPen(QPen(theme.wallBorder.darker(220), 1.0));
                    painter.drawRoundedRect(cx + 1, cy + 1, cellSize - 2, cellSize - 2, cornerR, cornerR);
                }
            } else {
                // Open corridor
                if (visible) {
                    painter.fillRect(cx, cy, cellSize, cellSize, theme.corridor);
                    if (board[sy][sx][r][c] == 2) {
                        // Glowing Candy Energy Pellet
                        int dotSize = std::max(5, cellSize * 6 / 30);
                        int dX = cx + cellSize / 2;
                        int dY = cy + cellSize / 2;

                        // Luminous soft halo
                        painter.setBrush(theme.dotGlow);
                        painter.setPen(Qt::NoPen);
                        painter.drawEllipse(dX - dotSize, dY - dotSize, dotSize * 2, dotSize * 2);

                        // Shiny crisp candy pellet
                        painter.setBrush(theme.dotCore);
                        painter.drawEllipse(dX - dotSize/2, dY - dotSize/2, dotSize, dotSize);

                        // Specular glint
                        painter.setBrush(QColor(255, 255, 255, 220));
                        painter.drawEllipse(dX - dotSize/4, dY - dotSize/3, std::max(1, dotSize/3), std::max(1, dotSize/3));
                    }
                } else {
                    painter.fillRect(cx, cy, cellSize, cellSize, QColor(4, 4, 10));
                    if (board[sy][sx][r][c] == 2) {
                        int dotSize = std::max(2, cellSize * 3 / 30);
                        painter.setBrush(QColor(120, 110, 95, 70));
                        painter.setPen(Qt::NoPen);
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

    // 5. Draw Primary Hero / Player 1 (if active and in this sector)
    if (isP1Active() && isCurrent && (p1Alive || (p1RespawnTicks > 0 && p1RespawnTicks % 4 < 2))) {
        int px = originX + pacmanX * cellSize;
        int py = originY + pacmanY * cellSize;

        bool isPinkSolo = (!isAnyMultiplayer() && singlePlayerHero == 2);

        int startAngle = 0;
        if (pacmanDir == RIGHT) startAngle = pacmanMouthAngle;
        else if (pacmanDir == DOWN) startAngle = 270 + pacmanMouthAngle;
        else if (pacmanDir == LEFT) startAngle = 180 + pacmanMouthAngle;
        else if (pacmanDir == UP) startAngle = 90 + pacmanMouthAngle;
        int spanAngle = 360 - 2 * pacmanMouthAngle;

        // 3D Spherical Radial Gradient
        QRadialGradient heroGrad(px + cellSize * 0.38, py + cellSize * 0.32, cellSize * 0.65);
        if (isPinkSolo) {
            heroGrad.setColorAt(0.0, p1Alive ? QColor(255, 215, 235) : QColor(255, 220, 240, 130));
            heroGrad.setColorAt(0.55, p1Alive ? QColor(255, 105, 180) : QColor(255, 130, 190, 120));
            heroGrad.setColorAt(1.0, p1Alive ? QColor(190, 45, 120) : QColor(170, 50, 110, 100));
        } else {
            heroGrad.setColorAt(0.0, p1Alive ? QColor(255, 255, 160) : QColor(255, 255, 180, 130));
            heroGrad.setColorAt(0.55, p1Alive ? QColor(255, 220, 0) : QColor(255, 230, 40, 120));
            heroGrad.setColorAt(1.0, p1Alive ? QColor(210, 145, 0) : QColor(190, 150, 20, 100));
        }
        painter.setBrush(heroGrad);
        painter.setPen(QPen(isPinkSolo ? QColor(255, 70, 150) : QColor(210, 150, 0), 1.2));
        painter.drawPie(px + 2, py + 2, cellSize - 4, cellSize - 4, startAngle * 16, spanAngle * 16);

        // Expressive Cartoon Eye
        int eyeR = std::max(3, cellSize / 8);
        int eyeX = px + cellSize * 0.42;
        int eyeY = py + cellSize * 0.28;
        if (pacmanDir == LEFT) eyeX = px + cellSize * 0.32;
        else if (pacmanDir == RIGHT) eyeX = px + cellSize * 0.58;
        else if (pacmanDir == DOWN) { eyeX = px + cellSize * 0.35; eyeY = py + cellSize * 0.60; }

        painter.setBrush(QColor(25, 20, 30));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(eyeX - eyeR/2, eyeY - eyeR/2, eyeR, eyeR);
        painter.setBrush(Qt::white);
        painter.drawEllipse(eyeX - eyeR/4, eyeY - eyeR/3, std::max(1, eyeR/3), std::max(1, eyeR/3));

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

            // Beauty mark
            painter.setBrush(QColor(20, 20, 60));
            painter.drawEllipse(px + cellSize * 3 / 5, py + cellSize / 3, std::max(2, cellSize / 7), std::max(2, cellSize / 7));
        }

        // Active control indicator badge
        painter.setFont(QFont("Arial", std::max(6, cellSize / 4), QFont::Bold));
        if (isAnyMultiplayer()) {
            QString stat = p1ReachedExit ? " 🏁" : "";
            if (hostRole == "p1") {
                painter.setPen(p1ReachedExit ? QColor(0, 255, 140) : QColor(255, 240, 0));
                painter.drawText(px - 15, py - 5, cellSize + 30, cellSize / 2, Qt::AlignCenter, QString("★ HOST%1").arg(stat));
            } else {
                painter.setPen(p1ReachedExit ? QColor(0, 255, 140) : QColor(255, 230, 0));
                QString p1Tag = p1PlayerName.isEmpty() ? "P1 (Pac-Man)" : p1PlayerName;
                painter.drawText(px - 15, py - 5, cellSize + 30, cellSize / 2, Qt::AlignCenter, QString("%1%2").arg(p1Tag, stat));
            }
        }
    }

    // Draw Player 2 (Ms. Pacman - Pink with bow) (if active and in this sector)
    if (isP2Active() && isCurrent && (p2Alive || (p2RespawnTicks > 0 && p2RespawnTicks % 4 < 2))) {
        int px = originX + p2X * cellSize;
        int py = originY + p2Y * cellSize;

        int startAngle = 0;
        if (p2Dir == RIGHT) startAngle = p2MouthAngle;
        else if (p2Dir == DOWN) startAngle = 270 + p2MouthAngle;
        else if (p2Dir == LEFT) startAngle = 180 + p2MouthAngle;
        else if (p2Dir == UP) startAngle = 90 + p2MouthAngle;
        int spanAngle = 360 - 2 * p2MouthAngle;

        QRadialGradient p2Grad(px + cellSize * 0.38, py + cellSize * 0.32, cellSize * 0.65);
        p2Grad.setColorAt(0.0, p2Alive ? QColor(255, 215, 235) : QColor(255, 220, 240, 130));
        p2Grad.setColorAt(0.55, p2Alive ? QColor(255, 105, 180) : QColor(255, 130, 190, 120));
        p2Grad.setColorAt(1.0, p2Alive ? QColor(190, 45, 120) : QColor(170, 50, 110, 100));
        painter.setBrush(p2Grad);
        painter.setPen(QPen(QColor(255, 70, 150), 1.2));
        painter.drawPie(px + 2, py + 2, cellSize - 4, cellSize - 4, startAngle * 16, spanAngle * 16);

        // Expressive cartoon eye with glamorous eyelashes
        int eyeR = std::max(3, cellSize / 8);
        int eyeX = px + cellSize * 0.42;
        int eyeY = py + cellSize * 0.28;
        if (p2Dir == LEFT) eyeX = px + cellSize * 0.32;
        else if (p2Dir == RIGHT) eyeX = px + cellSize * 0.58;
        else if (p2Dir == DOWN) { eyeX = px + cellSize * 0.35; eyeY = py + cellSize * 0.60; }

        painter.setBrush(QColor(25, 20, 30));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(eyeX - eyeR/2, eyeY - eyeR/2, eyeR, eyeR);
        painter.setBrush(Qt::white);
        painter.drawEllipse(eyeX - eyeR/4, eyeY - eyeR/3, std::max(1, eyeR/3), std::max(1, eyeR/3));

        // Cute Red Ribbon Bow on head
        painter.setBrush(QColor(235, 20, 60));
        int bowSize = std::max(4, cellSize / 4);
        int bx = px + cellSize / 2;
        int by = py + 2;
        painter.drawEllipse(bx - bowSize, by, bowSize * 3 / 4, bowSize / 2);
        painter.drawEllipse(bx, by, bowSize * 3 / 4, bowSize / 2);
        painter.setBrush(QColor(255, 220, 230));
        painter.drawEllipse(bx - bowSize / 4, by + bowSize / 6, bowSize / 2, bowSize / 2);

        // Beauty mark
        painter.setBrush(QColor(20, 20, 60));
        painter.drawEllipse(px + cellSize * 3 / 5, py + cellSize / 3, std::max(2, cellSize / 7), std::max(2, cellSize / 7));

        // Active control indicator badge
        painter.setFont(QFont("Arial", std::max(6, cellSize / 4), QFont::Bold));
        painter.setPen(p2ReachedExit ? QColor(0, 255, 140) : QColor(255, 130, 200));
        QString p2Stat = p2ReachedExit ? " 🏁" : "";
        QString p2Tag = (hostRole == "p2") ? QString("★ HOST%1").arg(p2Stat) : QString("%1%2").arg(p2PlayerName.isEmpty() ? "P2 (Ms. Pac)" : p2PlayerName, p2Stat);
        painter.drawText(px - 15, py - 5, cellSize + 30, cellSize / 2, Qt::AlignCenter, p2Tag);
    }

    // Draw Hero 3 (Jr. Pac-Man - Cyan with cap) (if connected or host and in this sector)
    if ((hero3.clientId != -1 || hostRole == "p3") && isCurrent && (hero3.alive || (hero3.respawnTicks > 0 && hero3.respawnTicks % 4 < 2))) {
        int px = originX + hero3.x * cellSize;
        int py = originY + hero3.y * cellSize;

        int startAngle = 0;
        if (hero3.dir == RIGHT) startAngle = hero3.mouthAngle;
        else if (hero3.dir == DOWN) startAngle = 270 + hero3.mouthAngle;
        else if (hero3.dir == LEFT) startAngle = 180 + hero3.mouthAngle;
        else if (hero3.dir == UP) startAngle = 90 + hero3.mouthAngle;
        int spanAngle = 360 - 2 * hero3.mouthAngle;

        QRadialGradient h3Grad(px + cellSize * 0.38, py + cellSize * 0.32, cellSize * 0.65);
        h3Grad.setColorAt(0.0, hero3.alive ? QColor(160, 245, 255) : QColor(190, 250, 255, 130));
        h3Grad.setColorAt(0.55, hero3.alive ? QColor(0, 229, 255) : QColor(80, 210, 240, 120));
        h3Grad.setColorAt(1.0, hero3.alive ? QColor(0, 140, 200) : QColor(40, 130, 180, 100));
        painter.setBrush(h3Grad);
        painter.setPen(QPen(QColor(0, 180, 240), 1.2));
        painter.drawPie(px + 2, py + 2, cellSize - 4, cellSize - 4, startAngle * 16, spanAngle * 16);

        // Expressive Eye
        int eyeR = std::max(3, cellSize / 8);
        int eyeX = px + cellSize * 0.42;
        int eyeY = py + cellSize * 0.30;
        if (hero3.dir == LEFT) eyeX = px + cellSize * 0.32;
        else if (hero3.dir == RIGHT) eyeX = px + cellSize * 0.58;
        painter.setBrush(QColor(25, 20, 30));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(eyeX - eyeR/2, eyeY - eyeR/2, eyeR, eyeR);
        painter.setBrush(Qt::white);
        painter.drawEllipse(eyeX - eyeR/4, eyeY - eyeR/3, std::max(1, eyeR/3), std::max(1, eyeR/3));

        // Blue Propeller Beanie Cap
        painter.setBrush(QColor(20, 90, 240));
        painter.drawEllipse(px + cellSize / 4, py + 1, cellSize / 2, cellSize / 4);
        painter.setPen(QPen(QColor(255, 50, 50), 2));
        painter.drawLine(px + cellSize / 2, py + 2, px + cellSize / 2, py - 3);
        painter.drawLine(px + cellSize / 2 - 4, py - 3, px + cellSize / 2 + 4, py - 3);
        painter.setPen(Qt::NoPen);

        // Name tag
        painter.setFont(QFont("Arial", std::max(6, cellSize / 4), QFont::Bold));
        painter.setPen(hero3.reachedExit ? QColor(0, 255, 140) : QColor(0, 255, 255));
        QString p3Stat = hero3.reachedExit ? " 🏁" : "";
        QString tag = (hostRole == "p3") ? QString("★ HOST%1").arg(p3Stat) : QString("%1%2").arg(hero3.playerName.isEmpty() ? "P3 (Jr. Pac)" : hero3.playerName, p3Stat);
        painter.drawText(px - 15, py - 5, cellSize + 30, cellSize / 2, Qt::AlignCenter, tag);
    }

    // Draw Hero 4 (Miracle Pac-Man - Emerald Green) (if connected or host and in this sector)
    if ((hero4.clientId != -1 || hostRole == "p4") && isCurrent && (hero4.alive || (hero4.respawnTicks > 0 && hero4.respawnTicks % 4 < 2))) {
        int px = originX + hero4.x * cellSize;
        int py = originY + hero4.y * cellSize;

        int startAngle = 0;
        if (hero4.dir == RIGHT) startAngle = hero4.mouthAngle;
        else if (hero4.dir == DOWN) startAngle = 270 + hero4.mouthAngle;
        else if (hero4.dir == LEFT) startAngle = 180 + hero4.mouthAngle;
        else if (hero4.dir == UP) startAngle = 90 + hero4.mouthAngle;
        int spanAngle = 360 - 2 * hero4.mouthAngle;

        QRadialGradient h4Grad(px + cellSize * 0.38, py + cellSize * 0.32, cellSize * 0.65);
        h4Grad.setColorAt(0.0, hero4.alive ? QColor(140, 255, 180) : QColor(180, 255, 210, 130));
        h4Grad.setColorAt(0.55, hero4.alive ? QColor(0, 235, 100) : QColor(60, 220, 120, 120));
        h4Grad.setColorAt(1.0, hero4.alive ? QColor(0, 150, 60) : QColor(30, 140, 60, 100));
        painter.setBrush(h4Grad);
        painter.setPen(QPen(QColor(0, 180, 70), 1.2));
        painter.drawPie(px + 2, py + 2, cellSize - 4, cellSize - 4, startAngle * 16, spanAngle * 16);

        // Expressive Eye
        int eyeR = std::max(3, cellSize / 8);
        int eyeX = px + cellSize * 0.42;
        int eyeY = py + cellSize * 0.30;
        if (hero4.dir == LEFT) eyeX = px + cellSize * 0.32;
        else if (hero4.dir == RIGHT) eyeX = px + cellSize * 0.58;
        painter.setBrush(QColor(25, 20, 30));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(eyeX - eyeR/2, eyeY - eyeR/2, eyeR, eyeR);
        painter.setBrush(Qt::white);
        painter.drawEllipse(eyeX - eyeR/4, eyeY - eyeR/3, std::max(1, eyeR/3), std::max(1, eyeR/3));

        // Superhero headband with golden star
        painter.setBrush(QColor(0, 170, 60));
        painter.drawRect(px + 2, py + cellSize / 4, cellSize - 4, std::max(2, cellSize / 8));
        painter.setBrush(QColor(255, 225, 50));
        painter.drawEllipse(px + cellSize / 2 - 2, py + cellSize / 4, 4, 4);

        // Name tag
        painter.setFont(QFont("Arial", std::max(6, cellSize / 4), QFont::Bold));
        painter.setPen(hero4.reachedExit ? QColor(0, 255, 140) : QColor(50, 255, 120));
        QString p4Stat = hero4.reachedExit ? " 🏁" : "";
        QString tag = (hostRole == "p4") ? QString("★ HOST%1").arg(p4Stat) : QString("%1%2").arg(hero4.playerName.isEmpty() ? "P4 (Miracle)" : hero4.playerName, p4Stat);
        painter.drawText(px - 15, py - 5, cellSize + 30, cellSize / 2, Qt::AlignCenter, tag);
    }

    // 6. Draw Ghosts in this sector (with floating hover bobbing & pupil tracking!)
    QColor ghostNormalColors[] = {
        QColor(255, 50, 50),   // Blinky (Red)
        QColor(255, 150, 210), // Pinky (Pink)
        QColor(50, 220, 255),  // Inky (Cyan)
        QColor(255, 160, 50)   // Clyde (Orange)
    };

    for (const Ghost &g : std::as_const(ghosts)) {
        if (g.sectorX != sx || g.sectorY != sy) continue;
        if (!isCellVisible(sx, sy, g.x, g.y)) continue;

        // Floating hover bobbing animation
        int bobY = int(std::sin(elapsedTimer.elapsed() * 0.007f + g.colorIndex * 1.5f) * (cellSize * 0.09f));
        int gx = originX + g.x * cellSize;
        int gy = originY + g.y * cellSize + bobY;

        QRadialGradient ghostGrad(gx + cellSize * 0.38, gy + cellSize * 0.32, cellSize * 0.65);
        if (ghostsFrightened) {
            if (frightenedTicksLeft <= 8 && (frightenedTicksLeft % 2 == 0)) {
                ghostGrad.setColorAt(0.0, QColor(255, 255, 255));
                ghostGrad.setColorAt(1.0, QColor(210, 230, 255));
            } else {
                ghostGrad.setColorAt(0.0, QColor(90, 130, 255));
                ghostGrad.setColorAt(0.6, QColor(35, 45, 220));
                ghostGrad.setColorAt(1.0, QColor(15, 20, 150));
            }
        } else {
            QColor base = ghostNormalColors[g.colorIndex];
            ghostGrad.setColorAt(0.0, base.lighter(140));
            ghostGrad.setColorAt(0.65, base);
            ghostGrad.setColorAt(1.0, base.darker(150));
        }
        painter.setBrush(ghostGrad);
        painter.setPen(Qt::NoPen);

        // Body Dome
        painter.drawEllipse(gx + 2, gy + 2, cellSize - 4, cellSize - 4);
        painter.drawRect(gx + 2, gy + cellSize/2, cellSize - 4, (cellSize - 4)/2);

        // Animated Rippling Tentacle Skirt
        int waveW = (cellSize - 4) / 3;
        float wavePhase = elapsedTimer.elapsed() * 0.012f + g.colorIndex;
        for (int w = 0; w < 3; ++w) {
            int wx = gx + 2 + w * waveW;
            int waveOffset = int(std::sin(wavePhase + w * 2.2f) * 2.0f);
            int wy = gy + cellSize - cellSize / 5 + waveOffset;
            painter.drawEllipse(wx, wy, waveW, cellSize / 5);
        }

        // Expressive Googly Cartoon Eyes that turn in direction of travel!
        if (ghostsFrightened) {
            painter.setBrush(QColor(255, 210, 210));
            int eyeSize = std::max(3, cellSize * 4 / 30);
            painter.drawEllipse(gx + cellSize * 8 / 30, gy + cellSize * 10 / 30, eyeSize, eyeSize);
            painter.drawEllipse(gx + cellSize * 18 / 30, gy + cellSize * 10 / 30, eyeSize, eyeSize);

            // Shivering panic wavy mouth
            painter.setPen(QPen(QColor(255, 210, 210), 1.5));
            for (int m = 0; m < 4; ++m) {
                int mx = gx + cellSize * 6 / 30 + m * (cellSize * 5 / 30);
                int my = gy + cellSize * 18 / 30 + (m % 2 == 0 ? 0 : 2);
                painter.drawLine(mx, my, mx + cellSize * 4 / 30, my + (m % 2 == 0 ? 2 : -2));
            }
            painter.setPen(Qt::NoPen);
        } else {
            int eyeW = std::max(5, cellSize * 7 / 30);
            int eyeH = std::max(6, cellSize * 9 / 30);
            int eye1X = gx + cellSize * 5 / 30;
            int eye2X = gx + cellSize * 16 / 30;
            int eyeY = gy + cellSize * 7 / 30;

            // Sclera (White)
            painter.setBrush(Qt::white);
            painter.drawEllipse(eye1X, eyeY, eyeW, eyeH);
            painter.drawEllipse(eye2X, eyeY, eyeW, eyeH);

            // Pupils pointing towards movement direction (g.dir)
            int pdx = 0, pdy = 0;
            if (g.dir == UP) pdy = -2;
            else if (g.dir == DOWN) pdy = 2;
            else if (g.dir == LEFT) pdx = -2;
            else if (g.dir == RIGHT) pdx = 2;

            int pupilSize = std::max(3, cellSize * 4 / 30);
            painter.setBrush(QColor(20, 50, 180));
            painter.drawEllipse(eye1X + (eyeW - pupilSize)/2 + pdx, eyeY + (eyeH - pupilSize)/2 + pdy, pupilSize, pupilSize);
            painter.drawEllipse(eye2X + (eyeW - pupilSize)/2 + pdx, eyeY + (eyeH - pupilSize)/2 + pdy, pupilSize, pupilSize);

            // Catchlight glints
            painter.setBrush(Qt::white);
            painter.drawEllipse(eye1X + (eyeW - pupilSize)/2 + pdx + 1, eyeY + (eyeH - pupilSize)/2 + pdy, std::max(1, pupilSize/3), std::max(1, pupilSize/3));
            painter.drawEllipse(eye2X + (eyeW - pupilSize)/2 + pdx + 1, eyeY + (eyeH - pupilSize)/2 + pdy, std::max(1, pupilSize/3), std::max(1, pupilSize/3));
        }

        // Mode B: Show player name tag if controlled by host or classmate
        if (isHostGhost() && getHostGhostIndex() == g.colorIndex) {
            painter.setFont(QFont("Arial", std::max(7, cellSize / 4), QFont::Bold));
            painter.setPen(QColor(255, 220, 0)); // Glowing gold badge for host
            painter.drawText(gx - 25, gy - 6, cellSize + 50, cellSize / 2, Qt::AlignCenter, "★ HOST");
        } else if (g.clientId >= 0 && !g.playerName.isEmpty()) {
            painter.setFont(QFont("Arial", std::max(7, cellSize / 4), QFont::Bold));
            painter.setPen(ghostNormalColors[g.colorIndex]);
            painter.drawText(gx - 20, gy - 6, cellSize + 40, cellSize / 2, Qt::AlignCenter, QString("[%1]").arg(g.playerName));
        }
    }

    // 5. Lantern Light Spotlight (Fused multi-hero & host ghost lighting)
    if (isCurrent && fogOfWarEnabled && !fogPeekActive) {
        int secW = COLS * cellSize;
        int secH = ROWS * cellSize;
        QPixmap fogPix(secW, secH);
        fogPix.fill(QColor(4, 6, 14, 235));

        QPainter maskPainter(&fogPix);
        maskPainter.setRenderHint(QPainter::Antialiasing);
        maskPainter.setCompositionMode(QPainter::CompositionMode_DestinationOut);

        auto carveLight = [&](int hx, int hy, double r) {
            int cx = hx * cellSize + cellSize / 2;
            int cy = hy * cellSize + cellSize / 2;
            int radiusPx = int((r + 0.6) * cellSize);

            QRadialGradient grad(cx, cy, radiusPx);
            grad.setColorAt(0.0, QColor(0, 0, 0, 255));
            grad.setColorAt(0.65, QColor(0, 0, 0, 220));
            grad.setColorAt(1.0, QColor(0, 0, 0, 0));

            maskPainter.fillRect(cx - radiusPx, cy - radiusPx, radiusPx * 2, radiusPx * 2, grad);
        };

        if (isP1Active() && p1Alive) {
            carveLight(pacmanX, pacmanY, currentVisionRadius);
        }
        if (isP2Active() && p2Alive) {
            carveLight(p2X, p2Y, currentVisionRadius);
        }
        if ((hero3.clientId != -1 || hostRole == "p3") && hero3.alive) {
            carveLight(hero3.x, hero3.y, currentVisionRadius);
        }
        if ((hero4.clientId != -1 || hostRole == "p4") && hero4.alive) {
            carveLight(hero4.x, hero4.y, currentVisionRadius);
        }
        if (isHostGhost()) {
            int hgi = getHostGhostIndex();
            if (hgi >= 0 && hgi < ghosts.size()) {
                if (ghosts[hgi].sectorX == sx && ghosts[hgi].sectorY == sy) {
                    carveLight(ghosts[hgi].x, ghosts[hgi].y, currentVisionRadius);
                }
            }
        }
        maskPainter.end();

        painter.drawPixmap(originX, originY, fogPix);
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

    // 7. Dynamic Stargate Vortex EXIT Portal (Hidden until Pacman is close or Fog Peek is triggered)
    if (sx == exitSectorX && sy == exitSectorY) {
        bool exitVisible = isCellVisible(sx, sy, exitX, exitY);

        if (!fogOfWarEnabled || exitVisible) {
            float pcx = originX + (exitX + 0.5f) * cellSize;
            float pcy = originY + (exitY + 0.5f) * cellSize;
            float pr = cellSize * 0.50f;
            float exitPulse = (std::sin(pulseTime * 2.4f) + 1.0f) * 0.5f;
            float spinAngle = elapsedTimer.elapsed() * 0.08f;

            // Multi-tier Pulsating Cosmic Aura
            QRadialGradient exitAura(pcx, pcy, cellSize * 2.5f);
            exitAura.setColorAt(0.0, QColor(0, 255, 180, int(150 + 60 * exitPulse)));
            exitAura.setColorAt(0.4, QColor(0, 200, 110, int(60 + 40 * exitPulse)));
            exitAura.setColorAt(0.75, QColor(0, 140, 240, 20));
            exitAura.setColorAt(1.0, QColor(0, 0, 0, 0));
            painter.fillRect(int(pcx - cellSize * 2.5f), int(pcy - cellSize * 2.5f),
                             int(cellSize * 5.0f), int(cellSize * 5.0f), exitAura);

            // Swirling Outer Ripple Rings
            painter.save();
            painter.translate(pcx, pcy);

            for (int ring = 1; ring <= 3; ++ring) {
                float ringR = pr * (0.65f + ring * 0.22f + 0.08f * exitPulse);
                int ringAlpha = int(130 / ring + 35 * exitPulse);
                painter.setPen(QPen(QColor(0, 255, 200, ringAlpha), std::max(1, cellSize / 16), Qt::DashLine));
                painter.setBrush(Qt::NoBrush);
                painter.drawEllipse(QPointF(0, 0), ringR, ringR);
            }

            // Stargate Metallic Obsidian Ring Frame
            painter.setPen(QPen(QColor(0, 255, 170, 230), std::max(2, cellSize / 9)));
            QRadialGradient ringGrad(0, 0, pr);
            ringGrad.setColorAt(0.0, QColor(0, 255, 150, 180));
            ringGrad.setColorAt(0.65, QColor(10, 50, 42, 230));
            ringGrad.setColorAt(1.0, QColor(4, 24, 18, 255));
            painter.setBrush(ringGrad);
            painter.drawEllipse(QPointF(0, 0), pr, pr);

            // Glowing Concentric Event Horizon Core
            QRadialGradient coreGrad(0, 0, pr * 0.65f);
            coreGrad.setColorAt(0.0, QColor(255, 255, 255, 255));
            coreGrad.setColorAt(0.3, QColor(160, 255, 220, 230));
            coreGrad.setColorAt(0.7, QColor(0, 240, 150, 190));
            coreGrad.setColorAt(1.0, QColor(0, 160, 100, 100));
            painter.setBrush(coreGrad);
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(QPointF(0, 0), pr * 0.62f, pr * 0.62f);

            // Rotating 4-Point Star Core Sparkle
            painter.rotate(spinAngle);
            QPainterPath starSparkle;
            float starLen = pr * (0.46f + 0.12f * exitPulse);
            float starWid = pr * 0.14f;
            starSparkle.moveTo(0, -starLen);
            starSparkle.lineTo(starWid, -starWid);
            starSparkle.lineTo(starLen, 0);
            starSparkle.lineTo(starWid, starWid);
            starSparkle.lineTo(0, starLen);
            starSparkle.lineTo(-starWid, starWid);
            starSparkle.lineTo(-starLen, 0);
            starSparkle.lineTo(-starWid, -starWid);
            starSparkle.closeSubpath();
            painter.setBrush(QColor(255, 255, 255, 235));
            painter.fillPath(starSparkle, QColor(255, 255, 255, 235));

            painter.restore();

            // Floating "★ EXIT ★" Arcade Badge
            int badgeW = std::max(48, int(cellSize * 1.55f));
            int badgeH = std::max(15, int(cellSize * 0.42f));
            int badgeX = int(pcx - badgeW / 2);
            int badgeY = int(pcy + pr + 2);

            painter.setBrush(QColor(6, 26, 20, 230));
            painter.setPen(QPen(QColor(0, 255, 150, 235), 1.2));
            painter.drawRoundedRect(badgeX, badgeY, badgeW, badgeH, badgeH / 2, badgeH / 2);

            painter.setFont(QFont("Arial", std::max(7, int(badgeH * 0.60)), QFont::Bold));
            painter.setPen(QColor(255, 255, 255));
            painter.drawText(QRect(badgeX, badgeY, badgeW, badgeH), Qt::AlignCenter, "★ EXIT ★");
        }
    }
}

void MainWindow::drawGameScreen(QPainter &painter)
{
    int hudHeight = 74;
    int availW = width();
    int availH = height() - hudHeight;

    int cellSize = std::min(availW / COLS, availH / ROWS);
    if (cellSize < 12) cellSize = 12;

    int mazeW = COLS * cellSize;
    int mazeH = ROWS * cellSize;
    int startX = (availW - mazeW) / 2;
    int startY = (availH - mazeH) / 2;

    // Fill background letterbox with subtle dark grid atmosphere
    painter.fillRect(rect(), QColor(6, 7, 14));

    // Top-Left Saga Map Button (Glassmorphism Capsule)
    QRect mapBtn = getMapButtonRect();
    bool mapBtnHovered = mapBtn.contains(mapFromGlobal(QCursor::pos()));

    // Drop shadow
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 110));
    painter.drawRoundedRect(mapBtn.adjusted(1, 2, 1, 2), mapBtn.height() / 2, mapBtn.height() / 2);

    QLinearGradient mapBtnGrad(mapBtn.left(), mapBtn.top(), mapBtn.left(), mapBtn.bottom());
    if (mapBtnHovered) {
        mapBtnGrad.setColorAt(0.0, QColor(35, 75, 155, 240));
        mapBtnGrad.setColorAt(1.0, QColor(15, 35, 80, 240));
    } else {
        mapBtnGrad.setColorAt(0.0, QColor(20, 45, 95, 215));
        mapBtnGrad.setColorAt(1.0, QColor(10, 20, 50, 215));
    }
    painter.setBrush(mapBtnGrad);
    painter.setPen(QPen(mapBtnHovered ? QColor(0, 255, 240) : QColor(0, 180, 255, 180), mapBtnHovered ? 2.0 : 1.3));
    painter.drawRoundedRect(mapBtn, mapBtn.height() / 2, mapBtn.height() / 2);

    // Specular highlight
    QPainterPath mapGlint;
    mapGlint.addRoundedRect(QRectF(mapBtn.left() + 3, mapBtn.top() + 1, mapBtn.width() - 6, mapBtn.height() * 0.45),
                            (mapBtn.height() * 0.45) / 2, (mapBtn.height() * 0.45) / 2);
    painter.fillPath(mapGlint, QColor(255, 255, 255, 30));

    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.setPen(mapBtnHovered ? Qt::white : QColor(220, 245, 255));
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
    // Bottom Fullscreen HUD Bar (Arcade Glass Command Deck)
    // ========================================================
    int hudY = height() - hudHeight;

    // 1. Frosted Glassmorphism Panel
    QLinearGradient hudGrad(0, hudY, 0, hudY + hudHeight);
    hudGrad.setColorAt(0.0, QColor(14, 18, 38, 245));
    hudGrad.setColorAt(1.0, QColor(6, 8, 18, 255));
    painter.fillRect(0, hudY, width(), hudHeight, hudGrad);

    // Glowing Top Border Neon Highlight
    QLinearGradient topBorderGrad(0, hudY, width(), hudY);
    topBorderGrad.setColorAt(0.0, QColor(0, 240, 255, 120));
    topBorderGrad.setColorAt(0.3, QColor(0, 255, 180, 200));
    topBorderGrad.setColorAt(0.7, QColor(255, 90, 200, 200));
    topBorderGrad.setColorAt(1.0, QColor(0, 240, 255, 120));
    painter.setPen(QPen(topBorderGrad, 2));
    painter.drawLine(0, hudY, width(), hudY);

    // Helper: Draw Glass Capsule Pill Badge
    auto drawHudPill = [&](int x, int y, int w, int h, const QString &text, const QColor &bgCol, const QColor &borderCol, const QColor &textCol, bool pulsing = false) {
        // Drop shadow
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 95));
        painter.drawRoundedRect(x + 1, y + 2, w, h, h / 2, h / 2);

        // Body with gradient
        QLinearGradient bg(x, y, x, y + h);
        bg.setColorAt(0.0, bgCol.lighter(135));
        bg.setColorAt(1.0, bgCol);
        painter.setBrush(bg);

        float penW = pulsing ? (1.5f + 0.8f * std::sin(elapsedTimer.elapsed() * 0.012f)) : 1.3f;
        painter.setPen(QPen(borderCol, penW));
        painter.drawRoundedRect(x, y, w, h, h / 2, h / 2);

        // Top specular arc highlight
        QPainterPath glint;
        glint.addRoundedRect(QRectF(x + 2, y + 1, w - 4, h * 0.44), (h * 0.44) / 2, (h * 0.44) / 2);
        painter.fillPath(glint, QColor(255, 255, 255, 30));

        // Text
        painter.setFont(QFont("Arial", std::clamp(int(h * 0.46), 8, 11), QFont::Bold));
        painter.setPen(textCol);
        painter.drawText(QRect(x, y, w, h), Qt::AlignCenter, text);
    };

    // Count discovered sectors for HUD
    int discCount = 0;
    for (int sy = 0; sy < sectorRows; ++sy) {
        for (int sx = 0; sx < sectorCols; ++sx) {
            if (sectorDiscovered[sy][sx]) discCount++;
        }
    }

    int pillH = 22;
    int pillY = hudY + 5;
    int curX = 14;
    int gap = 8;
    int worldNum = (currentLevel - 1) / LEVELS_PER_WORLD + 1;
    int stageNum = (currentLevel - 1) % LEVELS_PER_WORLD + 1;

    if (!isAnyMultiplayer()) {
        // 1. SCORE
        QString scoreStr = QString("🏆 %1").arg(score);
        int scoreW = std::max(78, int(width() * 0.085));
        drawHudPill(curX, pillY, scoreW, pillH, scoreStr, QColor(48, 36, 10), QColor(255, 215, 0), QColor(255, 240, 150));
        curX += scoreW + gap;

        // 2. WORLD & STAGE
        QString wsStr = QString("🪐 W%1-S%2 (L%3)").arg(worldNum).arg(stageNum).arg(currentLevel);
        int wsW = std::max(105, int(width() * 0.11));
        drawHudPill(curX, pillY, wsW, pillH, wsStr, QColor(12, 38, 55), QColor(0, 230, 180), QColor(200, 255, 240));
        curX += wsW + gap;

        // 3. SECTOR
        QString secStr = QString("🧭 (%1,%2)").arg(currentSectorX + 1).arg(currentSectorY + 1);
        int secW = std::max(68, int(width() * 0.075));
        drawHudPill(curX, pillY, secW, pillH, secStr, QColor(14, 28, 60), QColor(0, 200, 255), QColor(180, 240, 255));
        curX += secW + gap;

        // 4. EXPLORED SECTORS
        QString expStr = QString("🗺️ %1 SEC").arg(discCount);
        int expW = std::max(68, int(width() * 0.075));
        drawHudPill(curX, pillY, expW, pillH, expStr, QColor(32, 20, 52), QColor(190, 120, 255), QColor(235, 210, 255));
        curX += expW + gap;

        // 5. LIVES
        QString livesStr;
        if (p1Lives <= 0) {
            livesStr = "💀 0 LIVES";
            int livW = std::max(90, int(width() * 0.09));
            drawHudPill(curX, pillY, livW, pillH, livesStr, QColor(50, 14, 18), QColor(255, 60, 80), QColor(255, 140, 150), true);
            curX += livW + gap;
        } else if (!p1Alive && p1RespawnTicks > 0) {
            livesStr = QString("⏳ RESPAWN %1s").arg((p1RespawnTicks * 130 + 999) / 1000);
            int livW = std::max(115, int(width() * 0.12));
            drawHudPill(curX, pillY, livW, pillH, livesStr, QColor(52, 32, 12), QColor(255, 160, 40), QColor(255, 220, 140), true);
            curX += livW + gap;
        } else {
            QString hearts;
            for (int i = 0; i < p1Lives; ++i) hearts += "❤️";
            livesStr = QString("💖 %1").arg(hearts);
            int livW = std::max(85, int(width() * 0.085));
            QColor lBg = p1Lives > 1 ? QColor(48, 16, 32) : QColor(55, 20, 10);
            QColor lBorder = p1Lives > 1 ? QColor(255, 80, 140) : QColor(255, 140, 0);
            drawHudPill(curX, pillY, livW, pillH, livesStr, lBg, lBorder, Qt::white, p1Lives == 1);
            curX += livW + gap;
        }
    } else {
        // MULTIPLAYER TEAM DASHBOARD
        auto formatHearts = [](int lives) {
            if (lives <= 0) return QString("💀 OUT");
            QString h;
            for (int i = 0; i < lives; ++i) h += "❤️";
            return h;
        };

        // P1 Pill
        if (isP1Active()) {
            QString p1Str;
            QColor p1Bg, p1Border;
            if (p1Lives <= 0) {
                p1Str = "🟡 P1: 💀 OUT";
                p1Bg = QColor(48, 14, 18); p1Border = QColor(255, 60, 80);
            } else if (p1ReachedExit) {
                p1Str = QString("🟡 P1: %1 🏁").arg(scoreP1);
                p1Bg = QColor(10, 48, 28); p1Border = QColor(0, 255, 150);
            } else if (!p1Alive && p1RespawnTicks > 0) {
                p1Str = QString("🟡 P1: ⏳%1s").arg((p1RespawnTicks * 130 + 999) / 1000);
                p1Bg = QColor(50, 32, 10); p1Border = QColor(255, 160, 40);
            } else {
                p1Str = QString("🟡 P1: %1 %2").arg(scoreP1).arg(formatHearts(p1Lives));
                p1Bg = QColor(42, 36, 12); p1Border = QColor(255, 220, 40);
            }
            int pW = std::max(105, int(width() * 0.105));
            drawHudPill(curX, pillY, pW, pillH, p1Str, p1Bg, p1Border, Qt::white);
            curX += pW + gap;
        }

        // P2 Pill
        if (isP2Active()) {
            QString p2Str;
            QColor p2Bg, p2Border;
            if (p2Lives <= 0) {
                p2Str = "🎀 P2: 💀 OUT";
                p2Bg = QColor(48, 14, 18); p2Border = QColor(255, 60, 80);
            } else if (p2ReachedExit) {
                p2Str = QString("🎀 P2: %1 🏁").arg(scoreP2);
                p2Bg = QColor(10, 48, 28); p2Border = QColor(0, 255, 150);
            } else if (!p2Alive && p2RespawnTicks > 0) {
                p2Str = QString("🎀 P2: ⏳%1s").arg((p2RespawnTicks * 130 + 999) / 1000);
                p2Bg = QColor(50, 32, 10); p2Border = QColor(255, 160, 40);
            } else {
                p2Str = QString("🎀 P2: %1 %2").arg(scoreP2).arg(formatHearts(p2Lives));
                p2Bg = QColor(48, 16, 38); p2Border = QColor(255, 105, 180);
            }
            int pW = std::max(105, int(width() * 0.105));
            drawHudPill(curX, pillY, pW, pillH, p2Str, p2Bg, p2Border, Qt::white);
            curX += pW + gap;
        }

        // P3 Pill
        if (hero3.clientId != -1 || hostRole == "p3") {
            QString p3Str;
            QColor p3Bg, p3Border;
            if (hero3.lives <= 0) {
                p3Str = "🧢 P3: 💀 OUT";
                p3Bg = QColor(48, 14, 18); p3Border = QColor(255, 60, 80);
            } else if (hero3.reachedExit) {
                p3Str = QString("🧢 P3: %1 🏁").arg(hero3.score);
                p3Bg = QColor(10, 48, 28); p3Border = QColor(0, 255, 150);
            } else if (!hero3.alive && hero3.respawnTicks > 0) {
                p3Str = QString("🧢 P3: ⏳%1s").arg((hero3.respawnTicks * 130 + 999) / 1000);
                p3Bg = QColor(50, 32, 10); p3Border = QColor(255, 160, 40);
            } else {
                p3Str = QString("🧢 P3: %1 %2").arg(hero3.score).arg(formatHearts(hero3.lives));
                p3Bg = QColor(10, 36, 52); p3Border = QColor(0, 229, 255);
            }
            int pW = std::max(105, int(width() * 0.105));
            drawHudPill(curX, pillY, pW, pillH, p3Str, p3Bg, p3Border, Qt::white);
            curX += pW + gap;
        }

        // P4 Pill
        if (hero4.clientId != -1 || hostRole == "p4") {
            QString p4Str;
            QColor p4Bg, p4Border;
            if (hero4.lives <= 0) {
                p4Str = "🟢 P4: 💀 OUT";
                p4Bg = QColor(48, 14, 18); p4Border = QColor(255, 60, 80);
            } else if (hero4.reachedExit) {
                p4Str = QString("🟢 P4: %1 🏁").arg(hero4.score);
                p4Bg = QColor(10, 48, 28); p4Border = QColor(0, 255, 150);
            } else if (!hero4.alive && hero4.respawnTicks > 0) {
                p4Str = QString("🟢 P4: ⏳%1s").arg((hero4.respawnTicks * 130 + 999) / 1000);
                p4Bg = QColor(50, 32, 10); p4Border = QColor(255, 160, 40);
            } else {
                p4Str = QString("🟢 P4: %1 %2").arg(hero4.score).arg(formatHearts(hero4.lives));
                p4Bg = QColor(10, 44, 25); p4Border = QColor(0, 255, 120);
            }
            int pW = std::max(105, int(width() * 0.105));
            drawHudPill(curX, pillY, pW, pillH, p4Str, p4Bg, p4Border, Qt::white);
            curX += pW + gap;
        }

        // Host Role Indicator
        QString hostStr = QString("★ HOST: %1 %2").arg(getRoleIcon(hostRole), getRoleDisplayName(hostRole));
        int hostW = std::max(115, int(width() * 0.115));
        QColor hBg = isHostGhost() ? QColor(48, 18, 22) : QColor(48, 38, 14);
        QColor hBorder = isHostGhost() ? QColor(255, 100, 100) : QColor(255, 215, 60);
        drawHudPill(curX, pillY, hostW, pillH, hostStr, hBg, hBorder, Qt::white);
        curX += hostW + gap;

        // Team Total Score
        QString tScoreStr = QString("🏆 %1").arg(score);
        int tW = std::max(78, int(width() * 0.075));
        drawHudPill(curX, pillY, tW, pillH, tScoreStr, QColor(40, 32, 12), QColor(255, 215, 0), QColor(255, 240, 140));
        curX += tW + gap;

        // World / Stage
        QString wsStr = QString("🪐 W%1-S%2").arg(worldNum).arg(stageNum);
        int wsW = std::max(75, int(width() * 0.07));
        drawHudPill(curX, pillY, wsW, pillH, wsStr, QColor(12, 38, 55), QColor(0, 230, 180), QColor(200, 255, 240));
        curX += wsW + gap;

        // Sector
        QString secStr = QString("🧭 (%1,%2)").arg(currentSectorX + 1).arg(currentSectorY + 1);
        int sW = std::max(64, int(width() * 0.06));
        drawHudPill(curX, pillY, sW, pillH, secStr, QColor(14, 28, 60), QColor(0, 200, 255), QColor(180, 240, 255));
        curX += sW + gap;
    }

    // Right-aligned Utility Pills (Timer, Fog Peek, Power)
    qint64 elapsedSec = elapsedTimer.elapsed() / 1000;
    int mins = elapsedSec / 60;
    int secs = elapsedSec % 60;
    QString timeStr = QString("⏱️ %1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
    int timeW = 68;
    int timeX = width() - timeW - 14;
    drawHudPill(timeX, pillY, timeW, pillH, timeStr, QColor(25, 20, 48), QColor(170, 130, 240), QColor(230, 215, 255));

    // Fog Peek pill
    int peekW = 108;
    int peekX = timeX - peekW - gap;
    if (fogPeekActive) {
        int msLeft = std::max(0, FOG_PEEK_DURATION_MS - (int)fogPeekTimer.elapsed());
        double secLeft = msLeft / 1000.0;
        QString peekStr = QString("💡 PEEK %1s").arg(secLeft, 0, 'f', 1);
        drawHudPill(peekX, pillY, peekW, pillH, peekStr, QColor(50, 42, 10), QColor(255, 230, 50), QColor(255, 255, 200), true);
    } else if (p1FogPeeks > 0) {
        QString peekStr = QString("🏮 PEEK [F]: %1").arg(p1FogPeeks);
        drawHudPill(peekX, pillY, peekW, pillH, peekStr, QColor(12, 36, 44), QColor(0, 220, 240), QColor(200, 255, 255));
    } else {
        drawHudPill(peekX, pillY, peekW, pillH, "🏮 PEEK: 0 (Fruit)", QColor(44, 18, 20), QColor(255, 100, 110), QColor(255, 180, 190));
    }

    // Frightened Power pill
    if (ghostsFrightened) {
        int pwrW = 95;
        int pwrX = peekX - pwrW - gap;
        QString pwrStr = QString("⚡ POWER! %1").arg(frightenedTicksLeft);
        drawHudPill(pwrX, pillY, pwrW, pillH, pwrStr, QColor(50, 14, 45), QColor(255, 80, 200), Qt::white, true);
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

        painter.setFont(QFont("Arial", std::max(11, width() / 110), QFont::Bold));
        painter.setPen(borderCol);
        painter.drawText(tX, tY, tW, tH, Qt::AlignCenter, peekToastMessage);
        painter.restore();
    }

    // Line 2: Sleek Mission Objective Ribbon
    int curWorld = (currentLevel - 1) / LEVELS_PER_WORLD + 1;
    int curStage = (currentLevel - 1) % LEVELS_PER_WORLD + 1;
    QString objectiveText;
    QColor objColor = QColor(255, 220, 90);
    int elig = getEligibleHeroCount();
    int rch = getReachedExitHeroCount();

    if (isAnyMultiplayer() && elig > 1 && rch > 0 && rch < elig) {
        objectiveText = QString("🏁 ALL PLAYERS MUST REACH EXIT! (%1/%2 arrived • Waiting for teammates to reach portal...)").arg(rch).arg(elig);
        objColor = QColor(0, 255, 240);
    } else if (currentSectorX == exitSectorX && currentSectorY == exitSectorY) {
        bool exitNear = isCellVisible(currentSectorX, currentSectorY, exitX, exitY) || (!fogOfWarEnabled);
        if (exitNear) {
            objectiveText = isAnyMultiplayer()
                ? QString("🏁 OBJECTIVE: All alive players must enter the green [EXIT] portal to clear Stage %1/20 (%2)!").arg(curStage).arg(getWorldName(curWorld))
                : QString("🏁 OBJECTIVE: Enter the green [EXIT] portal to clear Stage %1/20 (%2)!").arg(curStage).arg(getWorldName(curWorld));
            objColor = QColor(60, 255, 140);
        } else {
            objectiveText = QString("🧭 OBJECTIVE: Search the dark labyrinth with your lantern to locate the hidden [EXIT] portal! (%1)").arg(getWorldName(curWorld));
            objColor = QColor(255, 215, 60);
        }
    } else {
        if (!fogOfWarEnabled) {
            QString dirStr;
            if (currentSectorX < exitSectorX) dirStr = "RIGHT wall [▶ GATEWAY]";
            else if (currentSectorX > exitSectorX) dirStr = "LEFT wall [◀ GATEWAY]";
            else if (currentSectorY < exitSectorY) dirStr = "BOTTOM wall [▼ GATEWAY]";
            else if (currentSectorY > exitSectorY) dirStr = "TOP wall [▲ GATEWAY]";
            objectiveText = QString("💡 PEEK REVEAL: Head toward the %1 to reach the exit sector! (%2)").arg(dirStr).arg(getWorldName(curWorld));
            objColor = QColor(0, 255, 240);
        } else {
            objectiveText = QString("🧭 OBJECTIVE: Explore the dark corridors with your lantern to discover gateways! (%1)").arg(getWorldName(curWorld));
            objColor = QColor(255, 220, 80);
        }
    }

    int objBoxW = width() - 28;
    int objBoxH = 18;
    int objBoxY = hudY + 31;
    painter.setBrush(QColor(10, 14, 28, 160));
    painter.setPen(QPen(objColor.darker(150), 1));
    painter.drawRoundedRect(14, objBoxY, objBoxW, objBoxH, 4, 4);

    painter.setFont(QFont("Arial", std::clamp(int(width() / 115), 9, 11), QFont::Bold));
    painter.setPen(objColor);
    painter.drawText(QRect(20, objBoxY, objBoxW - 12, objBoxH), Qt::AlignVCenter | Qt::AlignLeft, objectiveText);

    // Line 3: Modern Keycap Shortcuts Bar
    int ctrlY = hudY + 52;
    painter.setFont(QFont("Arial", std::clamp(int(width() / 135), 8, 10)));
    painter.setPen(QColor(150, 175, 215));

    if (!isAnyMultiplayer()) {
        QString heroName = (singlePlayerHero == 2) ? "🎀 Ms. Pacman" : "🟡 Pacman";
        painter.drawText(20, ctrlY + 14,
            QString("HERO: [%1]   •   [Arrows / WASD] Move   •   [Tab] Switch Character   •   [F] Fog Lantern Peek   •   [M] Saga Map   •   [F11] Fullscreen")
            .arg(heroName));
    } else {
        QString roleDisplay = QString("%1 %2").arg(getRoleIcon(hostRole), getRoleDisplayName(hostRole));
        if (isHostGhost()) {
            painter.drawText(20, ctrlY + 14,
                QString("HOST ROLE: [%1]   •   [Arrows / WASD] Move Ghost   •   [F] Ghost Sonar   •   [Tab] Switch Role   •   [H] Multiplayer Lobby   •   [M] Saga Map")
                .arg(roleDisplay));
        } else {
            painter.drawText(20, ctrlY + 14,
                QString("HOST ROLE: [%1]   •   [Arrows / WASD] Move Hero   •   [F] Fog Peek (%2 left)   •   [Tab] Switch Hero   •   [H] Multiplayer Lobby   •   [M] Saga Map")
                .arg(roleDisplay).arg(p1FogPeeks));
        }
    }

    // ========================================================
    // Frosted Glass Modals: Game Over & Level Complete
    // ========================================================
    if (gameOver) {
        // Darkened backdrop scrim
        painter.fillRect(rect(), QColor(5, 4, 12, 215));

        int modalW = std::clamp(int(width() * 0.62f), 480, 680);
        int modalH = 220;
        int modalX = (width() - modalW) / 2;
        int modalY = (height() - modalH) / 2;

        // Drop shadow
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 160));
        painter.drawRoundedRect(modalX + 4, modalY + 6, modalW, modalH, 20, 20);

        // Frosted Card Body
        QLinearGradient cardGrad(modalX, modalY, modalX, modalY + modalH);
        cardGrad.setColorAt(0.0, QColor(36, 12, 22, 250));
        cardGrad.setColorAt(1.0, QColor(14, 6, 12, 255));
        painter.setBrush(cardGrad);

        float pulse = (std::sin(elapsedTimer.elapsed() * 0.008f) + 1.0f) * 0.5f;
        painter.setPen(QPen(QColor(255, 50, 80, int(180 + 75 * pulse)), 2.5));
        painter.drawRoundedRect(modalX, modalY, modalW, modalH, 20, 20);

        // Title Header
        painter.setFont(QFont("Arial", std::max(20, modalW / 24), QFont::Bold));
        painter.setPen(QColor(255, 60, 80));
        painter.drawText(modalX, modalY + 38, modalW, 32, Qt::AlignCenter, "💀 GAME OVER 💀");

        // Subtitle Details
        painter.setFont(QFont("Arial", std::max(12, modalW / 42)));
        painter.setPen(QColor(255, 200, 210));
        QString subMsg = isAnyMultiplayer()
            ? QString("All Players Lost Their 3 Lives in Stage %1/20 (%2)").arg(curStage).arg(getWorldName(curWorld))
            : QString("All 3 Lives Lost in Stage %1/20 (%2)").arg(curStage).arg(getWorldName(curWorld));
        painter.drawText(modalX, modalY + 76, modalW, 24, Qt::AlignCenter, subMsg);

        // Stat Badges
        int badgeW = (modalW - 50) / 2;
        int b1X = modalX + 20;
        int b2X = modalX + 30 + badgeW;
        int bY = modalY + 110;
        int bH = 34;

        // Stat 1: Total Score
        painter.setBrush(QColor(24, 8, 14, 200));
        painter.setPen(QPen(QColor(255, 100, 120, 160), 1.2));
        painter.drawRoundedRect(b1X, bY, badgeW, bH, 8, 8);
        painter.setFont(QFont("Arial", 11, QFont::Bold));
        painter.setPen(QColor(255, 220, 120));
        painter.drawText(QRect(b1X, bY, badgeW, bH), Qt::AlignCenter, QString("🏆 FINAL SCORE: %1").arg(score));

        // Stat 2: Sectors Explored
        painter.setBrush(QColor(24, 8, 14, 200));
        painter.setPen(QPen(QColor(255, 100, 120, 160), 1.2));
        painter.drawRoundedRect(b2X, bY, badgeW, bH, 8, 8);
        painter.setPen(QColor(180, 220, 255));
        painter.drawText(QRect(b2X, bY, badgeW, bH), Qt::AlignCenter, QString("🧭 EXPLORED: %1 SECTORS").arg(discCount));

        // Action Prompts
        int promptY = modalY + 165;
        painter.setFont(QFont("Arial", 12, QFont::Bold));
        painter.setPen(QColor(255, 230, 120));
        painter.drawText(modalX, promptY, modalW, 22, Qt::AlignCenter,
                         "Press [Space] or [R] to Restart Stage   •   Press [M] for Saga Map");

    } else if (levelComplete) {
        // Darkened backdrop scrim with festive emerald tone
        painter.fillRect(rect(), QColor(4, 12, 10, 215));

        int modalW = std::clamp(int(width() * 0.62f), 480, 680);
        int modalH = 225;
        int modalX = (width() - modalW) / 2;
        int modalY = (height() - modalH) / 2;

        // Drop shadow
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 160));
        painter.drawRoundedRect(modalX + 4, modalY + 6, modalW, modalH, 20, 20);

        // Frosted Card Body
        QLinearGradient cardGrad(modalX, modalY, modalX, modalY + modalH);
        cardGrad.setColorAt(0.0, QColor(10, 36, 26, 250));
        cardGrad.setColorAt(1.0, QColor(4, 16, 12, 255));
        painter.setBrush(cardGrad);

        float pulse = (std::sin(elapsedTimer.elapsed() * 0.008f) + 1.0f) * 0.5f;
        painter.setPen(QPen(QColor(0, 255, 160, int(180 + 75 * pulse)), 2.5));
        painter.drawRoundedRect(modalX, modalY, modalW, modalH, 20, 20);

        // Celebration Title Header
        painter.setFont(QFont("Arial", std::max(20, modalW / 24), QFont::Bold));
        painter.setPen(QColor(0, 255, 160));
        painter.drawText(modalX, modalY + 36, modalW, 32, Qt::AlignCenter, "✨ STAGE COMPLETE! ✨");

        // 3 Golden Bouncing Celebration Stars
        float starBob = 3.0f * std::sin(elapsedTimer.elapsed() * 0.009f);
        painter.setFont(QFont("Arial", 22));
        painter.setPen(QColor(255, 215, 0));
        painter.drawText(modalX, int(modalY + 68 + starBob), modalW, 28, Qt::AlignCenter, "⭐ ⭐ ⭐");

        // Subtitle
        painter.setFont(QFont("Arial", std::max(12, modalW / 42)));
        painter.setPen(QColor(200, 255, 225));
        QString clrMsg = isAnyMultiplayer()
            ? QString("Team Escaped Stage %1/20 (%2)!").arg(curStage).arg(getWorldName(curWorld))
            : QString("Labyrinth Escaped: Stage %1/20 (%2)!").arg(curStage).arg(getWorldName(curWorld));
        painter.drawText(modalX, modalY + 104, modalW, 22, Qt::AlignCenter, clrMsg);

        // Stat Badges
        int badgeW = (modalW - 50) / 2;
        int b1X = modalX + 20;
        int b2X = modalX + 30 + badgeW;
        int bY = modalY + 132;
        int bH = 32;

        painter.setBrush(QColor(8, 26, 18, 200));
        painter.setPen(QPen(QColor(0, 230, 140, 160), 1.2));
        painter.drawRoundedRect(b1X, bY, badgeW, bH, 8, 8);
        painter.setFont(QFont("Arial", 11, QFont::Bold));
        painter.setPen(QColor(255, 230, 100));
        painter.drawText(QRect(b1X, bY, badgeW, bH), Qt::AlignCenter, QString("🏆 SCORE: %1").arg(score));

        painter.setBrush(QColor(8, 26, 18, 200));
        painter.setPen(QPen(QColor(0, 230, 140, 160), 1.2));
        painter.drawRoundedRect(b2X, bY, badgeW, bH, 8, 8);
        painter.setPen(QColor(160, 255, 220));
        painter.drawText(QRect(b2X, bY, badgeW, bH), Qt::AlignCenter, QString("⏱️ TIME: %1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0')));

        // Action Prompts
        int promptY = modalY + 178;
        painter.setFont(QFont("Arial", 12, QFont::Bold));
        painter.setPen(QColor(255, 235, 120));
        painter.drawText(modalX, promptY, modalW, 22, Qt::AlignCenter,
                         "Press [Space] for Next Stage   •   Press [M] for Saga Map");
    }

    // Top-Right Badges
    drawSoundBadge(painter);
    drawMultiplayerBadge(painter);

    // Classroom Multiplayer Lobby Overlay
    if (showMultiplayerLobby) {
        drawMultiplayerLobby(painter);
    }
}

QRect MainWindow::getSoundButtonRect() const
{
    return QRect(width() - 325, 14, 68, 36);
}

void MainWindow::drawSoundBadge(QPainter &painter)
{
    QRect btn = getSoundButtonRect();
    bool hovered = btn.contains(mapFromGlobal(QCursor::pos()));
    bool muted = SoundManager::instance().isMuted();

    // Drop shadow
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 110));
    painter.drawRoundedRect(btn.adjusted(1, 2, 1, 2), btn.height() / 2, btn.height() / 2);

    QLinearGradient bgGrad(btn.left(), btn.top(), btn.left(), btn.bottom());
    if (muted) {
        if (hovered) {
            bgGrad.setColorAt(0.0, QColor(70, 30, 30, 240));
            bgGrad.setColorAt(1.0, QColor(40, 15, 15, 240));
        } else {
            bgGrad.setColorAt(0.0, QColor(50, 20, 20, 215));
            bgGrad.setColorAt(1.0, QColor(28, 10, 10, 215));
        }
    } else {
        if (hovered) {
            bgGrad.setColorAt(0.0, QColor(35, 75, 155, 240));
            bgGrad.setColorAt(1.0, QColor(15, 35, 80, 240));
        } else {
            bgGrad.setColorAt(0.0, QColor(20, 45, 95, 215));
            bgGrad.setColorAt(1.0, QColor(10, 20, 50, 215));
        }
    }
    painter.setBrush(bgGrad);
    painter.setPen(QPen(muted ? (hovered ? QColor(255, 100, 100) : QColor(220, 70, 70, 180))
                              : (hovered ? QColor(0, 255, 240) : QColor(0, 180, 255, 180)),
                        hovered ? 2.0 : 1.3));
    painter.drawRoundedRect(btn, btn.height() / 2, btn.height() / 2);

    // Specular highlight
    QPainterPath glint;
    glint.addRoundedRect(QRectF(btn.left() + 3, btn.top() + 1, btn.width() - 6, btn.height() * 0.45),
                         (btn.height() * 0.45) / 2, (btn.height() * 0.45) / 2);
    painter.fillPath(glint, QColor(255, 255, 255, 30));

    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.setPen(muted ? (hovered ? Qt::white : QColor(255, 140, 140))
                         : (hovered ? Qt::white : QColor(210, 240, 255)));
    QString text = muted ? "🔇 [S]" : "🔊 [S]";
    painter.drawText(btn, Qt::AlignCenter, text);
}

QRect MainWindow::getMultiplayerButtonRect() const
{
    return QRect(width() - 250, 14, 235, 36);
}

QRect MainWindow::getLobbyModalRect() const
{
    int modalW = std::min(780, width() - 40);
    int modalH = std::min(590, height() - 50);
    int modalX = (width() - modalW) / 2;
    int modalY = (height() - modalH) / 2;
    return QRect(modalX, modalY, modalW, modalH);
}

QRect MainWindow::getLobbyCloseButtonRect() const
{
    QRect m = getLobbyModalRect();
    return QRect(m.right() - 44, m.top() + 14, 30, 30);
}

QRect MainWindow::getLobbyUrlBoxRect() const
{
    QRect m = getLobbyModalRect();
    return QRect(m.left() + 35, m.top() + 78, m.width() - 230, 48);
}

QRect MainWindow::getCopyLinkButtonRect() const
{
    QRect m = getLobbyModalRect();
    return QRect(m.left() + m.width() - 180, m.top() + 78, 145, 48);
}

void MainWindow::drawMultiplayerBadge(QPainter &painter)
{
    QRect btn = getMultiplayerButtonRect();
    bool hovered = btn.contains(mapFromGlobal(QCursor::pos()));

    int count = gameServer ? gameServer->getConnectedClientCount() : 0;

    // Drop shadow
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 110));
    painter.drawRoundedRect(btn.adjusted(1, 2, 1, 2), btn.height() / 2, btn.height() / 2);

    QLinearGradient bgGrad(btn.left(), btn.top(), btn.left(), btn.bottom());
    if (hovered) {
        bgGrad.setColorAt(0.0, QColor(35, 75, 155, 240));
        bgGrad.setColorAt(1.0, QColor(15, 35, 80, 240));
    } else {
        bgGrad.setColorAt(0.0, QColor(20, 45, 95, 215));
        bgGrad.setColorAt(1.0, QColor(10, 20, 50, 215));
    }
    painter.setBrush(bgGrad);
    painter.setPen(QPen(hovered ? QColor(0, 255, 240) : (count > 0 ? QColor(0, 255, 180) : QColor(0, 180, 255, 180)), hovered ? 2.0 : 1.3));
    painter.drawRoundedRect(btn, btn.height() / 2, btn.height() / 2);

    // Specular highlight
    QPainterPath glint;
    glint.addRoundedRect(QRectF(btn.left() + 3, btn.top() + 1, btn.width() - 6, btn.height() * 0.45),
                         (btn.height() * 0.45) / 2, (btn.height() * 0.45) / 2);
    painter.fillPath(glint, QColor(255, 255, 255, 30));

    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.setPen(hovered ? Qt::white : (count > 0 ? QColor(120, 255, 200) : QColor(210, 240, 255)));
    QString text = count > 0 ? QString("🌐 ROOM: %1 ONLINE [H]").arg(count) : "🌐 HOST ROOM [H]";
    painter.drawText(btn, Qt::AlignCenter, text);
}

void MainWindow::drawMultiplayerLobby(QPainter &painter)
{
    // Semi-transparent backdrop scrim
    painter.fillRect(rect(), QColor(5, 7, 18, 225));

    QRect modalRect = getLobbyModalRect();
    int modalX = modalRect.x();
    int modalY = modalRect.y();
    int modalW = modalRect.width();
    int modalH = modalRect.height();

    // Glowing modal background
    QLinearGradient bgGrad(modalX, modalY, modalX, modalY + modalH);
    bgGrad.setColorAt(0, QColor(20, 26, 52));
    bgGrad.setColorAt(1, QColor(10, 13, 28));
    painter.setBrush(bgGrad);
    painter.setPen(QPen(QColor(0, 229, 255), 2.5));
    painter.drawRoundedRect(modalRect, 20, 20);

    // Title Header
    painter.setPen(QColor(255, 230, 0));
    painter.setFont(QFont("Arial", 20, QFont::Bold));
    painter.drawText(modalX, modalY + 18, modalW, 32, Qt::AlignCenter, "🌐 CLASSROOM MULTIPLAYER ROOM");

    // Close button (X)
    QRect closeBtn = getLobbyCloseButtonRect();
    bool closeHovered = closeBtn.contains(mapFromGlobal(QCursor::pos()));
    painter.setBrush(closeHovered ? QColor(255, 60, 80, 220) : QColor(30, 36, 68));
    painter.setPen(QPen(closeHovered ? Qt::white : QColor(120, 140, 180), 1.5));
    painter.drawEllipse(closeBtn);
    painter.setFont(QFont("Arial", 14, QFont::Bold));
    painter.setPen(closeHovered ? Qt::white : QColor(200, 210, 235));
    painter.drawText(closeBtn, Qt::AlignCenter, "✕");

    // Subtitle
    painter.setPen(QColor(180, 200, 240));
    painter.setFont(QFont("Arial", 12));
    painter.drawText(modalX, modalY + 52, modalW, 20, Qt::AlignCenter,
                     "Connect instantly from your phone or laptop browser — no download needed!");

    // BIG HIGHLIGHTED URL BOX
    QString roomUrl = gameServer ? gameServer->getServerUrl() : "http://127.0.0.1:8080";
    QRect urlBox = getLobbyUrlBoxRect();
    bool urlHovered = urlBox.contains(mapFromGlobal(QCursor::pos()));
    painter.setBrush(urlHovered ? QColor(16, 22, 48) : QColor(12, 16, 36));
    painter.setPen(QPen(urlHovered ? QColor(0, 255, 255) : QColor(0, 255, 200), 1.8));
    painter.drawRoundedRect(urlBox, 12, 12);

    painter.setPen(QColor(0, 255, 220));
    painter.setFont(QFont("Courier New", 17, QFont::Bold));
    painter.drawText(urlBox, Qt::AlignCenter, QString("👉 %1").arg(roomUrl));

    // COPY LINK BUTTON
    QRect copyBtn = getCopyLinkButtonRect();
    bool copyHovered = copyBtn.contains(mapFromGlobal(QCursor::pos()));
    bool isCopiedRecently = linkCopiedFeedback && linkCopiedTimer.elapsed() < 3000;

    QLinearGradient copyGrad(copyBtn.left(), copyBtn.top(), copyBtn.left(), copyBtn.bottom());
    if (isCopiedRecently) {
        copyGrad.setColorAt(0, QColor(0, 230, 120));
        copyGrad.setColorAt(1, QColor(0, 160, 70));
    } else if (copyHovered) {
        copyGrad.setColorAt(0, QColor(0, 220, 255));
        copyGrad.setColorAt(1, QColor(0, 130, 255));
    } else {
        copyGrad.setColorAt(0, QColor(0, 160, 255));
        copyGrad.setColorAt(1, QColor(0, 90, 210));
    }

    painter.setBrush(copyGrad);
    painter.setPen(QPen(isCopiedRecently ? QColor(255, 255, 255) : (copyHovered ? QColor(200, 255, 255) : QColor(0, 220, 255)), 1.5));
    painter.drawRoundedRect(copyBtn, 12, 12);

    painter.setFont(QFont("Arial", 12, QFont::Bold));
    painter.setPen(Qt::white);
    if (isCopiedRecently) {
        painter.drawText(copyBtn, Qt::AlignCenter, "✅ COPIED!");
    } else {
        painter.drawText(copyBtn, Qt::AlignCenter, "📋 COPY LINK");
    }

    // Two Columns: Pac-Heroes vs Ghost Hunters
    int colW = (modalW - 70) / 2;
    int col1X = modalX + 25;
    int col2X = modalX + 45 + colW;
    int cardY = modalY + 138;
    int cardH = 375;

    // Left Column: Pac-Family Co-op (Mode A)
    painter.setBrush(QColor(16, 20, 42));
    painter.setPen(QPen(QColor(255, 105, 180), 1.5));
    painter.drawRoundedRect(col1X, cardY, colW, cardH, 14, 14);

    painter.setPen(QColor(255, 180, 220));
    painter.setFont(QFont("Arial", 13, QFont::Bold));
    painter.drawText(col1X + 15, cardY + 14, colW - 30, 24, Qt::AlignLeft, "🟡 PAC-FAMILY CO-OP (Mode A)");

    painter.setFont(QFont("Arial", 10));
    painter.setPen(QColor(150, 170, 200));
    painter.drawText(col1X + 15, cardY + 36, colW - 30, 18, Qt::AlignLeft, "Team up & share lantern vision in the dark maze!");

    auto drawRoleSlot = [&](const QString &role, const QString &icon, const QString &roleName, QColor roleColor) {
        QRect slotR = getLobbyRoleRect(role);
        bool isHost = (hostRole == role);
        bool isConnectedPlayer = false;
        QString pName;

        if (role == "p1") {
            if (p1ClientId != -1) { isConnectedPlayer = true; pName = p1PlayerName.isEmpty() ? "Classmate" : p1PlayerName; }
        } else if (role == "p2") {
            if (p2ClientId != -1) { isConnectedPlayer = true; pName = p2PlayerName.isEmpty() ? "Classmate" : p2PlayerName; }
        } else if (role == "p3") {
            if (hero3.clientId != -1) { isConnectedPlayer = true; pName = hero3.playerName.isEmpty() ? "Classmate" : hero3.playerName; }
        } else if (role == "p4") {
            if (hero4.clientId != -1) { isConnectedPlayer = true; pName = hero4.playerName.isEmpty() ? "Classmate" : hero4.playerName; }
        } else if (role == "blinky") {
            if (ghosts.size() > 0 && ghosts[0].clientId >= 0) { isConnectedPlayer = true; pName = ghosts[0].playerName; }
        } else if (role == "pinky") {
            if (ghosts.size() > 1 && ghosts[1].clientId >= 0) { isConnectedPlayer = true; pName = ghosts[1].playerName; }
        } else if (role == "inky") {
            if (ghosts.size() > 2 && ghosts[2].clientId >= 0) { isConnectedPlayer = true; pName = ghosts[2].playerName; }
        } else if (role == "clyde") {
            if (ghosts.size() > 3 && ghosts[3].clientId >= 0) { isConnectedPlayer = true; pName = ghosts[3].playerName; }
        }

        bool isHovered = slotR.contains(mapFromGlobal(QCursor::pos()));

        // Background & border
        if (isHost) {
            // Glowing Gold for host
            painter.setBrush(QColor(50, 42, 10, 240));
            painter.setPen(QPen(QColor(255, 220, 0), 2.2));
        } else if (isConnectedPlayer) {
            // Connected classmate
            painter.setBrush(QColor(22, 32, 60, 220));
            painter.setPen(QPen(roleColor, 1.5));
        } else if (isHovered) {
            // Hover over available slot
            painter.setBrush(QColor(28, 38, 70, 230));
            painter.setPen(QPen(QColor(0, 255, 240), 1.8));
        } else {
            // Open available slot
            painter.setBrush(QColor(12, 16, 32, 200));
            painter.setPen(QPen(QColor(50, 65, 95), 1.0));
        }
        painter.drawRoundedRect(slotR, 10, 10);

        // Icon
        painter.setFont(QFont("Arial", 16));
        painter.drawText(slotR.left() + 8, slotR.top() + 6, 38, 44, Qt::AlignCenter, icon);

        // Role title
        painter.setFont(QFont("Arial", 12, QFont::Bold));
        painter.setPen(isHost ? QColor(255, 230, 0) : roleColor);
        painter.drawText(slotR.left() + 50, slotR.top() + 10, slotR.width() - 170, 20, Qt::AlignLeft, roleName);

        // Host badge or selection button
        if (isHost) {
            QRect badgeRect(slotR.right() - 115, slotR.top() + 14, 105, 28);
            painter.setBrush(QColor(255, 200, 0));
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(badgeRect, 6, 6);
            painter.setFont(QFont("Arial", 9, QFont::Bold));
            painter.setPen(QColor(20, 20, 20));
            painter.drawText(badgeRect, Qt::AlignCenter, "★ HOST (YOU)");
        } else if (!isConnectedPlayer) {
            QRect clickRect(slotR.right() - 115, slotR.top() + 14, 105, 28);
            painter.setBrush(isHovered ? QColor(0, 180, 255, 80) : QColor(255, 255, 255, 20));
            painter.setPen(QPen(isHovered ? QColor(0, 255, 240) : QColor(100, 140, 180), 1.0));
            painter.drawRoundedRect(clickRect, 6, 6);
            painter.setFont(QFont("Arial", 8, QFont::Bold));
            painter.setPen(isHovered ? QColor(0, 255, 240) : QColor(160, 180, 210));
            painter.drawText(clickRect, Qt::AlignCenter, "⚡ PLAY THIS");
        }

        // Subtitle / Status
        painter.setFont(QFont("Arial", 9));
        int rLives = getHeroLives(role);
        QString livesBadge;
        if (role.startsWith("p")) {
            if (rLives <= 0) livesBadge = " • 💀 0 Lives (OUT)";
            else {
                QString h;
                for (int i = 0; i < rLives; ++i) h += "❤️";
                livesBadge = QString(" • %1 (%2)").arg(h).arg(rLives);
            }
        }

        if (isHost) {
            painter.setPen(QColor(255, 240, 160));
            QString hostSub = role.startsWith("p")
                ? QString("Host Keyboard%1").arg(livesBadge)
                : QString("Host Keyboard [Ghost Hunter]");
            painter.drawText(slotR.left() + 50, slotR.top() + 32, slotR.width() - 170, 18, Qt::AlignLeft, hostSub);
        } else if (isConnectedPlayer) {
            painter.setPen(QColor(200, 240, 255));
            QString playerSub = role.startsWith("p")
                ? QString("● %1%2").arg(pName, livesBadge)
                : QString("● %1").arg(pName);
            painter.drawText(slotR.left() + 50, slotR.top() + 32, slotR.width() - 170, 18, Qt::AlignLeft, playerSub);
        } else {
            painter.setPen(QColor(110, 130, 160));
            if (role.startsWith("p")) {
                painter.drawText(slotR.left() + 50, slotR.top() + 32, slotR.width() - 170, 18, Qt::AlignLeft,
                                 "○ Open (Click to play or invite classmate)");
            } else {
                painter.drawText(slotR.left() + 50, slotR.top() + 32, slotR.width() - 170, 18, Qt::AlignLeft,
                                 "🤖 AI Ghost (Click to play as this ghost)");
            }
        }
    };

    drawRoleSlot("p1", "🟡", "Pac-Man (Yellow)", QColor(255, 230, 0));
    drawRoleSlot("p2", "🎀", "Ms. Pac-Man (Pink)", QColor(255, 105, 180));
    drawRoleSlot("p3", "🧢", "Jr. Pac-Man (Cyan)", QColor(0, 229, 255));
    drawRoleSlot("p4", "🟢", "Miracle Pac-Man", QColor(0, 255, 100));

    // Right Column: Playable Ghosts (Mode B)
    painter.setBrush(QColor(16, 20, 42));
    painter.setPen(QPen(QColor(255, 70, 70), 1.5));
    painter.drawRoundedRect(col2X, cardY, colW, cardH, 14, 14);

    painter.setPen(QColor(255, 120, 120));
    painter.setFont(QFont("Arial", 13, QFont::Bold));
    painter.drawText(col2X + 15, cardY + 14, colW - 30, 24, Qt::AlignLeft, "👻 GHOST HUNTERS (Mode B)");

    painter.setFont(QFont("Arial", 10));
    painter.setPen(QColor(150, 170, 200));
    painter.drawText(col2X + 15, cardY + 36, colW - 30, 18, Qt::AlignLeft, "Control ghosts to hunt Pac-Man!");

    drawRoleSlot("blinky", "🔴", "Blinky (Red Ghost)", QColor(255, 70, 70));
    drawRoleSlot("pinky",  "🌸", "Pinky (Pink Ghost)", QColor(255, 150, 200));
    drawRoleSlot("inky",   "🩵", "Inky (Cyan Ghost)", QColor(0, 220, 255));
    drawRoleSlot("clyde",  "🟠", "Clyde (Orange Ghost)", QColor(255, 160, 50));

    // Footer banner
    painter.setPen(QColor(140, 170, 220));
    painter.setFont(QFont("Arial", 11, QFont::Bold));
    painter.drawText(modalX, modalY + modalH - 26, modalW, 20, Qt::AlignCenter,
                     "Click any character above to play as them  •  [Tab] Switch Character  •  [C] Copy Link  •  [H]/[Esc] Close");
}
