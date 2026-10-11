#ifndef MAZESOLVER_H
#define MAZESOLVER_H

#include <QString>
#include <QVector>
#include <QQueue>
#include <cmath>
#include <queue>
#include <vector>

struct Point {
    int x;
    int y;

    bool operator==(const Point &other) const {
        return x == other.x && y == other.y;
    }

    bool operator!=(const Point &other) const {
        return !(*this == other);
    }
};

struct SolveResult {
    QVector<Point> path;           // Path from start to goal
    QVector<Point> exploredOrder;  // Order of explored cells
    bool found = false;
};

// Abstract Strategy for Maze Solver Pattern
class MazeSolverStrategy {
public:
    virtual ~MazeSolverStrategy() = default;
    virtual QString name() const = 0;
    virtual QString description() const = 0;
    virtual SolveResult solve(const int board[21][21], Point start, Point goal) = 0;

protected:
    static QVector<Point> getNeighbors(Point p, const int board[21][21]) {
        QVector<Point> neighbors;
        // Direction order: UP, RIGHT, DOWN, LEFT
        const int dx[] = {0, 1, 0, -1};
        const int dy[] = {-1, 0, 1, 0};

        for (int i = 0; i < 4; ++i) {
            int nx = p.x + dx[i];
            int ny = p.y + dy[i];

            // Wrap around horizontal tunnels
            if (nx < 0) nx = 21 - 1;
            if (nx >= 21) nx = 0;

            if (ny >= 0 && ny < 21) {
                if (board[ny][nx] != 1) { // 1 is wall
                    neighbors.push_back({nx, ny});
                }
            }
        }
        return neighbors;
    }
};

// 1. BFS Solver (Guaranteed Shortest Path)
class BFSSolver : public MazeSolverStrategy {
public:
    QString name() const override { return "BFS"; }
    QString description() const override { return "Breadth-First Search (Shortest Path)"; }

    SolveResult solve(const int board[21][21], Point start, Point goal) override {
        SolveResult result;
        if (start == goal) {
            result.path.push_back(start);
            result.found = true;
            return result;
        }

        bool visited[21][21] = {};
        Point parent[21][21];
        QQueue<Point> queue;

        queue.enqueue(start);
        visited[start.y][start.x] = true;

        bool reached = false;
        while (!queue.isEmpty()) {
            Point curr = queue.dequeue();
            result.exploredOrder.push_back(curr);

            if (curr == goal) {
                reached = true;
                break;
            }

            for (Point next : getNeighbors(curr, board)) {
                if (!visited[next.y][next.x]) {
                    visited[next.y][next.x] = true;
                    parent[next.y][next.x] = curr;
                    queue.enqueue(next);
                }
            }
        }

        if (reached) {
            result.found = true;
            Point curr = goal;
            while (curr != start) {
                result.path.push_front(curr);
                curr = parent[curr.y][curr.x];
            }
            result.path.push_front(start);
        }

        return result;
    }
};

// 2. A* Solver (Heuristic Shortest Path)
class AStarSolver : public MazeSolverStrategy {
public:
    QString name() const override { return "A*"; }
    QString description() const override { return "A* Search (Optimal & Fast)"; }

    SolveResult solve(const int board[21][21], Point start, Point goal) override {
        SolveResult result;
        if (start == goal) {
            result.path.push_back(start);
            result.found = true;
            return result;
        }

        struct Node {
            Point pt;
            int g;
            int f;
            bool operator>(const Node &other) const {
                return f > other.f;
            }
        };

        std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;
        int gScore[21][21];
        bool closedSet[21][21] = {};
        Point parent[21][21];

        for (int r = 0; r < 21; ++r) {
            for (int c = 0; c < 21; ++c) {
                gScore[r][c] = 1e9;
            }
        }

        auto heuristic = [](Point a, Point b) {
            return std::abs(a.x - b.x) + std::abs(a.y - b.y);
        };

        gScore[start.y][start.x] = 0;
        openSet.push({start, 0, heuristic(start, goal)});

        bool reached = false;
        while (!openSet.empty()) {
            Node current = openSet.top();
            openSet.pop();
            Point curr = current.pt;

            if (closedSet[curr.y][curr.x]) continue;
            closedSet[curr.y][curr.x] = true;
            result.exploredOrder.push_back(curr);

            if (curr == goal) {
                reached = true;
                break;
            }

            for (Point next : getNeighbors(curr, board)) {
                if (closedSet[next.y][next.x]) continue;

                int tentativeG = gScore[curr.y][curr.x] + 1;
                if (tentativeG < gScore[next.y][next.x]) {
                    gScore[next.y][next.x] = tentativeG;
                    parent[next.y][next.x] = curr;
                    int f = tentativeG + heuristic(next, goal);
                    openSet.push({next, tentativeG, f});
                }
            }
        }

        if (reached) {
            result.found = true;
            Point curr = goal;
            while (curr != start) {
                result.path.push_front(curr);
                curr = parent[curr.y][curr.x];
            }
            result.path.push_front(start);
        }

        return result;
    }
};

// 3. DFS Solver (Depth-First Search)
class DFSSolver : public MazeSolverStrategy {
public:
    QString name() const override { return "DFS"; }
    QString description() const override { return "Depth-First Search (Explores Deep Paths)"; }

    SolveResult solve(const int board[21][21], Point start, Point goal) override {
        SolveResult result;
        if (start == goal) {
            result.path.push_back(start);
            result.found = true;
            return result;
        }

        bool visited[21][21] = {};
        Point parent[21][21];
        QVector<Point> stack;

        stack.push_back(start);
        visited[start.y][start.x] = true;

        bool reached = false;
        while (!stack.isEmpty()) {
            Point curr = stack.takeLast();
            result.exploredOrder.push_back(curr);

            if (curr == goal) {
                reached = true;
                break;
            }

            for (Point next : getNeighbors(curr, board)) {
                if (!visited[next.y][next.x]) {
                    visited[next.y][next.x] = true;
                    parent[next.y][next.x] = curr;
                    stack.push_back(next);
                }
            }
        }

        if (reached) {
            result.found = true;
            Point curr = goal;
            while (curr != start) {
                result.path.push_front(curr);
                curr = parent[curr.y][curr.x];
            }
            result.path.push_front(start);
        }

        return result;
    }
};

#endif // MAZESOLVER_H
