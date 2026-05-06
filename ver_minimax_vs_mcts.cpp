#include "board.h"
#include "minimax.h"
#include "mcts.h"
#include <iostream>
#include <chrono>
#include <string>

int main(int argc, char* argv[]) {
    // 默认参数
    int boardSize = 38;
    std::string minimaxColor = "white";
    int minimaxDepth = 5;
    double defendWeight = 1.12;
    std::string mctsColor = "black";
    int mctsIterations = 50000;

    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--size" && i + 1 < argc) {
            boardSize = std::stoi(argv[++i]);
        } else if (arg == "--minimax-color" && i + 1 < argc) {
            minimaxColor = argv[++i];
        } else if (arg == "--minimax-depth" && i + 1 < argc) {
            minimaxDepth = std::stoi(argv[++i]);
        } else if (arg == "--defend-weight" && i + 1 < argc) {
            defendWeight = std::stod(argv[++i]);
        } else if (arg == "--mcts-color" && i + 1 < argc) {
            mctsColor = argv[++i];
        } else if (arg == "--mcts-iterations" && i + 1 < argc) {
            mctsIterations = std::stoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]\n"
                      << "Options:\n"
                      << "  --size <n>             Board size (default: 38)\n"
                      << "  --minimax-color <c>    Minimax color: black/white (default: white)\n"
                      << "  --minimax-depth <n>    Minimax search depth (default: 5)\n"
                      << "  --defend-weight <d>    Defense weight (default: 1.12)\n"
                      << "  --mcts-color <c>       MCTS color: black/white (default: black)\n"
                      << "  --mcts-iterations <n>  MCTS iterations (default: 50000)\n"
                      << "  --help, -h             Show this help\n";
            return 0;
        }
    }

    Player minimaxPlayer = (minimaxColor == "black") ? BLACK : WHITE;
    Player mctsPlayer = (mctsColor == "black") ? BLACK : WHITE;

    if (minimaxPlayer == mctsPlayer) {
        std::cout << "Error: Minimax and MCTS cannot have the same color" << std::endl;
        return 1;
    }

    Gomoku g(boardSize);
    Minimax minimax(minimaxPlayer, minimaxDepth, defendWeight);
    MCTS mcts(mctsPlayer, mctsIterations);
    int currentTurn = 0;
    int lastX = 0, lastY = 0;
    Player lastPlayer = NONE;

    while (!g.GameOver()) {
        currentTurn++;
        int x, y;
        Player p = CurrentPlayer(currentTurn);

        if (p == minimax.getSelf()) {
            auto move = minimax.getBestMove(g);
            x = move.first, y = move.second;
            if (x == -1 || y == -1) {
                std::cout << "Minimax can't find a move" << std::endl;
                break;
            }
            g.set(x, y, p);
        }
        else {
            auto move = mcts.getBestMove(g);
            x = move.first, y = move.second;
            if (x == -1 || y == -1) {
                std::cout << "MCTS can't find a move" << std::endl;
                break;
            }
            g.set(x, y, p);
        }

        lastX = x;
        lastY = y;
        lastPlayer = p;

        if (g.Win(x, y, p)) {
            std::cout << (p == minimaxPlayer ? "minimax" : "mcts") << " win" << std::endl;
            break;
        }
    }

    if (g.GameOver() && lastPlayer != NONE && !g.Win(lastX, lastY, lastPlayer)) {
        std::cout << "draw" << std::endl;
    }

    return 0;
}