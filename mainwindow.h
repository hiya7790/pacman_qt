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

    static const int ROWS = 21;
    static const int COLS = 21;
    static const int CELL_SIZE = 30;

    int board[ROWS][COLS]; // 0 = empty, 1 = wall, 2 = dot, 3 = power pellet

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
};

#endif // MAINWINDOW_H
