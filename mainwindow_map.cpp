#include "mainwindow.h"
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QCursor>
#include <cmath>
#include <algorithm>

QString MainWindow::getLevelTitle(int lvl) const
{
    int cols, rows;
    getSectorDimsForLevel(lvl, cols, rows);
    int windows = cols * rows;
    int world = (lvl - 1) / LEVELS_PER_WORLD + 1;
    int stage = (lvl - 1) % LEVELS_PER_WORLD + 1;
    return QString("World %1 • Stage %2 of 20 (Level %3: %4 Windows)").arg(world).arg(stage).arg(lvl).arg(windows);
}

QRect MainWindow::getPlayButtonRect() const
{
    int btnW = std::min(320, int(width() * 0.45));
    int btnH = std::max(42, int(height() * 0.054));
    if (btnH > 52) btnH = 52;
    int btnX = (width() - btnW) / 2;
    int btnY = height() - btnH - 12;
    return QRect(btnX, btnY, btnW, btnH);
}

QRect MainWindow::getMyLevelButtonRect() const
{
    int btnW = std::clamp(int(width() * 0.16), 125, 160);
    return QRect(18, 22, btnW, 36);
}

QRect MainWindow::getRecordBadgeRect() const
{
    int btnW = std::clamp(int(width() * 0.16), 125, 160);
    return QRect(width() - btnW - 18, 22, btnW, 36);
}

QRect MainWindow::getModeSelectorRect() const
{
    int selW = std::min(460, int(width() * 0.62));
    int selH = std::max(36, int(height() * 0.046));
    if (selH > 42) selH = 42;
    int selX = (width() - selW) / 2;
    int selY = getPlayButtonRect().top() - selH - 8;
    return QRect(selX, selY, selW, selH);
}

QRect MainWindow::getModeYellowRect() const
{
    QRect total = getModeSelectorRect();
    int w3 = total.width() / 3;
    return QRect(total.left(), total.top(), w3, total.height());
}

QRect MainWindow::getModePinkRect() const
{
    QRect total = getModeSelectorRect();
    int w3 = total.width() / 3;
    return QRect(total.left() + w3, total.top(), w3, total.height());
}

QRect MainWindow::getModeCoopRect() const
{
    QRect total = getModeSelectorRect();
    int w3 = total.width() / 3;
    return QRect(total.left() + 2 * w3, total.top(), total.width() - 2 * w3, total.height());
}

QRect MainWindow::getMapButtonRect() const
{
    return QRect(16, 14, 115, 32);
}

float MainWindow::getNodeSpacingY() const
{
    return 82.0f;
}

int MainWindow::getTotalMappedLevels() const
{
    int curWorld = (highestUnlockedLevel - 1) / LEVELS_PER_WORLD + 1;
    int totalWorlds = std::max(5, curWorld + 2); // Show at least 5 worlds (100 levels) or 2 worlds beyond progress
    return totalWorlds * LEVELS_PER_WORLD;
}

int MainWindow::getCurrentVisibleWorld() const
{
    int virtualH = getVirtualMapHeight();
    float centerVirtualY = virtualH - (mapScrollY + height() * 0.5f);
    int totalLvls = getTotalMappedLevels();
    for (int lvl = 1; lvl <= totalLvls; ++lvl) {
        int wIdx = (lvl - 1) / LEVELS_PER_WORLD;
        float dist = 260.0f + (lvl - 1) * 82.0f + wIdx * 140.0f;
        if (dist >= centerVirtualY) {
            return (lvl - 1) / LEVELS_PER_WORLD + 1;
        }
    }
    return totalLvls / LEVELS_PER_WORLD;
}

int MainWindow::getVirtualMapHeight() const
{
    int totalLvls = getTotalMappedLevels();
    int totalWorlds = totalLvls / LEVELS_PER_WORLD;
    return int(260.0f + (totalLvls - 1) * 82.0f + (totalWorlds - 1) * 140.0f + 360.0f);
}

void MainWindow::scrollToLevel(int lvl, bool immediate)
{
    int totalLvls = getTotalMappedLevels();
    lvl = std::clamp(lvl, 1, totalLvls);

    int virtualH = getVirtualMapHeight();
    int worldIdx = (lvl - 1) / LEVELS_PER_WORLD;
    float distFromBottom = 260.0f + (lvl - 1) * 82.0f + worldIdx * 140.0f;
    int nodeVirtualY = virtualH - int(distFromBottom);

    int maxScroll = std::max(0, virtualH - height());
    float desired = std::clamp(float(nodeVirtualY - height() / 2), 0.0f, float(maxScroll));
    targetMapScrollY = desired;
    if (immediate) {
        mapScrollY = desired;
    }
}

QPoint MainWindow::getLevelNodePos(int lvl, int mapW, int mapH) const
{
    Q_UNUSED(mapH);
    int worldIdx = (lvl - 1) / LEVELS_PER_WORLD;
    float distFromBottom = 260.0f + (lvl - 1) * 82.0f + worldIdx * 140.0f;

    // Continuous serpentine sinusoidal wave (never resets at world boundaries!)
    float wave = std::sin((lvl - 1) * 0.58f);
    float xNorm = 0.50f + 0.32f * wave;
    int nodeX = int(mapW * std::clamp(xNorm, 0.16f, 0.84f));

    int virtualH = getVirtualMapHeight();
    int nodeVirtualY = virtualH - int(distFromBottom);
    int nodeScreenY = nodeVirtualY - int(mapScrollY);

    return QPoint(nodeX, nodeScreenY);
}

void MainWindow::drawPacmanAvatar(QPainter &painter, int cx, int cy, int size)
{
    painter.save();
    int r = size / 2;

    // Golden glow behind Pacman
    float pulse = (std::sin(elapsedTimer.elapsed() * 0.009f) + 1.0f) * 0.5f;
    QRadialGradient glow(cx, cy, r * 1.8);
    glow.setColorAt(0.0, QColor(255, 230, 80, 130 + int(70 * pulse)));
    glow.setColorAt(0.6, QColor(255, 200, 30, 50));
    glow.setColorAt(1.0, QColor(0, 0, 0, 0));
    painter.fillRect(cx - r * 2, cy - r * 2, r * 4, r * 4, glow);

    // Pacman body with animated mouth
    int startAngle = int(35 * std::abs(std::sin(elapsedTimer.elapsed() * 0.008f)));
    int spanAngle = 360 - 2 * startAngle;

    painter.setBrush(QColor(255, 225, 20));
    painter.setPen(QPen(QColor(220, 160, 0), 2));
    painter.drawPie(cx - r, cy - r, size, size, startAngle * 16, spanAngle * 16);

    // Eye
    painter.setBrush(QColor(30, 30, 30));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(cx + r / 5, cy - r / 2, std::max(3, r / 4), std::max(4, r / 3));

    // Specular shine
    painter.setBrush(QColor(255, 255, 255, 180));
    painter.drawEllipse(cx - r / 3, cy - r / 2, std::max(3, r / 3), std::max(2, r / 4));

    // Little banner badge above Pacman ("P1" in multiplayer, "YOU" in solo)
    painter.setFont(QFont("Arial", std::max(7, size / 4), QFont::Bold));
    painter.setBrush(QColor(255, 50, 90));
    painter.setPen(QPen(QColor(255, 220, 220), 1));
    int bW = std::max(36, size + 12);
    int bH = 16;
    painter.drawRoundedRect(cx - bW / 2, cy - r - bH - 3, bW, bH, 4, 4);

    painter.setPen(Qt::white);
    painter.drawText(cx - bW / 2, cy - r - bH - 3, bW, bH, Qt::AlignCenter, isMultiplayer ? "P1" : "YOU");

    painter.restore();
}

void MainWindow::drawMsPacmanAvatar(QPainter &painter, int cx, int cy, int size)
{
    painter.save();
    int r = size / 2;

    // Pink glow behind Ms. Pacman
    float pulse = (std::sin(elapsedTimer.elapsed() * 0.009f + 1.6f) + 1.0f) * 0.5f;
    QRadialGradient glow(cx, cy, r * 1.8);
    glow.setColorAt(0.0, QColor(255, 100, 180, 130 + int(70 * pulse)));
    glow.setColorAt(0.6, QColor(255, 50, 140, 50));
    glow.setColorAt(1.0, QColor(0, 0, 0, 0));
    painter.fillRect(cx - r * 2, cy - r * 2, r * 4, r * 4, glow);

    // Coral Pink body with animated mouth
    int startAngle = int(35 * std::abs(std::sin(elapsedTimer.elapsed() * 0.008f + 1.2f)));
    int spanAngle = 360 - 2 * startAngle;

    painter.setBrush(QColor(255, 105, 180));
    painter.setPen(QPen(QColor(220, 50, 120), 2));
    painter.drawPie(cx - r, cy - r, size, size, (180 + startAngle) * 16, spanAngle * 16);

    // Eye
    painter.setBrush(QColor(20, 20, 50));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(cx - r / 5, cy - r / 2, std::max(3, r / 4), std::max(4, r / 3));

    // Specular shine
    painter.setBrush(QColor(255, 255, 255, 180));
    painter.drawEllipse(cx - r / 3, cy - r / 2, std::max(2, r / 4), std::max(2, r / 4));

    // Iconic Red Ribbon Bow on head
    int bowSize = std::max(6, r * 3 / 5);
    int bx = cx + r / 4;
    int by = cy - r;
    painter.setBrush(QColor(235, 25, 60));
    painter.setPen(QPen(QColor(180, 10, 40), 1));
    painter.drawEllipse(bx - bowSize, by, bowSize * 3 / 4, bowSize / 2);
    painter.drawEllipse(bx, by, bowSize * 3 / 4, bowSize / 2);
    painter.setBrush(QColor(255, 210, 220));
    painter.drawEllipse(bx - bowSize / 5, by + bowSize / 8, bowSize * 2 / 5, bowSize * 2 / 5);

    // Little "P2" banner badge above Ms. Pacman
    painter.setFont(QFont("Arial", std::max(7, size / 4), QFont::Bold));
    painter.setBrush(QColor(230, 40, 120));
    painter.setPen(QPen(QColor(255, 220, 240), 1));
    int bW = std::max(32, size);
    int bH = 16;
    painter.drawRoundedRect(cx - bW / 2, cy - r - bH - 3, bW, bH, 4, 4);

    painter.setPen(Qt::white);
    painter.drawText(cx - bW / 2, cy - r - bH - 3, bW, bH, Qt::AlignCenter, "P2");

    painter.restore();
}

void MainWindow::drawLevelNode(QPainter &painter, int lvl, int cx, int cy, int radius)
{
    painter.save();
    bool isCompleted = (lvl < highestUnlockedLevel);
    bool isCurrent = (lvl == highestUnlockedLevel);
    bool isSelected = (lvl == selectedMapLevel);
    bool isLocked = (lvl > highestUnlockedLevel);
    bool isHovered = (lvl == hoveredMapLevel);

    int r = radius;
    if (isHovered && !isLocked) r += 3;

    // 1. Aura / Pulsing ring for current/selected node
    if (isCurrent || isSelected) {
        float pulse = (std::sin(elapsedTimer.elapsed() * 0.007f) + 1.0f) * 0.5f;
        int ringR = r + 8 + int(8 * pulse);
        QRadialGradient ringGrad(cx, cy, ringR);
        ringGrad.setColorAt(0.0, QColor(0, 230, 255, 120));
        ringGrad.setColorAt(0.7, QColor(0, 180, 255, 60));
        ringGrad.setColorAt(1.0, QColor(0, 0, 0, 0));
        painter.fillRect(cx - ringR, cy - ringR, ringR * 2, ringR * 2, ringGrad);

        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(0, 240, 255, 160 + int(95 * pulse)), 3, Qt::DashLine));
        painter.drawEllipse(cx - ringR + 3, cy - ringR + 3, (ringR - 3) * 2, (ringR - 3) * 2);
    }

    // 2. Drop shadow
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 120));
    painter.drawEllipse(cx - r + 3, cy - r + 5, r * 2, r * 2);

    // 3. 3D Spherical Candy Button
    QRadialGradient grad(cx - r / 3, cy - r / 3, r * 1.3);
    if (isCompleted) {
        // Candy Emerald
        grad.setColorAt(0.0, QColor(110, 255, 150));
        grad.setColorAt(0.4, QColor(35, 200, 75));
        grad.setColorAt(0.85, QColor(15, 130, 45));
        grad.setColorAt(1.0, QColor(8, 70, 25));
        painter.setBrush(grad);
        painter.setPen(QPen(QColor(180, 255, 200), 2));
    } else if (isCurrent || isSelected) {
        // Candy Cyan / Royal Blue
        grad.setColorAt(0.0, QColor(140, 240, 255));
        grad.setColorAt(0.4, QColor(0, 160, 245));
        grad.setColorAt(0.85, QColor(0, 90, 190));
        grad.setColorAt(1.0, QColor(0, 40, 110));
        painter.setBrush(grad);
        painter.setPen(QPen(QColor(200, 245, 255), 2.5));
    } else {
        // Frosted Slate / Locked
        grad.setColorAt(0.0, QColor(85, 75, 105));
        grad.setColorAt(0.6, QColor(48, 40, 65));
        grad.setColorAt(1.0, QColor(25, 20, 36));
        painter.setBrush(grad);
        painter.setPen(QPen(QColor(70, 60, 85), 1.5));
    }
    painter.drawEllipse(cx - r, cy - r, r * 2, r * 2);

    // 4. Glossy Specular Highlight on top
    painter.setBrush(QColor(255, 255, 255, 75));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(cx - r * 0.5, cy - r * 0.75, int(r * 1.0), int(r * 0.55));

    // 5. Node Contents: Number or Padlock
    if (isLocked) {
        int lW = r * 0.6;
        int lH = r * 0.5;
        int lx = cx - lW / 2;
        int ly = cy - lH / 4;

        // Shackle
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(200, 190, 160), 2.5));
        painter.drawArc(lx + lW / 4, ly - lH / 2, lW / 2, lH, 0, 180 * 16);

        // Body
        painter.setBrush(QColor(180, 150, 60));
        painter.setPen(QPen(QColor(230, 200, 100), 1));
        painter.drawRoundedRect(lx, ly, lW, lH, 3, 3);
    } else {
        // Bold Level Number
        painter.setFont(QFont("Arial", std::max(12, r * 3 / 4), QFont::Bold));
        painter.setPen(QColor(0, 0, 0, 120));
        painter.drawText(cx - r + 1, cy - r + 2, r * 2, r * 2, Qt::AlignCenter, QString::number(lvl));
        painter.setPen(Qt::white);
        painter.drawText(cx - r, cy - r, r * 2, r * 2, Qt::AlignCenter, QString::number(lvl));

        // Golden Stars for completed levels
        if (isCompleted) {
            painter.setFont(QFont("Arial", std::max(8, r / 3)));
            painter.setPen(QColor(255, 215, 0));
            painter.drawText(cx - r, cy + r - 3, r * 2, 16, Qt::AlignCenter, "⭐⭐⭐");
        }
    }

    // 6. Milestone Jewel Rings on Stages 5, 10, 15
    int stageInWorld = (lvl - 1) % LEVELS_PER_WORLD + 1;
    if (stageInWorld % 5 == 0 && stageInWorld != LEVELS_PER_WORLD) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(255, 215, 0, 190), 2.5, Qt::SolidLine));
        painter.drawEllipse(cx - r - 4, cy - r - 4, (r + 4) * 2, (r + 4) * 2);
    }

    // 7. Stage 20 Crown Trophy (Final Summit of each World!)
    if (lvl % LEVELS_PER_WORLD == 0) {
        int cw = 28, ch = 18;
        int cx0 = cx - cw / 2;
        int cy0 = cy - r - ch - 4;
        painter.setBrush(QColor(255, 210, 30));
        painter.setPen(QPen(QColor(255, 240, 120), 1));
        QPolygon crown;
        crown << QPoint(cx0, cy0 + ch)
              << QPoint(cx0 + cw, cy0 + ch)
              << QPoint(cx0 + cw, cy0 + ch / 3)
              << QPoint(cx0 + cw * 3 / 4, cy0 + ch * 2 / 3)
              << QPoint(cx0 + cw / 2, cy0)
              << QPoint(cx0 + cw / 4, cy0 + ch * 2 / 3)
              << QPoint(cx0, cy0 + ch / 3);
        painter.drawPolygon(crown);
    }

    // 8. Pacman Avatar on the Selected Level Node
    if (lvl == selectedMapLevel) {
        int bob = int(4 * std::sin(elapsedTimer.elapsed() * 0.007f));
        if (isMultiplayer) {
            drawPacmanAvatar(painter, cx - 18, cy - r - 22 + bob, std::max(20, r - 3));
            drawMsPacmanAvatar(painter, cx + 18, cy - r - 22 - bob, std::max(20, r - 3));
        } else if (singlePlayerHero == 2) {
            drawMsPacmanAvatar(painter, cx, cy - r - 22 + bob, std::max(22, r));
        } else {
            drawPacmanAvatar(painter, cx, cy - r - 22 + bob, std::max(22, r));
        }
    }

    painter.restore();
}

MainWindow::WorldPalette MainWindow::getWorldPalette(int world) const
{
    static const WorldPalette palettes[] = {
        // World 1: Sweet Peppermint Plains (Plum / Raspberry Twilight)
        { QColor(20, 10, 42),  QColor(48, 16, 70),  QColor(90, 24, 85),   QColor(36, 12, 50) },
        // World 2: Caramel Lava Caverns (Warm Molten Amber / Dark Caramel)
        { QColor(35, 12, 10),  QColor(75, 28, 15),  QColor(130, 55, 18),  QColor(50, 18, 12) },
        // World 3: Gummy Bear Galaxy (Cosmic Indigo & Royal Violet Nebula)
        { QColor(10, 8, 38),   QColor(22, 15, 68),  QColor(50, 25, 110),  QColor(18, 10, 52) },
        // World 4: Cotton Candy Clouds (Pastel Strawberry Frost & Lavender)
        { QColor(38, 12, 45),  QColor(80, 25, 75),  QColor(125, 45, 105), QColor(48, 15, 55) },
        // World 5: Chocolate Chip Chasm (Dark Cocoa & Golden Hazelnut Bronze)
        { QColor(25, 14, 10),  QColor(55, 28, 18),  QColor(95, 48, 25),   QColor(38, 18, 12) },
        // World 6: Electric Licorice & Emerald Lagoon
        { QColor(8, 22, 28),   QColor(12, 52, 58),  QColor(18, 88, 75),   QColor(10, 36, 40) }
    };
    int count = sizeof(palettes) / sizeof(palettes[0]);
    int idx = (world - 1) % count;
    if (idx < 0) idx = 0;
    return palettes[idx];
}

void MainWindow::drawWorldGateway(QPainter &painter, int boundaryLvl, int w, int h)
{
    Q_UNUSED(h);
    int nextWorld = boundaryLvl / LEVELS_PER_WORLD + 1;
    QPoint p0 = getLevelNodePos(boundaryLvl, w, h);
    QPoint p1 = getLevelNodePos(boundaryLvl + 1, w, h);

    int gateY = (p0.y() + p1.y()) / 2;
    if (gateY < -90 || gateY > height() + 90) return; // Screen culled!

    int gateW = std::clamp(int(w * 0.72), 340, 500);
    int gateH = 54;
    int gateX = (w - gateW) / 2;

    bool isUnlocked = (highestUnlockedLevel > boundaryLvl);

    painter.save();

    // 1. Shimmering Barrier across road if locked
    if (!isUnlocked) {
        QLinearGradient barrierGrad(gateX, gateY - 24, gateX + gateW, gateY + 24);
        barrierGrad.setColorAt(0.0, QColor(140, 40, 200, 0));
        barrierGrad.setColorAt(0.5, QColor(220, 60, 240, 90));
        barrierGrad.setColorAt(1.0, QColor(140, 40, 200, 0));
        painter.fillRect(gateX - 20, gateY - 14, gateW + 40, 28, barrierGrad);
    }

    // 2. Arch Ribbon Drop Shadow
    painter.setBrush(QColor(0, 0, 0, 140));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(gateX + 2, gateY - gateH / 2 + 4, gateW, gateH, 14, 14);

    // 3. Arch Body Gradient
    QLinearGradient gGrad(gateX, gateY - gateH / 2, gateX, gateY + gateH / 2);
    if (isUnlocked) {
        gGrad.setColorAt(0.0, QColor(45, 190, 125));
        gGrad.setColorAt(0.5, QColor(20, 145, 88));
        gGrad.setColorAt(1.0, QColor(10, 95, 52));
    } else {
        gGrad.setColorAt(0.0, QColor(75, 35, 100));
        gGrad.setColorAt(0.5, QColor(48, 20, 68));
        gGrad.setColorAt(1.0, QColor(30, 12, 45));
    }
    painter.setBrush(gGrad);
    painter.setPen(QPen(isUnlocked ? QColor(160, 255, 200) : QColor(220, 170, 255), 2.5));
    painter.drawRoundedRect(gateX, gateY - gateH / 2, gateW, gateH, 14, 14);

    // 4. Arch Title & Subtitle Texts
    int startLvl = boundaryLvl + 1;
    int endLvl = boundaryLvl + LEVELS_PER_WORLD;
    QString worldTitle = getWorldName(nextWorld);

    painter.setFont(QFont("Arial", std::max(11, gateH * 3 / 8), QFont::Bold));
    painter.setPen(isUnlocked ? QColor(240, 255, 240) : QColor(255, 225, 140));
    painter.drawText(gateX, gateY - gateH / 2 + 4, gateW, gateH * 5 / 8, Qt::AlignCenter,
                     QString("✦ %1 ✦").arg(worldTitle));

    painter.setFont(QFont("Arial", std::max(8, gateH / 5), QFont::Bold));
    if (isUnlocked) {
        painter.setPen(QColor(180, 255, 210));
        painter.drawText(gateX, gateY + 4, gateW, gateH * 3 / 8, Qt::AlignCenter,
                         QString("✓ WORLD UNLOCKED  •  Stages 1-20 (Levels %1 - %2)").arg(startLvl).arg(endLvl));
    } else {
        painter.setPen(QColor(255, 175, 195));
        painter.drawText(gateX, gateY + 4, gateW, gateH * 3 / 8, Qt::AlignCenter,
                         QString("🔒 Clear Stage %1 to Unlock!  (Levels %2 - %3)").arg(boundaryLvl).arg(startLvl).arg(endLvl));
    }

    // 5. Side Candy Pillars
    int pillarW = 12;
    int pillarH = gateH + 18;
    QRect pL(gateX - pillarW / 2, gateY - pillarH / 2, pillarW, pillarH);
    QRect pR(gateX + gateW - pillarW / 2, gateY - pillarH / 2, pillarW, pillarH);
    painter.setBrush(isUnlocked ? QColor(255, 215, 80) : QColor(190, 150, 230));
    painter.setPen(QPen(Qt::white, 1));
    painter.drawRoundedRect(pL, 4, 4);
    painter.drawRoundedRect(pR, 4, 4);

    painter.restore();
}

void MainWindow::drawCandyCrushMap(QPainter &painter)
{
    int w = width();
    int h = height();

    int curW = getCurrentVisibleWorld();
    viewedMapWorld = curW;
    int totalLvls = getTotalMappedLevels();

    // 1. Rich Dynamic Atmospheric Gradient Background (Smoothly blends as you scroll across Worlds!)
    int virtualH = getVirtualMapHeight();
    float centerVirtualY = virtualH - (mapScrollY + h * 0.5f);
    float levelCenter = 1.0f;
    for (int lvl = 1; lvl <= totalLvls; ++lvl) {
        int wIdx = (lvl - 1) / LEVELS_PER_WORLD;
        float dist = 260.0f + (lvl - 1) * 82.0f + wIdx * 140.0f;
        if (dist >= centerVirtualY) {
            levelCenter = lvl;
            break;
        }
    }
    int curWorld = (int(levelCenter) - 1) / LEVELS_PER_WORLD + 1;
    int nextWorld = curWorld + 1;
    float tWorld = float((int(levelCenter) - 1) % LEVELS_PER_WORLD) / float(LEVELS_PER_WORLD);
    tWorld = std::clamp(tWorld, 0.0f, 1.0f);

    WorldPalette pal1 = getWorldPalette(curWorld);
    WorldPalette pal2 = getWorldPalette(nextWorld);

    auto blend = [](const QColor &c1, const QColor &c2, float t) {
        int r = int(c1.red() * (1.0f - t) + c2.red() * t);
        int g = int(c1.green() * (1.0f - t) + c2.green() * t);
        int b = int(c1.blue() * (1.0f - t) + c2.blue() * t);
        return QColor(r, g, b);
    };

    QLinearGradient bg(0, 0, 0, h);
    bg.setColorAt(0.0, blend(pal1.top, pal2.top, tWorld));
    bg.setColorAt(0.35, blend(pal1.mid1, pal2.mid1, tWorld));
    bg.setColorAt(0.70, blend(pal1.mid2, pal2.mid2, tWorld));
    bg.setColorAt(1.0, blend(pal1.bot, pal2.bot, tWorld));
    painter.fillRect(rect(), bg);

    // Gentle candy hills near the base of the mountain (World 1)
    float distFromBottom = (virtualH - h) - mapScrollY;
    if (distFromBottom < h * 1.4f) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(60, 16, 68, 140));
        QPainterPath hill1;
        hill1.moveTo(0, h * 0.86);
        hill1.quadTo(w * 0.35, h * 0.74, w * 0.7, h * 0.88);
        hill1.quadTo(w * 0.88, h * 0.94, w, h * 0.86);
        hill1.lineTo(w, h);
        hill1.lineTo(0, h);
        painter.drawPath(hill1);

        painter.setBrush(QColor(42, 12, 54, 180));
        QPainterPath hill2;
        hill2.moveTo(0, h * 0.91);
        hill2.quadTo(w * 0.45, h * 0.83, w, h * 0.93);
        hill2.lineTo(w, h);
        hill2.lineTo(0, h);
        painter.drawPath(hill2);
    }

    // Twinkling stars / sugar sprinkles
    painter.setPen(Qt::NoPen);
    static const int starCoords[14][2] = {
        {12, 18}, {28, 10}, {84, 14}, {92, 26}, {15, 45}, {88, 52},
        {8, 70}, {94, 68}, {45, 12}, {62, 38}, {22, 60}, {76, 80},
        {50, 70}, {38, 90}
    };
    for (int i = 0; i < 14; ++i) {
        int sx = int(starCoords[i][0] * 0.01 * w);
        int sy = int(starCoords[i][1] * 0.01 * h);
        float pulse = (std::sin(elapsedTimer.elapsed() * 0.005f + i * 1.3f) + 1.0f) * 0.5f;
        int starA = 80 + int(140 * pulse);
        painter.setBrush(QColor(255, 240, 160, starA));
        int starD = 3 + (i % 3);
        painter.drawEllipse(sx, sy, starD, starD);
    }

    // 2. The Continuous Serpentine Candy Road connecting ALL levels in snake fashion!
    QPainterPath roadPath;
    roadPath.moveTo(getLevelNodePos(1, w, h));
    for (int lvl = 2; lvl <= totalLvls; ++lvl) {
        QPoint p0 = getLevelNodePos(lvl - 1, w, h);
        QPoint p1 = getLevelNodePos(lvl, w, h);
        QPoint cp1(p0.x(), (p0.y() + p1.y()) / 2);
        QPoint cp2(p1.x(), (p0.y() + p1.y()) / 2);
        roadPath.cubicTo(cp1, cp2, p1);
    }

    // Layer 1: Road Glow Aura
    painter.strokePath(roadPath, QPen(QColor(255, 110, 190, 45), 32, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    // Layer 2: Road Bed (Chocolate / Caramel)
    painter.strokePath(roadPath, QPen(QColor(135, 60, 25, 230), 20, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    // Layer 3: Dashed Yellow Sugar Road Line
    painter.strokePath(roadPath, QPen(QColor(255, 230, 110, 240), 5, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));

    // Decorative Sugar Power Dots along the road (culled to screen)
    for (int lvl = 1; lvl < totalLvls; ++lvl) {
        QPoint p0 = getLevelNodePos(lvl, w, h);
        QPoint p1 = getLevelNodePos(lvl + 1, w, h);
        if (std::max(p0.y(), p1.y()) < -30 || std::min(p0.y(), p1.y()) > h + 30) continue;

        for (int step = 1; step <= 3; ++step) {
            double t = step / 4.0;
            double it = 1.0 - t;
            double cy0 = (p0.y() + p1.y()) / 2.0;
            double bx = it * it * it * p0.x() + 3 * it * it * t * p0.x() + 3 * it * t * t * p1.x() + t * t * t * p1.x();
            double by = it * it * it * p0.y() + 3 * it * it * t * cy0 + 3 * it * t * t * cy0 + t * t * t * p1.y();

            painter.setBrush(QColor(255, 230, 160));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(int(bx) - 3, int(by) - 3, 6, 6);
        }
    }

    // 3. Draw World Gateways between worlds (at levels 20, 40, 60, 80...)
    int totalWorlds = totalLvls / LEVELS_PER_WORLD;
    for (int wIdx = 1; wIdx < totalWorlds; ++wIdx) {
        drawWorldGateway(painter, wIdx * LEVELS_PER_WORLD, w, h);
    }

    // 4. Level Nodes along the continuous snake road (culled to screen)
    int radius = std::clamp(std::min(w, h) / 24, 22, 32);
    for (int lvl = 1; lvl <= totalLvls; ++lvl) {
        QPoint np = getLevelNodePos(lvl, w, h);
        if (np.y() < -60 || np.y() > h + 60) continue;
        drawLevelNode(painter, lvl, np.x(), np.y(), radius);
    }

    // 5. Vertical Mountain Elevation Scrollbar Indicator (Right margin)
    int maxScroll = std::max(1, getVirtualMapHeight() - h);
    int sbW = 6;
    int trackH = h - 160;
    int sbH = std::clamp(int(trackH * (float(h) / getVirtualMapHeight())), 24, trackH / 3);
    int sbX = w - sbW - 6;
    float scrollRatio = std::clamp(mapScrollY / float(maxScroll), 0.0f, 1.0f);
    int sbY = int(scrollRatio * (trackH - sbH)) + 80;

    painter.setBrush(QColor(255, 255, 255, 25));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(sbX, 80, sbW, trackH, 3, 3);

    painter.setBrush(QColor(255, 215, 100, 190));
    painter.drawRoundedRect(sbX, sbY, sbW, sbH, 3, 3);

    // Frosted Top Dock to keep Header & Quick Jump buttons crisp
    int hy = 16;
    int headerH = 64;
    QLinearGradient topGrad(0, 0, 0, hy + headerH + 36);
    topGrad.setColorAt(0.0, QColor(10, 5, 20, 235));
    topGrad.setColorAt(0.75, QColor(10, 5, 20, 180));
    topGrad.setColorAt(1.0, QColor(10, 5, 20, 0));
    painter.fillRect(0, 0, w, hy + headerH + 36, topGrad);

    // Frosted Bottom Dock for Mode Selector & Play Button
    QLinearGradient dockGrad(0, getModeSelectorRect().top() - 14, 0, h);
    dockGrad.setColorAt(0.0, QColor(10, 5, 20, 0));
    dockGrad.setColorAt(0.35, QColor(10, 5, 20, 215));
    dockGrad.setColorAt(1.0, QColor(10, 5, 20, 250));
    painter.fillRect(0, getModeSelectorRect().top() - 14, w, h - getModeSelectorRect().top() + 14, dockGrad);

    // 6. Quick Navigation Controls: [ 📍 MY HERO (L%1) ] on Left, [ 🏆 TOP: LVL %1 ] on Right
    QRect myBtn = getMyLevelButtonRect();
    QRect recBtn = getRecordBadgeRect();
    QPoint mousePos = mapFromGlobal(QCursor::pos());
    bool myHover = myBtn.contains(mousePos);
    bool recHover = recBtn.contains(mousePos);

    // My Hero Button
    painter.setBrush(myHover ? QColor(40, 120, 90, 230) : QColor(20, 55, 45, 210));
    painter.setPen(QPen(myHover ? QColor(100, 255, 180) : QColor(60, 200, 140), 1.5));
    painter.drawRoundedRect(myBtn, 18, 18);

    painter.setFont(QFont("Arial", std::max(9, myBtn.height() * 3 / 8), QFont::Bold));
    painter.setPen(myHover ? Qt::white : QColor(210, 255, 230));
    painter.drawText(myBtn, Qt::AlignCenter, QString("📍 MY HERO (L%1)").arg(highestUnlockedLevel));

    // Record Badge
    painter.setBrush(recHover ? QColor(65, 45, 15, 230) : QColor(38, 25, 10, 210));
    painter.setPen(QPen(recHover ? QColor(255, 220, 100) : QColor(210, 160, 60), 1.5));
    painter.drawRoundedRect(recBtn, 18, 18);

    painter.setFont(QFont("Arial", std::max(9, recBtn.height() * 3 / 8), QFont::Bold));
    painter.setPen(recHover ? Qt::white : QColor(255, 235, 160));
    painter.drawText(recBtn, Qt::AlignCenter, QString("🏆 TOP: LVL %1").arg(highestUnlockedLevel));

    // 7. Header Ribbon Banner
    int headerW = std::clamp(int(w - myBtn.width() * 2 - 48), 340, 560);
    int hx = (w - headerW) / 2;

    // Drop shadow
    painter.setBrush(QColor(0, 0, 0, 140));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(hx + 3, hy + 5, headerW, headerH, 14, 14);

    // Banner gradient
    QLinearGradient bGrad(hx, hy, hx, hy + headerH);
    bGrad.setColorAt(0.0, QColor(255, 75, 120));
    bGrad.setColorAt(0.5, QColor(220, 30, 80));
    bGrad.setColorAt(1.0, QColor(160, 15, 55));
    painter.setBrush(bGrad);
    painter.setPen(QPen(QColor(255, 215, 100), 3));
    painter.drawRoundedRect(hx, hy, headerW, headerH, 14, 14);

    // Title text
    painter.setFont(QFont("Arial", std::max(14, headerH * 3 / 8), QFont::Bold));
    painter.setPen(QColor(60, 10, 20));
    painter.drawText(hx + 1, hy + 2, headerW, headerH * 5 / 8, Qt::AlignCenter, "PAC-MAN LABYRINTH SAGA");
    painter.setPen(QColor(255, 235, 120));
    painter.drawText(hx, hy, headerW, headerH * 5 / 8, Qt::AlignCenter, "PAC-MAN LABYRINTH SAGA");

    int wStart = (curW - 1) * LEVELS_PER_WORLD + 1;
    int wEnd = wStart + LEVELS_PER_WORLD - 1;
    painter.setFont(QFont("Arial", std::max(8, headerH / 6), QFont::Bold));
    painter.setPen(QColor(255, 220, 230));
    painter.drawText(hx, hy + headerH * 4 / 8, headerW, headerH * 3 / 8, Qt::AlignCenter,
                     QString("%1  •  Stages 1-20 (Levels %2-%3)").arg(getWorldName(curW)).arg(wStart).arg(wEnd));

    // Highest Unlocked Badge centered directly under ribbon banner
    int pbW = 210, pbH = 26;
    int pbx = (w - pbW) / 2;
    int pby = hy + headerH + 6;
    painter.setBrush(QColor(15, 20, 45, 220));
    painter.setPen(QPen(QColor(0, 220, 255), 1.5));
    painter.drawRoundedRect(pbx, pby, pbW, pbH, pbH / 2, pbH / 2);

    painter.setFont(QFont("Arial", 9, QFont::Bold));
    painter.setPen(QColor(255, 215, 0));
    painter.drawText(pbx, pby, pbW, pbH, Qt::AlignCenter,
                     QString("⭐ CURRENT PROGRESS: LEVEL %1").arg(highestUnlockedLevel));

    // 5. Mode & Hero Selector: [ 🟡 YELLOW (1P) ]  [ 🎀 PINK (1P) ]  [ 👥 CO-OP (2P) ]
    QRect selRect = getModeSelectorRect();
    QRect pYellow = getModeYellowRect().adjusted(2, 2, -2, -2);
    QRect pPink = getModePinkRect().adjusted(2, 2, -2, -2);
    QRect pCoop = getModeCoopRect().adjusted(2, 2, -2, -2);
    bool yellowHover = pYellow.contains(mousePos);
    bool pinkHover = pPink.contains(mousePos);
    bool coopHover = pCoop.contains(mousePos);

    // Pill container shadow & background
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 140));
    painter.drawRoundedRect(selRect.adjusted(2, 3, 2, 3), selRect.height() / 2, selRect.height() / 2);

    painter.setBrush(QColor(24, 12, 42, 235));
    painter.setPen(QPen(QColor(130, 80, 180), 2));
    painter.drawRoundedRect(selRect, selRect.height() / 2, selRect.height() / 2);

    // Button 1: Yellow Solo
    bool yellowActive = (!isMultiplayer && singlePlayerHero == 1);
    if (yellowActive) {
        QLinearGradient g(pYellow.left(), pYellow.top(), pYellow.left(), pYellow.bottom());
        g.setColorAt(0.0, QColor(255, 225, 50));
        g.setColorAt(0.5, QColor(240, 160, 20));
        g.setColorAt(1.0, QColor(200, 110, 10));
        painter.setBrush(g);
        painter.setPen(QPen(QColor(255, 250, 200), yellowHover ? 2.5 : 1.5));
        painter.drawRoundedRect(pYellow, pYellow.height() / 2, pYellow.height() / 2);

        painter.setFont(QFont("Arial", std::max(9, pYellow.height() * 3 / 8), QFont::Bold));
        painter.setPen(QColor(40, 20, 5));
        painter.drawText(pYellow, Qt::AlignCenter, "🟡 YELLOW (1P)");
    } else {
        if (yellowHover) {
            painter.setBrush(QColor(70, 40, 100, 180));
            painter.setPen(QPen(QColor(180, 140, 220), 1));
            painter.drawRoundedRect(pYellow, pYellow.height() / 2, pYellow.height() / 2);
        }
        painter.setFont(QFont("Arial", std::max(9, pYellow.height() * 3 / 8)));
        painter.setPen(QColor(220, 200, 140));
        painter.drawText(pYellow, Qt::AlignCenter, "🟡 YELLOW");
    }

    // Button 2: Pink Solo
    bool pinkActive = (!isMultiplayer && singlePlayerHero == 2);
    if (pinkActive) {
        QLinearGradient g(pPink.left(), pPink.top(), pPink.left(), pPink.bottom());
        g.setColorAt(0.0, QColor(255, 110, 185));
        g.setColorAt(0.5, QColor(235, 45, 135));
        g.setColorAt(1.0, QColor(180, 15, 90));
        painter.setBrush(g);
        painter.setPen(QPen(QColor(255, 230, 245), pinkHover ? 2.5 : 1.5));
        painter.drawRoundedRect(pPink, pPink.height() / 2, pPink.height() / 2);

        painter.setFont(QFont("Arial", std::max(9, pPink.height() * 3 / 8), QFont::Bold));
        painter.setPen(Qt::white);
        painter.drawText(pPink, Qt::AlignCenter, "🎀 PINK (1P)");
    } else {
        if (pinkHover) {
            painter.setBrush(QColor(90, 30, 80, 180));
            painter.setPen(QPen(QColor(220, 120, 190), 1));
            painter.drawRoundedRect(pPink, pPink.height() / 2, pPink.height() / 2);
        }
        painter.setFont(QFont("Arial", std::max(9, pPink.height() * 3 / 8)));
        painter.setPen(QColor(230, 160, 200));
        painter.drawText(pPink, Qt::AlignCenter, "🎀 PINK");
    }

    // Button 3: 2-Player Co-op
    if (isMultiplayer) {
        QLinearGradient g(pCoop.left(), pCoop.top(), pCoop.left(), pCoop.bottom());
        g.setColorAt(0.0, QColor(40, 210, 255));
        g.setColorAt(0.5, QColor(10, 140, 240));
        g.setColorAt(1.0, QColor(5, 80, 180));
        painter.setBrush(g);
        painter.setPen(QPen(QColor(200, 245, 255), coopHover ? 2.5 : 1.5));
        painter.drawRoundedRect(pCoop, pCoop.height() / 2, pCoop.height() / 2);

        painter.setFont(QFont("Arial", std::max(9, pCoop.height() * 3 / 8), QFont::Bold));
        painter.setPen(Qt::white);
        painter.drawText(pCoop, Qt::AlignCenter, "👥 CO-OP (2P)");
    } else {
        if (coopHover) {
            painter.setBrush(QColor(30, 50, 100, 180));
            painter.setPen(QPen(QColor(100, 180, 240), 1));
            painter.drawRoundedRect(pCoop, pCoop.height() / 2, pCoop.height() / 2);
        }
        painter.setFont(QFont("Arial", std::max(9, pCoop.height() * 3 / 8)));
        painter.setPen(QColor(160, 210, 245));
        painter.drawText(pCoop, Qt::AlignCenter, "👥 CO-OP");
    }

    // 6. Big PLAY Button at the Bottom
    QRect btnRect = getPlayButtonRect();
    bool btnHovered = btnRect.contains(mousePos);

    // Drop shadow
    painter.setBrush(QColor(0, 0, 0, 150));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(btnRect.adjusted(3, 4, 3, 4), btnRect.height() / 2, btnRect.height() / 2);

    // Button body
    QLinearGradient pGrad(btnRect.left(), btnRect.top(), btnRect.left(), btnRect.bottom());
    if (btnHovered) {
        pGrad.setColorAt(0.0, QColor(70, 240, 120));
        pGrad.setColorAt(0.5, QColor(30, 200, 80));
        pGrad.setColorAt(1.0, QColor(10, 150, 50));
    } else {
        pGrad.setColorAt(0.0, QColor(255, 175, 40));
        pGrad.setColorAt(0.5, QColor(235, 120, 20));
        pGrad.setColorAt(1.0, QColor(190, 70, 10));
    }
    painter.setBrush(pGrad);
    painter.setPen(QPen(QColor(255, 245, 180), btnHovered ? 3.5 : 2.5));
    painter.drawRoundedRect(btnRect, btnRect.height() / 2, btnRect.height() / 2);

    // Button text
    QString playText;
    if (isMultiplayer) {
        playText = QString("▶ PLAY 2-PLAYER CO-OP (LVL %1)").arg(selectedMapLevel);
    } else if (singlePlayerHero == 2) {
        playText = QString("▶ PLAY AS PINK MS. PACMAN (LVL %1)").arg(selectedMapLevel);
    } else {
        playText = QString("▶ PLAY AS YELLOW PACMAN (LVL %1)").arg(selectedMapLevel);
    }

    painter.setFont(QFont("Arial", std::max(12, btnRect.height() * 3 / 8), QFont::Bold));
    painter.setPen(QColor(40, 20, 5));
    painter.drawText(btnRect.adjusted(1, 2, 1, 2), Qt::AlignCenter, playText);
    painter.setPen(Qt::white);
    painter.drawText(btnRect, Qt::AlignCenter, playText);

    // Subtitle below button
    QString subText;
    if (isMultiplayer) {
        subText = QString("%1  •  [Arrows] Steer Active Hero  •  [Tab] Swap Arrows to Partner!").arg(getLevelTitle(selectedMapLevel));
    } else if (singlePlayerHero == 2) {
        subText = QString("%1  •  Steer Pink Ms. Pacman with [Arrows] or [WASD]!").arg(getLevelTitle(selectedMapLevel));
    } else {
        subText = QString("%1  •  Steer Yellow Pacman with [Arrows] or [WASD]!").arg(getLevelTitle(selectedMapLevel));
    }

    painter.setFont(QFont("Arial", std::max(8, btnRect.height() / 4)));
    painter.setPen(QColor(230, 215, 245));
    painter.drawText(0, btnRect.bottom() + 3, w, 16, Qt::AlignCenter, subText);

    // Top-Right Multiplayer Badge
    drawMultiplayerBadge(painter);

    // Classroom Multiplayer Lobby Overlay
    if (showMultiplayerLobby) {
        drawMultiplayerLobby(painter);
    }
}
