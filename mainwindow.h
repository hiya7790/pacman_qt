#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QVector>
#include <QElapsedTimer>

enum GameScreen {
    SCREEN_HOME_MAP,
    SCREEN_PLAYING
};

enum Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    STOP
};

struct Ghost {
    int sectorX, sectorY;
    int x, y;
    Direction dir;
    int colorIndex;
    int clientId = -1;       // -1 = AI, >= 0 = controlled by human
    QString playerName = ""; // Name of human controlling this ghost
};

struct ExtraHero {
    int x = 9, y = 19;
    Direction dir = STOP;
    Direction nextDir = STOP;
    int mouthAngle = 45;
    bool mouthOpening = false;
    bool alive = false;
    int respawnTicks = 0;
    int score = 0;
    int clientId = -1;       // -1 if not connected
    QString playerName = "";
};

class GameServer;

// Bonus fruit types
enum FruitType {
    CHERRY = 0,
    APPLE,
    STRAWBERRY,
    ORANGE_FRUIT
};

struct BonusFruit {
    int sectorX, sectorY;
    int x, y;
    FruitType type;
    int ticksRemaining;
};

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void gameLoop();
    void onPlayerJoined(int clientId, const QString &role, const QString &name);
    void onPlayerLeft(int clientId, const QString &role, const QString &name);
    void onPlayerMoved(int clientId, const QString &role, int dir);
    void onPlayerPeek(int clientId, const QString &role);

private:
    void initGame();
    void initLevel();
    void startLevel(int lvl);
    void getSectorDimsForLevel(int lvl, int &cols, int &rows) const;
    void generateSectorMaze(int sx, int sy);
    void movePacman();
    void moveGhosts();
    bool canMove(int x, int y, Direction dir);
    void trySpawnFruit();
    void checkFruitPickup();
    void checkGhostCollisions();
    void respawnGhostFarAway(Ghost &g);
    void updateFogOfWar();
    bool isCellVisible(int sx, int sy, int x, int y) const;
    void drawSector(QPainter &painter, int sx, int sy, int originX, int originY, int cellSize);
    void drawCherry(QPainter &painter, int cx, int cy, int s);
    void drawApple(QPainter &painter, int cx, int cy, int s);
    void drawStrawberry(QPainter &painter, int cx, int cy, int s);
    void drawOrangeFruit(QPainter &painter, int cx, int cy, int s);
    void drawGameScreen(QPainter &painter);

    // Connected Multiplayer & Classroom Lobby
    void moveHero(ExtraHero &hero, int heroId);
    void broadcastCurrentGameState();
    void drawMultiplayerBadge(QPainter &painter);
    void drawMultiplayerLobby(QPainter &painter);
    QRect getMultiplayerButtonRect() const;
    QRect getCopyLinkButtonRect() const;
    QRect getLobbyCloseButtonRect() const;
    QRect getLobbyModalRect() const;
    QRect getLobbyUrlBoxRect() const;
    void copyRoomLinkToClipboard();

    struct WorldPalette {
        QColor top;
        QColor mid1;
        QColor mid2;
        QColor bot;
    };

    // Candy Crush Saga Map Home Page (Continuous Snake Map Across All Worlds!)
    void drawCandyCrushMap(QPainter &painter);
    void drawLevelNode(QPainter &painter, int lvl, int cx, int cy, int radius);
    void drawWorldGateway(QPainter &painter, int boundaryLvl, int w, int h);
    void drawPacmanAvatar(QPainter &painter, int cx, int cy, int size);
    void drawMsPacmanAvatar(QPainter &painter, int cx, int cy, int size);
    QPoint getLevelNodePos(int lvl, int mapW, int mapH) const;
    int getVirtualMapHeight() const;
    float getNodeSpacingY() const;
    int getTotalMappedLevels() const;
    int getCurrentVisibleWorld() const;
    WorldPalette getWorldPalette(int world) const;
    void scrollToLevel(int lvl, bool immediate = false);
    QRect getPlayButtonRect() const;
    QRect getMyLevelButtonRect() const;
    QRect getRecordBadgeRect() const;
    QRect getMapButtonRect() const;
    QRect getModeSelectorRect() const;
    QRect getModeYellowRect() const;
    QRect getModePinkRect() const;
    QRect getModeCoopRect() const;
    QString getLevelTitle(int lvl) const;
    QString getWorldName(int world) const;

    // Movement & Level Exit Helpers
    void movePlayer2();
    void checkLevelExit();

    static constexpr int LEVELS_PER_WORLD = 20;
    int viewedMapWorld;
    float mapScrollY;
    float targetMapScrollY;
    bool isDraggingMap;
    int dragStartY;
    float dragStartScrollY;

    GameScreen currentScreen;
    int highestUnlockedLevel;
    int selectedMapLevel;
    int hoveredMapLevel;

    // Multiplayer & Character Selection
    bool isMultiplayer;
    int singlePlayerHero; // 1 = Yellow Pacman, 2 = Pink Ms. Pacman
    int arrowPlayer;      // Which character uses Arrow Keys: 1 = Yellow, 2 = Pink
    int scoreP1;
    int scoreP2;
    bool p1Alive;
    bool p2Alive;
    int p1RespawnTicks;
    int p2RespawnTicks;

    // Embedded Classroom Web Server & Extra Heroes
    GameServer *gameServer;
    bool showMultiplayerLobby;
    bool linkCopiedFeedback;
    QElapsedTimer linkCopiedTimer;
    int p2ClientId;
    QString p2PlayerName;
    ExtraHero hero3; // Jr. Pac-Man (Cyan)
    ExtraHero hero4; // Miracle Pac-Man (Green)
    bool isP2Active() const { return (p2ClientId >= 0) || (isMultiplayer && p2ClientId == -1); }
    bool isAnyMultiplayer() const { return isMultiplayer || (p2ClientId >= 0) || (hero3.clientId != -1) || (hero4.clientId != -1); }

    static constexpr int ROWS = 21;
    static constexpr int COLS = 21;
    static constexpr int MAX_SECTORS_X = 6;
    static constexpr int MAX_SECTORS_Y = 6;
    static constexpr double DEFAULT_VISION_RADIUS = 3.8;
    double currentVisionRadius;
    int currentFrightenDuration;
    static constexpr int FOG_PEEK_DURATION_MS = 1000; // 1.0 second temporary peek
    static constexpr int INITIAL_FOG_PEEKS = 3;       // 3 peeks per level by default
    static constexpr int TRANSITION_DURATION_MS = 240; // 240ms smooth camera slide

    // Multi-Window Sector Grid
    int sectorCols;
    int sectorRows;
    int currentSectorX;
    int currentSectorY;
    int board[MAX_SECTORS_Y][MAX_SECTORS_X][ROWS][COLS]; // 0 = empty, 1 = wall, 2 = dot
    bool explored[MAX_SECTORS_Y][MAX_SECTORS_X][ROWS][COLS];
    bool sectorDiscovered[MAX_SECTORS_Y][MAX_SECTORS_X];

    // Camera sliding transition between windows
    bool isTransitioning;
    int fromSectorX, fromSectorY;
    int toSectorX, toSectorY;
    int transitionDirX, transitionDirY;
    QElapsedTimer transitionTimer;

    // Pacman position (local to currentSector) - Player 1
    int pacmanX;
    int pacmanY;
    Direction pacmanDir;
    Direction pacmanNextDir;
    int pacmanMouthAngle;
    bool pacmanMouthOpening;

    // Player 2 (Ms. Pacman) position & animation (Multiplayer)
    int p2X;
    int p2Y;
    Direction p2Dir;
    Direction p2NextDir;
    int p2MouthAngle;
    bool p2MouthOpening;

    int score;
    int moveCount;
    bool gameOver;
    bool levelComplete;

    // Sector Unfolding notification
    bool newSectorJustDiscovered;
    QElapsedTimer newSectorTimer;

    // Fog of War / Dark Maze system
    bool fogOfWarEnabled;
    bool fogPeekActive;
    QElapsedTimer fogPeekTimer;
    int fogPeeksRemaining;
    int p1FogPeeks;
    int p2FogPeeks;
    int hero3FogPeeks;
    int hero4FogPeeks;
    QString peekToastMessage;
    QElapsedTimer peekToastTimer;

    // Level system
    int currentLevel;
    int ghostTimerInterval;

    // Exit point (located in exitSector)
    int exitSectorX, exitSectorY;
    int exitX, exitY;

    // Frightened mode (power-up from eating fruit)
    bool ghostsFrightened;
    int frightenedTicksLeft;
    static constexpr int FRIGHTEN_DURATION = 25; // ~3.2 seconds at 130ms tick

    // Timer display
    QElapsedTimer elapsedTimer;

    QTimer *timer;
    QVector<Ghost> ghosts;

    // Bonus fruit system
    QVector<BonusFruit> activeFruits;
    int fruitSpawnCounter;
    static constexpr int MAX_FRUITS = 4;

    // Ghost speed control
    int ghostTickCounter;
    int ghostMoveEveryN;
};

#endif // MAINWINDOW_H
