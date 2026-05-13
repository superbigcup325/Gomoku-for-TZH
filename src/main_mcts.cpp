#include "../include/board.h"
#include "../include/mcts.h"
#include <iostream>
#include <cstdlib>

void clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void show(int currentTurn, Player p) {
    std::cout << "current turn is " << currentTurn << std::endl;
    std::cout << "current player is " << (p == BLACK ? "black" : "white") << std::endl;
}

int main() {
    Gomoku g(15);
    MCTS mcts(WHITE, 10000);  // 白棋，每次搜索10000次
    int currentTurn = 0;

    while (!g.GameOver()) {
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
            auto move = mcts.getBestMove(g);
            x = move.first, y = move.second;
            if (x == -1 || y == -1) {
                std::cout << "can't find a move" << std::endl;
                break;
            }
            std::cout << "placed at (" << x << ", " << y << ")" << std::endl;
            g.set(x, y, p);
        }

        clearScreen();
        g.show();

        if (g.Win(x, y, p)) {
            std::cout << (p == BLACK ? "BLACK" : "WHITE") << " win" << std::endl;
            break;
        }
    }
    return 0;
}