#include "mainwindow.h"
#include "soundmanager.h"
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
        SoundManager::instance().playClick();
        showMultiplayerLobby = !showMultiplayerLobby;
        update();
        return;
    }

    // Toggle Audio Mute / Unmute with [S]
    if (event->key() == Qt::Key_S) {
        bool muted = SoundManager::instance().toggleMute();
        if (!muted) SoundManager::instance().playClick();
        peekToastMessage = muted ? "🔇 AUDIO MUTED (Press [S] to Unmute)" : "🔊 RETRO AUDIO ON";
        peekToastTimer.start();
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
            SoundManager::instance().playClick();
            startLevel(selectedMapLevel);
            return;
        }
        if (event->key() == Qt::Key_Tab || event->key() == Qt::Key_C || event->key() == Qt::Key_P) {
            SoundManager::instance().playClick();
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
        if ((event->modifiers() & Qt::ShiftModifier) && event->key() == Qt::Key_R) {
            resetGameProgress();
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
            saveGameProgress();
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
            SoundManager::instance().playPeek();
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
            SoundManager::instance().playPeek();
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

    // In-Game Switch Active Host Character: [Tab] (Pac-Heroes or Ghosts!)
    if (event->key() == Qt::Key_Tab) {
        cycleNextHostRole();
        return;
    }

    // Direct role selection with number keys [1]-[4] in multiplayer
    if (isAnyMultiplayer() && !event->modifiers()) {
        if (event->key() == Qt::Key_1) { switchHostRole("p1"); return; }
        if (event->key() == Qt::Key_2) { switchHostRole("p2"); return; }
        if (event->key() == Qt::Key_3) { switchHostRole("p3"); return; }
        if (event->key() == Qt::Key_4) { switchHostRole("p4"); return; }
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

    // Local 2-Player Split-Keyboard Co-op (ONLY if strictly offline, 2P local co-op active, P2 alive, no online clients)
    bool isLocalOfflineCoop = isMultiplayer && !isOnlineMultiplayer() && (p2ClientId < 0) && p2Alive && (hostRole == "p1");
    if (isLocalOfflineCoop) {
        Direction p1Cmd = (arrowPlayer == 1) ? arrowAttempt : wasdAttempt;
        Direction p2Cmd = (arrowPlayer == 2) ? arrowAttempt : wasdAttempt;
        if (p2Cmd != STOP && p2Alive) {
            p2NextDir = p2Cmd;
            if (canMove(p2X, p2Y, p2Cmd)) {
                p2Dir = p2Cmd;
                movePlayer2();
            } else {
                p2Dir = p2Cmd;
            }
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
        // Host controls Player 1 (Yellow Pac-Man) by default, or P2 in solo pink mode, or active chosen role
        bool hostControlsP1 = (hostRole == "p1") || (!isAnyMultiplayer() && singlePlayerHero == 1);
        bool hostControlsP2 = (hostRole == "p2") || (!isAnyMultiplayer() && singlePlayerHero == 2);
        bool hostControlsP3 = (hostRole == "p3");
        bool hostControlsP4 = (hostRole == "p4");

        if (hostControlsP1) {
            if (p1Lives > 0 && !p1Alive && p1RespawnTicks == 0) {
                p1Alive = true;
            }
            if (p1Alive) {
                pacmanNextDir = moveCmd;
                movePacman();
                moveCount++;
                checkFruitPickup();
                checkGhostCollisions();
                checkLevelExit();
                updateFogOfWar();
                broadcastCurrentGameState();
                update();
            }
        } else if (hostControlsP2) {
            if (p2Lives > 0 && !p2Alive && p2RespawnTicks == 0) {
                p2Alive = true;
            }
            if (p2Alive) {
                p2NextDir = moveCmd;
                movePlayer2();
                moveCount++;
                checkFruitPickup();
                checkGhostCollisions();
                checkLevelExit();
                updateFogOfWar();
                broadcastCurrentGameState();
                update();
            }
        } else if (hostControlsP3) {
            if (hero3.lives > 0 && !hero3.alive && hero3.respawnTicks == 0) {
                hero3.alive = true;
            }
            if (hero3.alive) {
                hero3.nextDir = moveCmd;
                moveHero(hero3, 3);
                moveCount++;
                checkFruitPickup();
                checkGhostCollisions();
                checkLevelExit();
                updateFogOfWar();
                broadcastCurrentGameState();
                update();
            }
        } else if (hostControlsP4) {
            if (hero4.lives > 0 && !hero4.alive && hero4.respawnTicks == 0) {
                hero4.alive = true;
            }
            if (hero4.alive) {
                hero4.nextDir = moveCmd;
                moveHero(hero4, 4);
                moveCount++;
                checkFruitPickup();
                checkGhostCollisions();
                checkLevelExit();
                updateFogOfWar();
                broadcastCurrentGameState();
                update();
            }
        } else if (isHostGhost()) {
            int gIdx = getHostGhostIndex();
            if (gIdx >= 0 && gIdx < (int)ghosts.size()) {
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
    setFocus();
    activateWindow();
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

    if (getSoundButtonRect().contains(event->pos())) {
        bool muted = SoundManager::instance().toggleMute();
        if (!muted) SoundManager::instance().playClick();
        peekToastMessage = muted ? "🔇 AUDIO MUTED (Press [S] to Unmute)" : "🔊 RETRO AUDIO ON";
        peekToastTimer.start();
        update();
        return;
    }

    if (getMultiplayerButtonRect().contains(event->pos())) {
        SoundManager::instance().playClick();
        showMultiplayerLobby = !showMultiplayerLobby;
        update();
        return;
    }

    if (currentScreen == SCREEN_HOME_MAP) {
        QPoint pos = event->pos();

        // 1. Clicked My Hero or Record badges (Quick spring camera jump!)
        if (getMyLevelButtonRect().contains(pos) || getRecordBadgeRect().contains(pos)) {
            SoundManager::instance().playClick();
            selectedMapLevel = highestUnlockedLevel;
            scrollToLevel(highestUnlockedLevel, false);
            update();
            return;
        }

        // 2. Clicked Mode Selector buttons
        if (getModeYellowRect().contains(pos)) {
            SoundManager::instance().playClick();
            isMultiplayer = false;
            singlePlayerHero = 1;
            arrowPlayer = 1;
            saveGameProgress();
            update();
            return;
        }
        if (getModePinkRect().contains(pos)) {
            SoundManager::instance().playClick();
            isMultiplayer = false;
            singlePlayerHero = 2;
            arrowPlayer = 2;
            saveGameProgress();
            update();
            return;
        }
        if (getModeCoopRect().contains(pos)) {
            SoundManager::instance().playClick();
            isMultiplayer = true;
            update();
            return;
        }

        // 3. Clicked Play button
        if (getPlayButtonRect().contains(pos)) {
            SoundManager::instance().playClick();
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
            saveGameProgress();
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

