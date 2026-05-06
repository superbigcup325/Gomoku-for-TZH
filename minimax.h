#ifndef GOMOKU_MINIMAX_H
#define GOMOKU_MINIMAX_H

#include "board.h"
#include <vector>
#include <algorithm>
#include <climits>
#include <cmath>

class Minimax {
private:
    Player self;
    Player opponent;
    int maxDepth;
    int nodeCount;
    const double defendWeight;
    
    // 阶段权重
    const double openingWeight = 0.8;    // 开局倾向
    const double middleWeight = 1.0;     // 中局倾向
    const double endgameWeight = 1.2;    // 残局倾向
    
    // 进攻/防守系数（根据角色调整）
    double attackCoeff;
    double defenseCoeff;

    // 基础棋型分数
    inline int basePatternScore(const Gomoku::Pattern& pattern) const {
        switch (pattern.form()) {
            case FIVE:      return 10000000;
            case OVERLINE:  return 10000000;
            case FLEX4:     return 1000000;
            case BLOCK4:    return 500000;
            case FLEX3:     return 50000;
            case BLOCK3:    return 1000;
            case FLEX2:     return 200;
            case BLOCK2:    return 50;
            case FLEX1:     return 10;
            case BLOCK1:    return 2;
            default:        return 0;
        }
    }

    // 位置价值评估（中心比边缘更有价值）
    inline int positionScore(const Gomoku& g, int x, int y) const {
        int size = g.getSize();
        int center = (size + 1) / 2;
        int distX = std::abs(x - center);
        int distY = std::abs(y - center);
        int dist = std::max(distX, distY);
        
        int maxDist = size / 2;
        if (dist >= maxDist) return 1;
        return (maxDist - dist + 1) * 5;
    }

    // 根据阶段获取权重
    inline double getStageWeight(const Gomoku& g) const {
        int totalCells = g.getSize() * g.getSize();
        int placed = g.getCurrentCount();
        
        // 简化计算：根据已落子数判断阶段
        if (placed < 10) return openingWeight;      // 开局
        if (placed < totalCells * 0.6) return middleWeight;  // 中局
        return endgameWeight;                        // 残局
    }

    // 评估棋型组合（如双活三、冲四活三等）
    inline int evaluateCombination(const Gomoku& g, int x, int y, Player player) const {
        int flex4 = 0, block4 = 0, flex3 = 0;
        
        for (auto& [dx, dy] : Config::directions) {
            Gomoku::Pattern p = g.analyzeForm(x, y, dx, dy, player);
            if (p.isFlex4()) flex4++;
            else if (p.isBlock4()) block4++;
            else if (p.isFlex3()) flex3++;
        }
        
        int comboScore = 0;
        
        if (player == WHITE && (flex4 >= 2 || flex4 + block4 >= 2)) {
            comboScore += 2000000;
        }
        
        if (flex4 >= 1 && flex3 >= 1) {
            comboScore += 1000000;
        }
        
        if (flex3 >= 2) {
            comboScore += 300000;
        }
        
        if (block4 >= 1 && flex3 >= 1) {
            comboScore += 200000;
        }
        
        if (block4 >= 2) {
            comboScore += 150000;
        }
        
        return comboScore;
    }

    int evaluatePiece(const Gomoku& g,int x,int y,Player p);
    int evaluateEmpty(const Gomoku& g,int x,int y);
    int evaluate(const Gomoku& g);
    std::vector<std::pair<int,int>> getCandidateMoves(const Gomoku& g);
    int alphaBeta(Gomoku& g,int depth,int alpha,int beta,bool isMaximizing);

public:
    Minimax(const Player self_value,const int maxDepth_value,const double defendWeight_value=1.1);
    std::pair<int,int> getBestMove(Gomoku& g);
    void setDepth(int depth_value) { maxDepth=depth_value; }
    int getNodesSearched() const { return nodeCount; }
    Player getSelf() const { return self; }
};

// 单点棋型评估（包含位置价值和组合评估）
inline int Minimax::evaluatePiece(const Gomoku& g,int x,int y,Player player) {
    if (g.getColor(x,y)!=player) return 0;
    
    int score=0;
    
    // 基础棋型分数
    for (auto& [dx,dy]:Config::directions) {
        Gomoku::Pattern pattern=g.analyzeForm(x,y,dx,dy,player);
        score+=basePatternScore(pattern);
    }
    
    // 位置价值加成（开局阶段更重视位置）
    double stageWeight = getStageWeight(g);
    if (stageWeight < 1.0) {
        score += positionScore(g, x, y);
    }
    
    // 棋型组合加成
    score += evaluateCombination(g, x, y, player);
    
    return score;
}

// 空位潜力评估（考虑位置价值）
inline int Minimax::evaluateEmpty(const Gomoku& g,int x,int y) {
    if (g.getColor(x,y)!=NONE) return 0;
    
    int score=0;
    
    // 双方在该位置的棋型潜力
    for (auto& [dx,dy]:Config::directions) {
        score += basePatternScore(g.analyzeForm(x,y,dx,dy,BLACK));
        score += basePatternScore(g.analyzeForm(x,y,dx,dy,WHITE));
    }
    
    // 位置价值加成
    score += positionScore(g, x, y);
    
    return score;
}

// 全盘评估（只评估有棋子的位置，用 evaluated 数组全局去重）
inline int Minimax::evaluate(const Gomoku& g) {
    int selfScore=0;
    int opponentScore=0;
    const int size=g.getSize();
    std::vector<bool> evaluated(size*size,false);

    // 获取阶段权重
    double stageWeight = getStageWeight(g);

    for (int x=1;x<=size;x++) {
        for (int y=1;y<=size;y++) {
            Player p=g.getColor(x,y);
            if (p==NONE) continue;

            // 避免重复评估同一串棋子
            int idx=(x-1)*size+(y-1);
            if (evaluated[idx]) continue;

            // 只从每串棋子的端点评估一次
            if (p==self) {
                // 自身分数：进攻方权重调整
                int pieceScore = evaluatePiece(g, x, y, self);
                selfScore += static_cast<int>(pieceScore * attackCoeff * stageWeight);
            } else if (p==opponent) {
                // 对手分数：防守方权重调整
                int pieceScore = evaluatePiece(g, x, y, opponent);
                opponentScore += static_cast<int>(pieceScore * defenseCoeff * stageWeight);
            }

            // 标记该棋子四个方向上所有同色连续棋子
            evaluated[idx]=true;
            for (auto& [dx,dy]:Config::directions) {
                int nx=x+dx,ny=y+dy;
                while (!g.outOfRange(nx,ny)&&g.getColor(nx,ny)==p) {
                    evaluated[(nx-1)*size+(ny-1)]=true;
                    nx+=dx;
                    ny+=dy;
                }
                nx=x-dx,ny=y-dy;
                while (!g.outOfRange(nx,ny)&&g.getColor(nx,ny)==p) {
                    evaluated[(nx-1)*size+(ny-1)]=true;
                    nx-=dx;
                    ny-=dy;
                }
            }
        }
    }

    // 最终评估值：自身进攻得分 - 对手防守得分
    double val = static_cast<double>(selfScore) - static_cast<double>(opponentScore) * defendWeight;
    return static_cast<int>(val);
}

// 获取候选移动
inline std::vector<std::pair<int,int>> Minimax::getCandidateMoves(const Gomoku& g) {
    int size=g.getSize();
    std::vector<bool> visited(size*size,false);
    std::vector<std::pair<int,int>> candidates;

    for (int x=1;x<=size;x++)
    for (int y=1;y<=size;y++)
    if (g.getColor(x,y)!=NONE)
    for (int dx=-2;dx<=2;dx++)
    for (int dy=-2;dy<=2;dy++) {
        int nx=x+dx,ny=y+dy;
        if (!g.outOfRange(nx,ny)&&g.getColor(nx,ny)==NONE) {
            int idx=(nx-1)*size+(ny-1);
            if (!visited[idx]) {
                candidates.push_back({nx,ny});
                visited[idx]=true;
            }
        }
    }

    if (candidates.empty()) {
        candidates.push_back({(size+1)/2,(size+1)/2});
        return candidates;
    }

    // 按空位潜力排序
    std::sort(candidates.begin(),candidates.end(),
        [&](const std::pair<int,int>& a,const std::pair<int,int>& b) {
            return evaluateEmpty(g,a.first,a.second)>evaluateEmpty(g,b.first,b.second);
        });

    // 限制候选数量
    if (candidates.size()>15) {
        candidates.resize(15);
    }

    return candidates;
}

// Alpha-Beta 搜索
inline int Minimax::alphaBeta(Gomoku& g,int depth,int alpha,int beta,bool isMaximizing) {
    nodeCount++;

    if (depth==0||g.GameOver()) return evaluate(g);

    auto moves=getCandidateMoves(g);
    if (moves.empty()) return 0;

    if (isMaximizing) {
        int maxValue=INT_MIN;
        for (auto& [x,y]:moves) {
            if (!g.validPosition(x,y,self)) continue;
            g.set(x,y,self);
            if (g.Win(x,y,self)) {
                g.undo(x,y);
                return 10000000+depth;
            }
            int value=alphaBeta(g,depth-1,alpha,beta,false);
            g.undo(x,y);
            maxValue=std::max(maxValue,value);
            alpha=std::max(alpha,value);
            if (beta<=alpha) break;
        }
        return maxValue;
    } else {
        int minValue=INT_MAX;
        for (auto& [x,y]:moves) {
            if (!g.validPosition(x,y,opponent)) continue;
            g.set(x,y,opponent);
            if (g.Win(x,y,opponent)) {
                g.undo(x,y);
                return -(10000000+depth);
            }
            int value=alphaBeta(g,depth-1,alpha,beta,true);
            g.undo(x,y);
            minValue=std::min(minValue,value);
            beta=std::min(beta,value);
            if (beta<=alpha) break;
        }
        return minValue;
    }
}

// 构造函数（根据角色设置不同的进攻/防守策略）
inline Minimax::Minimax(const Player self_value,const int maxDepth_value,const double defendWeight_value):
    self(self_value),
    opponent(self_value==BLACK? WHITE:BLACK),
    maxDepth(maxDepth_value),
    nodeCount(0),
    defendWeight(defendWeight_value) {
    
    // 根据角色设置进攻/防守系数
    // 黑棋（先手）：更倾向进攻
    // 白棋（后手）：更倾向防守
    if (self == BLACK) {
        attackCoeff = 1.2;   // 黑棋进攻加成
        defenseCoeff = 1.0;  // 正常防守
    } else {
        attackCoeff = 1.0;   // 正常进攻
        defenseCoeff = 1.3;  // 白棋防守加成
    }
}

// 获取最优移动
inline std::pair<int,int> Minimax::getBestMove(Gomoku& g) {
    nodeCount=0;
    int bestScore=INT_MIN;
    std::pair<int,int> bestMove={-1,-1};
    auto moves=getCandidateMoves(g);

    for (auto& [x,y]:moves) {
        if (!g.validPosition(x,y,self)) continue;
        g.set(x,y,self);
        if (g.Win(x,y,self)) {
            g.undo(x,y);
            return {x,y};
        }
        int value=alphaBeta(g,maxDepth-1,INT_MIN,INT_MAX,false);
        g.undo(x,y);
        if (value>bestScore) {
            bestScore=value;
            bestMove={x,y};
        }
    }
    return bestMove;
}

#endif //GOMOKU_MINIMAX_H