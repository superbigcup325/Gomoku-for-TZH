#include "../include/board.h"
#include "../include/minimax.h"
#include "../include/mcts.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <ctime>

struct Move {
    int x, y;
};

class SimpleParser {
public:
    static std::vector<std::string> split(const std::string& s, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    static int parseInt(const std::string& s) {
        try {
            return std::stoi(s);
        } catch (...) {
            return 0;
        }
    }
};

// 简单的类JSON输出格式
void outputResult(bool success, int x, int y, const std::string& engine,
                  const std::string& algorithm, int nodesSearched = 0) {
    std::cout << "{\n";
    std::cout << "  \"success\": " << (success ? "true" : "false") << ",\n";
    if (success) {
        std::cout << "  \"move\": {\"x\": " << x << ", \"y\": " << y << "},\n";
        std::cout << "  \"engine\": \"" << engine << "\",\n";
        std::cout << "  \"algorithm\": \"" << algorithm << "\",\n";
        if (nodesSearched > 0) {
            std::cout << "  \"nodesSearched\": " << nodesSearched << ",\n";
        }
    } else {
        std::cout << "  \"error\": \"Invalid position\",\n";
    }
    std::cout << "  \"timestamp\": " << std::time(nullptr) << "\n";
    std::cout << "}\n";
}

int main() {
    // 从stdin读取所有输入
    std::stringstream buffer;
    buffer << std::cin.rdbuf();
    std::string input = buffer.str();

    // 解析参数（简化格式：每行一个 key=value）
    int boardSize = 15;
    int selfPlayer = 1; // BLACK
    std::string algorithm = "minimax";
    int searchDepth = 4;
    int iterations = 1000;

    std::vector<Move> history;

    auto lines = SimpleParser::split(input, '\n');
    for (const auto& line : lines) {
        if (line.empty() || line[0] == '#') continue; // 跳过空行和注释

        auto parts = SimpleParser::split(line, '=');
        if (parts.size() >= 2) {
            const std::string& key = parts[0];
            const std::string& value = parts[1];

            if (key == "boardSize") boardSize = SimpleParser::parseInt(value);
            else if (key == "selfPlayer") selfPlayer = SimpleParser::parseInt(value);
            else if (key == "algorithm") algorithm = value;
            else if (key == "searchDepth") searchDepth = SimpleParser::parseInt(value);
            else if (key == "iterations") iterations = SimpleParser::parseInt(value);
            else if (key == "move") {
                auto coords = SimpleParser::split(value, ',');
                if (coords.size() >= 2) {
                    history.push_back({SimpleParser::parseInt(coords[0]),
                                       SimpleParser::parseInt(coords[1])});
                }
            }
        }
    }

    try {
        // 创建棋盘
        Gomoku g(boardSize);

        // 恢复历史走法
        for (size_t i = 0; i < history.size(); i++) {
            Player player = (i % 2 == 0) ? BLACK : WHITE;
            g.set(history[i].x, history[i].y, player);
        }

        // 计算最佳走法
        std::pair<int, int> bestMove = {-1, -1};
        int nodesSearched = 0;

        Player self = static_cast<Player>(selfPlayer);

        if (algorithm == "minimax") {
            Minimax ai(self, searchDepth);
            bestMove = ai.getBestMove(g);
            nodesSearched = ai.getNodesSearched();
        } else if (algorithm == "mcts") {
            MCTS ai(self, iterations);
            bestMove = ai.getBestMove(g);
            nodesSearched = ai.getNodesSearched();
        }

        // 输出结果
        outputResult(bestMove.first != -1 && bestMove.second != -1,
                    bestMove.first, bestMove.second,
                    "cpp", algorithm, nodesSearched);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        outputResult(false, -1, -1, "cpp", algorithm);
        return 1;
    }

    return 0;
}
