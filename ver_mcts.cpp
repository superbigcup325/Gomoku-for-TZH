#include "board.h"
#include "mcts.h"
#include <iostream>

void show(int currentTurn, Player p) {
    std::cout << "current turn is " << currentTurn << std::endl;
    std::cout << "current player is " << (p == BLACK ? "black" : "white") << std::endl;
}

int main() {
    Gomoku g(15);
    MCTS mcts(WHITE, 10000);  // 白棋，每次搜索10000次
    int currentTurn = 0;

    while (!g.GameOver()) {
        g.show();
        currentTurn++;
        int x, y;
        Player p = CurrentPlayer(currentTurn);
        show(currentTurn, p);

        if (p == mcts.getSelf()) {
            auto move = mcts.getBestMove(g);
            x = move.first, y = move.second;
            if (x == -1 || y == -1) {
                std::cout << "can't find a move" << std::endl;
                break;
            }
            std::cout << "placed at (" << x << ", " << y << ")" << std::endl;
            g.set(x, y, p);
        }
        else {
            // 由人工输入
            // std::cin >> x >> y;
            // while (!g.validPosition(x, y, p)) {
            //     if (g.outOfRange(x, y)) std::cout << "out of range" << std::endl;
            //     else if (g.getColor(x, y) != NONE) std::cout << "already placed" << std::endl;
            //     else if (g.isForbidden(x, y, p)) std::cout << "forbidden" << std::endl;
            //     std::cout << "please try again: ";
            //     std::cin >> x >> y;
            // }
            // g.set(x, y, p);

            // AI 博弈
            auto move = mcts.getBestMove(g);
            x = move.first, y = move.second;
            if (x == -1 || y == -1) {
                std::cout << "can't find a move" << std::endl;
                break;
            }
            std::cout << "placed at (" << x << ", " << y << ")" << std::endl;
            g.set(x, y, p);
        }

        if (g.Win(x, y, p)) {
            std::cout << (p == BLACK ? "BLACK" : "WHITE") << " win" << std::endl;
            g.show();
            break;
        }
    }
    return 0;
}