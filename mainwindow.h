#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QKeyEvent>
#include <QTimer>
#include <QVector>

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
    int ticksRemaining; // disappears after this many game ticks
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
    void movePacman();
    void moveGhosts();
    bool canMove(int x, int y, Direction dir);
    void trySpawnFruit();
    void checkFruitPickup();
    void drawCherry(QPainter &painter, int cx, int cy);
    void drawApple(QPainter &painter, int cx, int cy);
    void drawStrawberry(QPainter &painter, int cx, int cy);
    void drawOrangeFruit(QPainter &painter, int cx, int cy);

    static const int ROWS = 21;
    static const int COLS = 21;
    static const int CELL_SIZE = 30;

    int board[ROWS][COLS]; // 0 = empty, 1 = wall, 2 = dot

    int pacmanX;
    int pacmanY;
    Direction pacmanDir;
    Direction pacmanNextDir;
    int pacmanMouthAngle;
    bool pacmanMouthOpening;

    int score;
    bool gameOver;
    bool gameWon;

    QTimer *timer;
    QVector<Ghost> ghosts;

    // Bonus fruit system
    QVector<BonusFruit> activeFruits;
    int fruitSpawnCounter;       // ticks until next spawn attempt
    static const int MAX_FRUITS = 3; // max fruits on screen at once
};

#endif // MAINWINDOW_H
