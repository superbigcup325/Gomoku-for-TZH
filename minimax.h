#ifndef GO_MINIMAX_H
#define GO_MINIMAX_H

#include "board.h"
#include <vector>
#include <algorithm>
#include <climits>

class Minimax {
private:
    Player self;
    Player opponent;
    const double defendWeight=1.1;
    int depth;
    int nodeCount;

    inline int patternScore(const Gomoku::Pattern& pattern) const {
        switch (pattern.form()) {
            case FIVE:      return 1000000;
            case OVERLINE:  return 1000000;
            case FLEX4:     return 100000;
            case BLOCK4:    return 15000;
            case FLEX3:     return 5000;
            case BLOCK3:    return 800;
            case FLEX2:     return 500;
            case BLOCK2:    return 50;
            case FLEX1:     return 30;
            case BLOCK1:    return 5;
            default:        return 0;
        }
    }

    int evaluatePiece(const Gomoku& g,int x,int y,Player p) const;
    int evaluateEmpty(const Gomoku& g,int x,int y) const;
    int evaluate(const Gomoku& g) const;
    std::vector<std::pair<int,int>> getCandidateMoves(const Gomoku& g) const;
    int alphaBeta(Gomoku& g,int depth,int alpha,int beta,bool isMaximizing);
public:
    Minimax(Player self_value,int depth_value,double defendWeight_value=1.1);
    std::pair<int,int> getBestMove(Gomoku& g);
    void setDepth(int depth_value) { depth = depth_value; }
    int getNodesSearched() const { return nodeCount; }
    Player getSelf() const { return self; }
};

int Minimax::evaluatePiece(const Gomoku& g, int x, int y, Player player) const {
    if (g.getPlayer(x,y)!=player) return 0;
    int score=0;
    for (auto&[dx,dy]:Config::directions) {
        int bx=x-dx,by=y-dy;
        if (!g.outOfRange(bx,by)&&g.getPlayer(bx,by)==player) continue;
        Gomoku::Pattern pattern=g.analyzeForm(x,y,dx,dy,player);
        score+=patternScore(pattern);
    }
    return score;
}

int Minimax::evaluateEmpty(const Gomoku& g,int x,int y) const {
    if (g.getPlayer(x,y)!=NONE) return 0;
    int score=0;

    for (auto& [dx,dy]:Config::directions) {
        score+=patternScore(g.analyzeForm(x,y,dx,dy,BLACK));
        score+=patternScore(g.analyzeForm(x,y,dx,dy,WHITE));
    }
    return score;
}

int Minimax::evaluate(const Gomoku& g) const {
    int selfScore=0;
    int opponentScore=0;
    int size=g.getSize();

    for (int x=1;x<=size;x++) {
        for (int y=1;y<=size;y++) {
            Player p=g.getPlayer(x,y);
            if (p==self) {
                selfScore+=evaluatePiece(g,x,y,self);
            }
            else if (p==opponent) {
                opponentScore+=evaluatePiece(g,x,y,opponent);
            }
        }
    }

    double val = static_cast<double>(selfScore) - static_cast<double>(opponentScore) * defendWeight;
    return static_cast<int>(val);
}

std::vector<std::pair<int,int>> Minimax::getCandidateMoves(const Gomoku& g) const {
    int size=g.getSize();
    std::vector<bool> visited(size*size,false);
    std::vector<std::pair<int,int>> candidates;

    // 收集所有已有棋子周围2格内的空位
    for (int x=1;x<=size;x++)
    for (int y=1;y<=size;y++)
    if (g.getPlayer(x,y)!=NONE)
    for (int dx=-2;dx<=2;dx++)
    for (int dy=-2;dy<=2;dy++) {
        int nx=x+dx,ny=y+dy;
        if (!g.outOfRange(nx,ny) && g.getPlayer(nx,ny)==NONE) {
            int idx=(nx-1)*size+(ny-1);
            if (!visited[idx]) {
                candidates.push_back({nx,ny});
                visited[idx]=true;
            }
        }
    }

    // 如果棋盘为空，返回中心
    if (candidates.empty()) {
        candidates.push_back({(size+1)/2, (size+1)/2});
        return candidates;
    }

    std::sort(candidates.begin(),candidates.end(),
        [&](const std::pair<int,int>& a,const std::pair<int,int>& b) {
            return evaluateEmpty(g,a.first,a.second)>evaluateEmpty(g,b.first,b.second);
        });
    
    // 限制数量
    if (candidates.size()>15){
        candidates.resize(15);   
    }

    return candidates;
}

int Minimax::alphaBeta(Gomoku& g,int depth,int alpha,int beta,bool isMaximizing) {
    nodeCount++;

    if (depth==0||g.GameOver()) return evaluate(g);

    auto moves=getCandidateMoves(g);
    if (moves.empty()) return 0;

    if (isMaximizing){
        int maxValue=INT_MIN;
        for (auto& [x,y]:moves) {
            if (!g.validPosition(x,y,self)) continue;
            g.set(x,y,self);
            if (g.Win(x,y,self)) {
                g.undo(x,y);
                return 1000000+depth;
            }
            int value=alphaBeta(g,depth-1,alpha,beta,!isMaximizing);
            g.undo(x,y);
            maxValue=std::max(maxValue,value);
            alpha=std::max(alpha,value);
            if (beta<=alpha) break;
        }
        return maxValue;
    }
    else {
        int minValue=INT_MAX;
        for (auto& [x,y]:moves) {
            if (!g.validPosition(x,y,opponent)) continue;
            g.set(x,y,opponent);
            if (g.Win(x,y,opponent)) {
                g.undo(x,y);
                return -(1000000+depth);
            }
            int value=alphaBeta(g,depth-1,alpha,beta,!isMaximizing);
            g.undo(x,y);
            minValue=std::min(minValue,value);
            beta=std::min(beta,value);
            if (beta<=alpha) break;
        }
        return minValue;
    }
}

Minimax::Minimax(Player self_value,int depth_value,double defendWeight_value):
    self(self_value),
    opponent(self_value==BLACK? WHITE:BLACK),
    depth(depth_value),
    nodeCount(0),
    defendWeight(defendWeight_value) {}

std::pair<int,int> Minimax::getBestMove(Gomoku& g) {
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
        int value=alphaBeta(g,depth-1,INT_MIN,INT_MAX,false);
        g.undo(x,y);
        if (value>bestScore) {
            bestScore=value;
            bestMove={x,y};
        }
    }
    return bestMove;
}



#endif //GO_MINIMAX_H