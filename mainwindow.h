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
};

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
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void gameLoop();

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

    // Candy Crush Saga Map Home Page
    void drawCandyCrushMap(QPainter &painter);
    void drawLevelNode(QPainter &painter, int lvl, int cx, int cy, int radius);
    void drawPacmanAvatar(QPainter &painter, int cx, int cy, int size);
    void drawMsPacmanAvatar(QPainter &painter, int cx, int cy, int size);
    QPoint getLevelNodePos(int lvl, int mapW, int mapH) const;
    QRect getPlayButtonRect() const;
    QRect getPrevWorldButtonRect() const;
    QRect getNextWorldButtonRect() const;
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

    static constexpr int LEVELS_PER_WORLD = 8;
    int viewedMapWorld;
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

    static constexpr int ROWS = 21;
    static constexpr int COLS = 21;
    static constexpr int MAX_SECTORS_X = 6;
    static constexpr int MAX_SECTORS_Y = 6;
    static constexpr double DEFAULT_VISION_RADIUS = 3.8;
    double currentVisionRadius;
    int currentFrightenDuration;
    static constexpr int FOG_PEEK_DURATION_MS = 1500; // 1.5 seconds temporary peek
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
