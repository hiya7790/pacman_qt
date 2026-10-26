#include "mainwindow.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // Toggle Fullscreen with F11
    if (event->key() == Qt::Key_F11) {
        if (isFullScreen()) {
            showNormal();
        } else {
            showFullScreen();
        }
        update();
        return;
    }

    // Exit fullscreen with Escape key
    if (event->key() == Qt::Key_Escape && isFullScreen()) {
        showNormal();
        update();
        return;
    }

    // Toggle Multiplayer Classroom Lobby with [H]
    if (event->key() == Qt::Key_H) {
        showMultiplayerLobby = !showMultiplayerLobby;
        update();
        return;
    }

    if (showMultiplayerLobby) {
        if (event->key() == Qt::Key_Escape) {
            showMultiplayerLobby = false;
            update();
            return;
        }
        if (event->key() == Qt::Key_C ||
            ((event->modifiers() & (Qt::ControlModifier | Qt::MetaModifier)) && event->key() == Qt::Key_C)) {
            copyRoomLinkToClipboard();
            return;
        }
    }

    // Home Map Key Handling
    if (currentScreen == SCREEN_HOME_MAP) {
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            startLevel(selectedMapLevel);
            return;
        }
        if (event->key() == Qt::Key_Tab || event->key() == Qt::Key_C || event->key() == Qt::Key_P) {
            if (!isMultiplayer && singlePlayerHero == 1) {
                singlePlayerHero = 2; // Switch to Pink solo
                isMultiplayer = false;
            } else if (!isMultiplayer && singlePlayerHero == 2) {
                isMultiplayer = true; // Switch to 2P Co-op
            } else {
                isMultiplayer = false;
                singlePlayerHero = 1; // Switch to Yellow solo
            }
            update();
            return;
        }
        if (event->key() == Qt::Key_1) {
            isMultiplayer = false;
            singlePlayerHero = 1;
            update();
            return;
        }
        if (event->key() == Qt::Key_2) {
            isMultiplayer = false;
            singlePlayerHero = 2;
            update();
            return;
        }
        if (event->key() == Qt::Key_3) {
            isMultiplayer = true;
            update();
            return;
        }
        if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Right) {
            if (selectedMapLevel < getTotalMappedLevels()) {
                selectedMapLevel++;
                scrollToLevel(selectedMapLevel, false);
                update();
            }
            return;
        }
        if (event->key() == Qt::Key_Down || event->key() == Qt::Key_Left) {
            if (selectedMapLevel > 1) {
                selectedMapLevel--;
                scrollToLevel(selectedMapLevel, false);
                update();
            }
            return;
        }
        if (event->key() == Qt::Key_PageUp || event->key() == Qt::Key_BracketRight) {
            int maxScroll = std::max(0, getVirtualMapHeight() - height());
            targetMapScrollY = std::clamp(targetMapScrollY - height() * 0.75f, 0.0f, float(maxScroll));
            update();
            return;
        }
        if (event->key() == Qt::Key_PageDown || event->key() == Qt::Key_BracketLeft) {
            int maxScroll = std::max(0, getVirtualMapHeight() - height());
            targetMapScrollY = std::clamp(targetMapScrollY + height() * 0.75f, 0.0f, float(maxScroll));
            update();
            return;
        }
        if (event->key() == Qt::Key_Home) {
            selectedMapLevel = 1;
            scrollToLevel(1, false);
            update();
            return;
        }
        if (event->key() == Qt::Key_End) {
            selectedMapLevel = highestUnlockedLevel;
            scrollToLevel(highestUnlockedLevel, false);
            update();
            return;
        }
        return;
    }

    // In-Game Game Over (Retry from that particular level!)
    if (gameOver) {
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_R) {
            initLevel();
            gameOver = false;
            update();
        } else if (event->key() == Qt::Key_M) {
            currentScreen = SCREEN_HOME_MAP;
            selectedMapLevel = currentLevel;
            viewedMapWorld = (selectedMapLevel - 1) / LEVELS_PER_WORLD + 1;
            scrollToLevel(selectedMapLevel, false);
            update();
        }
        return;
    }

    // In-Game Level Complete (Infinite progression!)
    if (levelComplete) {
        if (event->key() == Qt::Key_Space) {
            currentLevel++;
            if (currentLevel > highestUnlockedLevel) highestUnlockedLevel = currentLevel;
            selectedMapLevel = currentLevel;
            viewedMapWorld = (selectedMapLevel - 1) / LEVELS_PER_WORLD + 1;
            initLevel();
            levelComplete = false;
            update();
        } else if (event->key() == Qt::Key_M) {
            currentScreen = SCREEN_HOME_MAP;
            selectedMapLevel = currentLevel + 1;
            viewedMapWorld = (selectedMapLevel - 1) / LEVELS_PER_WORLD + 1;
            scrollToLevel(selectedMapLevel, false);
            update();
        }
        return;
    }

    // Return to Saga Map with [M]
    if (event->key() == Qt::Key_M) {
        currentScreen = SCREEN_HOME_MAP;
        selectedMapLevel = currentLevel;
        viewedMapWorld = (selectedMapLevel - 1) / LEVELS_PER_WORLD + 1;
        scrollToLevel(selectedMapLevel, false);
        update();
        return;
    }

    // Restart current level with [R]
    if (event->key() == Qt::Key_R) {
        initLevel();
        update();
        return;
    }

    // Fog Peek: 1.0-second temporary glimpse (limited to 3 times per level + extra by eating fruits!)
    if (event->key() == Qt::Key_F) {
        if (fogPeekActive) {
            return;
        }
        if (isHostGhost()) {
            fogOfWarEnabled = false;
            fogPeekActive = true;
            fogPeekTimer.start();
            peekToastMessage = "👻 GHOST SONAR ACTIVE! (1.0s) • Entire maze & all Pac-Heroes revealed!";
            peekToastTimer.start();
            broadcastCurrentGameState();
            update();
            return;
        }
        if (p1FogPeeks > 0) {
            p1FogPeeks--;
            fogPeeksRemaining = p1FogPeeks;
            fogOfWarEnabled = false;
            fogPeekActive = true;
            fogPeekTimer.start();
            peekToastMessage = QString("💡 HOST FOG PEEK ACTIVE! (1.0s) • %1 Left for Host").arg(p1FogPeeks);
            peekToastTimer.start();
            broadcastCurrentGameState();
        } else {
            peekToastMessage = "⚠️ HOST HAS NO FOG PEEKS LEFT! Eat bonus fruits 🍒 to earn more!";
            peekToastTimer.start();
        }
        update();
        return;
    }

    // In-Game Switch Active Host Character: [Tab] or [C] (Pac-Heroes or Ghosts!)
    if (event->key() == Qt::Key_Tab || event->key() == Qt::Key_C) {
        cycleNextHostRole();
        return;
    }

    // Toggle 2-Player Co-op drop-in on the fly with [2]
    if (event->key() == Qt::Key_2 && !isMultiplayer) {
        isMultiplayer = true;
        p2Alive = true;
        p2RespawnTicks = 0;
        p2X = (pacmanX > 1 && board[currentSectorY][currentSectorX][pacmanY][pacmanX - 1] != 1) ? pacmanX - 1 : pacmanX;
        p2Y = pacmanY;
        update();
        return;
    }

    // Prevent input during active camera window slide transition
    if (isTransitioning) return;

    // Arrow keys input
    Direction arrowAttempt = STOP;
    if (event->key() == Qt::Key_Up) arrowAttempt = UP;
    else if (event->key() == Qt::Key_Down) arrowAttempt = DOWN;
    else if (event->key() == Qt::Key_Left) arrowAttempt = LEFT;
    else if (event->key() == Qt::Key_Right) arrowAttempt = RIGHT;

    // WASD keys input
    Direction wasdAttempt = STOP;
    int k = event->key();
    QString txt = event->text().toLower();
    if (k == Qt::Key_W || txt == "w") wasdAttempt = UP;
    else if (k == Qt::Key_S || txt == "s") wasdAttempt = DOWN;
    else if (k == Qt::Key_A || txt == "a") wasdAttempt = LEFT;
    else if (k == Qt::Key_D || txt == "d") wasdAttempt = RIGHT;

    // Neither movement key pressed
    if (arrowAttempt == STOP && wasdAttempt == STOP) {
        QWidget::keyPressEvent(event);
        return;
    }

    Direction moveCmd = (arrowAttempt != STOP) ? arrowAttempt : wasdAttempt;

    // 2-Player Local Keyboard Co-op support (WASD steers Player 2)
    if (isMultiplayer && (p2ClientId < 0)) {
        Direction p1Cmd = (arrowPlayer == 1) ? arrowAttempt : wasdAttempt;
        Direction p2Cmd = (arrowPlayer == 2) ? arrowAttempt : wasdAttempt;
        if (p2Cmd != STOP && p2Alive) {
            p2NextDir = p2Cmd;
            movePlayer2();
            moveCount++;
            checkFruitPickup();
            checkGhostCollisions();
            checkLevelExit();
            updateFogOfWar();
            broadcastCurrentGameState();
            update();
        }
        moveCmd = p1Cmd;
    }

    if (moveCmd != STOP) {
        if (hostRole == "p1" && p1Alive) {
            pacmanNextDir = moveCmd;
            movePacman();
            moveCount++;
            checkFruitPickup();
            checkGhostCollisions();
            checkLevelExit();
            updateFogOfWar();
            broadcastCurrentGameState();
            update();
        } else if (hostRole == "p2" && p2Alive) {
            p2NextDir = moveCmd;
            movePlayer2();
            moveCount++;
            checkFruitPickup();
            checkGhostCollisions();
            checkLevelExit();
            updateFogOfWar();
            broadcastCurrentGameState();
            update();
        } else if (hostRole == "p3" && hero3.alive) {
            hero3.nextDir = moveCmd;
            if (canMove(hero3.x, hero3.y, moveCmd)) {
                hero3.dir = moveCmd;
                moveHero(hero3, 3);
                moveCount++;
                checkFruitPickup();
                checkGhostCollisions();
                checkLevelExit();
                updateFogOfWar();
            } else {
                hero3.dir = moveCmd;
            }
            broadcastCurrentGameState();
            update();
        } else if (hostRole == "p4" && hero4.alive) {
            hero4.nextDir = moveCmd;
            if (canMove(hero4.x, hero4.y, moveCmd)) {
                hero4.dir = moveCmd;
                moveHero(hero4, 4);
                moveCount++;
                checkFruitPickup();
                checkGhostCollisions();
                checkLevelExit();
                updateFogOfWar();
            } else {
                hero4.dir = moveCmd;
            }
            broadcastCurrentGameState();
            update();
        } else if (isHostGhost()) {
            int gIdx = getHostGhostIndex();
            if (gIdx >= 0 && gIdx < ghosts.size()) {
                moveHostGhost(gIdx, moveCmd);
                moveCount++;
                update();
            }
        }
        return;
    }

    QWidget::keyPressEvent(event);
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    if (currentScreen == SCREEN_HOME_MAP) {
        int delta = event->angleDelta().y();
        int maxScroll = std::max(0, getVirtualMapHeight() - height());
        targetMapScrollY -= delta * 1.1f;
        targetMapScrollY = std::clamp(targetMapScrollY, 0.0f, float(maxScroll));
        update();
        return;
    }
    QWidget::wheelEvent(event);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (showMultiplayerLobby) {
        QPoint pos = event->pos();
        if (getCopyLinkButtonRect().contains(pos) || getLobbyUrlBoxRect().contains(pos)) {
            copyRoomLinkToClipboard();
            return;
        }

        // Check if host clicked on any character card to switch role
        QStringList allRoles = {"p1", "p2", "p3", "p4", "blinky", "pinky", "inky", "clyde"};
        for (const QString &r : allRoles) {
            if (getLobbyRoleRect(r).contains(pos)) {
                switchHostRole(r);
                return;
            }
        }

        if (getLobbyCloseButtonRect().contains(pos) || !getLobbyModalRect().contains(pos)) {
            showMultiplayerLobby = false;
            update();
            return;
        }
        return;
    }

    if (getMultiplayerButtonRect().contains(event->pos())) {
        showMultiplayerLobby = !showMultiplayerLobby;
        update();
        return;
    }

    if (currentScreen == SCREEN_HOME_MAP) {
        QPoint pos = event->pos();

        // 1. Clicked My Hero or Record badges (Quick spring camera jump!)
        if (getMyLevelButtonRect().contains(pos) || getRecordBadgeRect().contains(pos)) {
            selectedMapLevel = highestUnlockedLevel;
            scrollToLevel(highestUnlockedLevel, false);
            update();
            return;
        }

        // 2. Clicked Mode Selector buttons
        if (getModeYellowRect().contains(pos)) {
            isMultiplayer = false;
            singlePlayerHero = 1;
            arrowPlayer = 1;
            update();
            return;
        }
        if (getModePinkRect().contains(pos)) {
            isMultiplayer = false;
            singlePlayerHero = 2;
            arrowPlayer = 2;
            update();
            return;
        }
        if (getModeCoopRect().contains(pos)) {
            isMultiplayer = true;
            update();
            return;
        }

        // 3. Clicked Play button
        if (getPlayButtonRect().contains(pos)) {
            if (selectedMapLevel <= highestUnlockedLevel) {
                startLevel(selectedMapLevel);
            } else {
                startLevel(highestUnlockedLevel);
            }
            return;
        }

        // 4. Clicked Level Node along continuous snake path
        int radius = std::clamp(std::min(width(), height()) / 24, 22, 32);
        int totalLvls = getTotalMappedLevels();
        bool clickedNode = false;

        for (int lvl = 1; lvl <= totalLvls; ++lvl) {
            QPoint np = getLevelNodePos(lvl, width(), height());
            if (np.y() < -50 || np.y() > height() + 50) continue;

            int dx = pos.x() - np.x();
            int dy = pos.y() - np.y();
            if (dx * dx + dy * dy <= (radius + 8) * (radius + 8)) {
                if (lvl <= highestUnlockedLevel) {
                    selectedMapLevel = lvl;
                    startLevel(lvl);
                    return;
                }
                selectedMapLevel = lvl;
                clickedNode = true;
                break;
            }
        }

        // 5. If not clicking a node or button, start drag scrolling
        if (!clickedNode) {
            isDraggingMap = true;
            dragStartY = pos.y();
            dragStartScrollY = targetMapScrollY;
        }

        update();
        return;
    }

    if (currentScreen == SCREEN_PLAYING) {
        QPoint pos = event->pos();

        // Clicked Saga Map button in HUD
        if (getMapButtonRect().contains(pos)) {
            currentScreen = SCREEN_HOME_MAP;
            selectedMapLevel = currentLevel;
            viewedMapWorld = (selectedMapLevel - 1) / LEVELS_PER_WORLD + 1;
            scrollToLevel(selectedMapLevel, false);
            update();
            return;
        }

        // Click on Pacman to toggle arrow control!
        int hudHeight = 72;
        int availW = width();
        int availH = height() - hudHeight;
        int cellSize = std::min(availW / COLS, availH / ROWS);
        int originX = (availW - COLS * cellSize) / 2;
        int originY = (availH - ROWS * cellSize) / 2;

        int p1CenterX = originX + pacmanX * cellSize + cellSize / 2;
        int p1CenterY = originY + pacmanY * cellSize + cellSize / 2;
        int p2CenterX = originX + p2X * cellSize + cellSize / 2;
        int p2CenterY = originY + p2Y * cellSize + cellSize / 2;

        if (std::hypot(pos.x() - p1CenterX, pos.y() - p1CenterY) <= cellSize * 1.5) {
            arrowPlayer = 1;
            if (!isMultiplayer) singlePlayerHero = 1;
            update();
            return;
        }
        if (isMultiplayer && std::hypot(pos.x() - p2CenterX, pos.y() - p2CenterY) <= cellSize * 1.5) {
            arrowPlayer = 2;
            update();
            return;
        }

        if (gameOver) {
            initLevel();
            gameOver = false;
            update();
            return;
        }

        if (levelComplete) {
            currentLevel++;
            if (currentLevel > highestUnlockedLevel) highestUnlockedLevel = currentLevel;
            selectedMapLevel = currentLevel;
            viewedMapWorld = (selectedMapLevel - 1) / LEVELS_PER_WORLD + 1;
            initLevel();
            levelComplete = false;
            update();
            return;
        }
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (currentScreen == SCREEN_HOME_MAP) {
        QPoint pos = event->pos();

        if (isDraggingMap) {
            float delta = pos.y() - dragStartY;
            int maxScroll = std::max(0, getVirtualMapHeight() - height());
            targetMapScrollY = std::clamp(dragStartScrollY - delta, 0.0f, float(maxScroll));
            mapScrollY = targetMapScrollY;
            update();
            return;
        }

        hoveredMapLevel = 0;
        int radius = std::clamp(std::min(width(), height()) / 24, 22, 32);
        int totalLvls = getTotalMappedLevels();

        for (int lvl = 1; lvl <= totalLvls; ++lvl) {
            QPoint np = getLevelNodePos(lvl, width(), height());
            if (np.y() < -50 || np.y() > height() + 50) continue;

            int dx = pos.x() - np.x();
            int dy = pos.y() - np.y();
            if (dx * dx + dy * dy <= (radius + 8) * (radius + 8)) {
                hoveredMapLevel = lvl;
                break;
            }
        }

        bool onMyHero = getMyLevelButtonRect().contains(pos);
        bool onRecord = getRecordBadgeRect().contains(pos);
        bool onMode = getModeSelectorRect().contains(pos);
        bool onPlay = getPlayButtonRect().contains(pos);
        if ((hoveredMapLevel != 0 && hoveredMapLevel <= highestUnlockedLevel) ||
            onMyHero || onRecord || onMode || onPlay) {
            setCursor(Qt::PointingHandCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }

        update();
    } else {
        if (getMapButtonRect().contains(event->pos())) {
            setCursor(Qt::PointingHandCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    isDraggingMap = false;
}

