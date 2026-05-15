#ifndef GOMOKU_MINIMAX_H
#define GOMOKU_MINIMAX_H

#include"board.h"
#include<vector>
#include<algorithm>
#include<climits>
#include<cmath>
#include<set>

class Minimax {
private:
    Player self;
    Player opponent;
    int maxDepth;
    int nodeCount;
    const double defendWeight;
    double attackCoeff;
    double defenseCoeff;

    inline int basePatternScore(const Gomoku::Pattern& pattern) const {
        switch (pattern.form()){
            case FIVE:      return 10000000;
            case OVERLINE:  return 10000000;
            case FLEX4:     return 5000000;
            case BLOCK4:    return 3000000;
            case FLEX3:     return 50000;
            case BLOCK3:    return 1000;
            case FLEX2:     return 200;
            case BLOCK2:    return 50;
            case FLEX1:     return 10;
            case BLOCK1:    return 2;
            default:        return 0;
        }
    }

    inline int positionScore(const Gomoku& g,int x,int y) const {
        int size=g.getSize();
        int center=(size+1)/2;
        int distX=std::abs(x-center);
        int distY=std::abs(y-center);
        int dist=std::max(distX,distY);
        int maxDist=size/2;
        if (dist>=maxDist) return 1;
        return (maxDist-dist+1)*5;
    }

    int evaluatePiece(const Gomoku& g,int x,int y,Player p);
    int evaluateEmpty(const Gomoku& g,int x,int y);
    int evaluate(const Gomoku& g);
    std::vector<std::pair<int,int>> getCandidateMoves(const Gomoku& g);
    int alphaBeta(Gomoku& g,int depth,int alpha,int beta,bool isMaximizing);

public:
    Minimax(const Player self_value,const int maxDepth_value,const double defendWeight_value=2.0);
    std::pair<int,int> getBestMove(Gomoku& g);
    void setDepth(int depth_value) { maxDepth=depth_value; }
    int getNodesSearched() const { return nodeCount; }
    Player getSelf() const { return self; }
};

inline int Minimax::evaluatePiece(const Gomoku& g,int x,int y,Player player) {
    if (g.getColor(x,y)!=player) return 0;
    int score=0;
    for (auto& [dx,dy]:Config::directions){
        Gomoku::Pattern pattern=g.analyzeForm(x,y,dx,dy,player);
        score+=basePatternScore(pattern);
    }
    
    score+=positionScore(g,x,y);
    return score;
}

inline int Minimax::evaluateEmpty(const Gomoku& g,int x,int y) {
    if (g.getColor(x,y)!=NONE) return 0;
    
    int score=0;
    for (auto& [dx,dy]:Config::directions){
        auto patternSelf=g.analyzeForm(x,y,dx,dy,self);
        auto patternOpponent=g.analyzeForm(x,y,dx,dy,opponent);
        
        score+=basePatternScore(patternSelf);
        score+=basePatternScore(patternOpponent);
    }
    
    score+=positionScore(g,x,y);
    return score;
}

inline int Minimax::evaluate(const Gomoku& g) {
    int selfScore=0;
    int opponentScore=0;
    const int size=g.getSize();
    std::vector<bool> evaluated(size*size,false);
    
    for (int x=1;x<=size;x++){
        for (int y=1;y<=size;y++){
            Player p=g.getColor(x,y);
            if (p==NONE) continue;
            int idx=(x-1)*size+(y-1);
            if (evaluated[idx]) continue;
            if (p==self){
                int pieceScore=evaluatePiece(g,x,y,self);
                selfScore+=static_cast<int>(pieceScore*attackCoeff);
            } else if (p==opponent){
                int pieceScore=evaluatePiece(g,x,y,opponent);
                opponentScore+=static_cast<int>(pieceScore*defenseCoeff);
            }
            evaluated[idx]=true;
            for (auto& [dx,dy]:Config::directions){
                int nx=x+dx,ny=y+dy;
                while (!g.outOfRange(nx,ny)&&g.getColor(nx,ny)==p){
                    evaluated[(nx-1)*size+(ny-1)]=true;
                    nx+=dx;
                    ny+=dy;
                }
                nx=x-dx,ny=y-dy;
                while (!g.outOfRange(nx,ny)&&g.getColor(nx,ny)==p){
                    evaluated[(nx-1)*size+(ny-1)]=true;
                    nx-=dx;
                    ny-=dy;
                }
            }
        }
    }
    double val=static_cast<double>(selfScore)-static_cast<double>(opponentScore)*defendWeight;
    return static_cast<int>(val);
}

inline std::vector<std::pair<int,int>> Minimax::getCandidateMoves(const Gomoku& g) {
    int size=g.getSize();
    std::vector<bool> visited(size*size,false);
    std::vector<std::pair<int,int>> candidates;
    for (int x=1;x<=size;x++)
    for (int y=1;y<=size;y++)
    if (g.getColor(x,y)!=NONE)
    for (int dx=-2;dx<=2;dx++)
    for (int dy=-2;dy<=2;dy++){
        int nx=x+dx,ny=y+dy;
        if (!g.outOfRange(nx,ny)&&g.getColor(nx,ny)==NONE){
            int idx=(nx-1)*size+(ny-1);
            if (!visited[idx]){
                candidates.push_back({nx,ny});
                visited[idx]=true;
            }
        }
    }
    if (candidates.empty()){
        candidates.push_back({(size+1)/2,(size+1)/2});
        return candidates;
    }

    // 必防点检测：对手在此落子是否立即获胜（五连）
    std::vector<std::pair<int,int>> mustDefendPoints;
    for (const auto& [x,y]:candidates){
        if (g.getColor(x,y)==NONE){
            // 检查4个方向：如果对手在此落子是否形成五连
            for (auto& [dx,dy]:Config::directions){
                int count=1;
                int nx=x+dx,ny=y+dy;
                while (!g.outOfRange(nx,ny)&&g.getColor(nx,ny)==opponent){count++;nx+=dx;ny+=dy;}
                nx=x-dx;ny=y-dy;
                while (!g.outOfRange(nx,ny)&&g.getColor(nx,ny)==opponent){count++;nx-=dx;ny-=dy;}
                if (count>=5){
                    mustDefendPoints.push_back({x,y});
                    break;
                }
            }
        }
    }

    // 同时检测活四威胁：对手在此落子形成活四（下一步必胜）
    for (const auto& [x,y]:candidates){
        bool alreadyMustDefend=false;
        for (const auto& pt:mustDefendPoints){if(pt.first==x&&pt.second==y){alreadyMustDefend=true;break;}}
        if (alreadyMustDefend) continue;
        
        for (auto& [dx,dy]:Config::directions){
            auto patternOpponent=g.analyzeForm(x,y,dx,dy,opponent);
            if (patternOpponent.isFlex4()){
                mustDefendPoints.push_back({x,y});
                break;
            }
        }
    }

    std::sort(candidates.begin(),candidates.end(),
        [&](const std::pair<int,int>& a,const std::pair<int,int>& b){
            return evaluateEmpty(g,a.first,a.second)>evaluateEmpty(g,b.first,b.second);
        });

    std::vector<std::pair<int,int>> result;
    std::set<std::pair<int,int>> added;
    for (const auto& pt:mustDefendPoints){
        if (added.find(pt)==added.end()){
            result.push_back(pt);
            added.insert(pt);
        }
    }
    for (const auto& pt:candidates){
        if (added.find(pt)==added.end()){
            result.push_back(pt);
            added.insert(pt);
        }
        if (result.size()>=20) break;
    }
    return result;
}

inline int Minimax::alphaBeta(Gomoku& g,int depth,int alpha,int beta,bool isMaximizing) {
    nodeCount++;
    if (depth==0||g.GameOver()) return evaluate(g);
    auto moves=getCandidateMoves(g);
    if (moves.empty()) return 0;
    if (isMaximizing){
        int maxValue=INT_MIN;
        for (auto& [x,y]:moves){
            if (!g.validPosition(x,y,self)) continue;
            g.set(x,y,self);
            if (g.Win(x,y,self)){
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
        for (auto& [x,y]:moves){
            if (!g.validPosition(x,y,opponent)) continue;
            g.set(x,y,opponent);
            if (g.Win(x,y,opponent)){
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

inline Minimax::Minimax(const Player self_value,const int maxDepth_value,const double defendWeight_value):
    self(self_value),
    opponent(self_value==BLACK? WHITE:BLACK),
    maxDepth(maxDepth_value),
    nodeCount(0),
    defendWeight(defendWeight_value) {
    if (self==BLACK){
        attackCoeff=1.2;
        defenseCoeff=1.0;
    } else {
        attackCoeff=1.0;
        defenseCoeff=1.3;
    }
}

inline std::pair<int,int> Minimax::getBestMove(Gomoku& g) {
    nodeCount=0;
    int bestScore=INT_MIN;
    std::pair<int,int> bestMove={-1,-1};
    auto moves=getCandidateMoves(g);
    for (auto& [x,y]:moves){
        if (!g.validPosition(x,y,self)) continue;
        g.set(x,y,self);
        if (g.Win(x,y,self)){
            g.undo(x,y);
            return {x,y};
        }
        int value=alphaBeta(g,maxDepth-1,INT_MIN,INT_MAX,false);
        g.undo(x,y);
        if (value>bestScore){
            bestScore=value;
            bestMove={x,y};
        }
    }
    return bestMove;
}

#endif
