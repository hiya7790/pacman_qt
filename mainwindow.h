#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QKeyEvent>
#include <QTimer>
#include <QVector>
#include <QElapsedTimer>

enum Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    STOP
};

struct Ghost {
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

private slots:
    void gameLoop();

private:
    void initGame();
    void initLevel();
    void generateMaze();
    void movePacman();
    void moveGhosts();
    bool canMove(int x, int y, Direction dir);
    void trySpawnFruit();
    void checkFruitPickup();
    void checkGhostCollisions();
    void respawnGhostFarAway(Ghost &g);
    void updateFogOfWar();
    bool isCellVisible(int x, int y) const;
    void drawCherry(QPainter &painter, int cx, int cy, int s);
    void drawApple(QPainter &painter, int cx, int cy, int s);
    void drawStrawberry(QPainter &painter, int cx, int cy, int s);
    void drawOrangeFruit(QPainter &painter, int cx, int cy, int s);

    static const int ROWS = 21;
    static const int COLS = 21;
    static constexpr double VISION_RADIUS = 3.6;
    static const int FOG_PEEK_DURATION_MS = 1500; // 1.5 seconds temporary peek

    int board[ROWS][COLS]; // 0 = empty, 1 = wall, 2 = dot

    int pacmanX;
    int pacmanY;
    Direction pacmanDir;
    Direction pacmanNextDir;
    int pacmanMouthAngle;
    bool pacmanMouthOpening;

    int score;
    int moveCount;
    bool gameOver;
    bool levelComplete;

    // Fog of War / Dark Maze system
    bool fogOfWarEnabled;
    bool fogPeekActive;
    QElapsedTimer fogPeekTimer;
    bool explored[ROWS][COLS];

    // Level system
    int currentLevel;
    int ghostTimerInterval;

    // Exit point
    int exitX, exitY;

    // Frightened mode (power-up from eating fruit)
    bool ghostsFrightened;
    int frightenedTicksLeft;
    static const int FRIGHTEN_DURATION = 25; // ~3.2 seconds at 130ms tick

    // Timer display
    QElapsedTimer elapsedTimer;

    QTimer *timer;
    QVector<Ghost> ghosts;

    // Bonus fruit system
    QVector<BonusFruit> activeFruits;
    int fruitSpawnCounter;
    static const int MAX_FRUITS = 3;

    // Ghost speed control
    int ghostTickCounter;
    int ghostMoveEveryN;
};

#endif // MAINWINDOW_H
