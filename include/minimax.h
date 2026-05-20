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
    std::vector<std::pair<int,int>> killerMoves;

    inline int basePatternScore(const Gomoku::Pattern& pattern) const {
        switch (pattern.form()){
            case FIVE:      return 10000000;
            case OVERLINE:  return 10000000;
            case FLEX4:     return 9000000;
            case BLOCK4:    return 1500000;
            case FLEX3:     return 100000;
            case BLOCK3:    return 5000;
            case FLEX2:     return 500;
            case BLOCK2:    return 100;
            case FLEX1:     return 20;
            case BLOCK1:    return 5;
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
        int baseScore=(maxDist-dist+1)*5;
        int pieceCount=g.getCurrentCount();
        if (pieceCount<8)       baseScore*=20;
        else if (pieceCount<20) baseScore*=5;
        return baseScore;
    }

    int evaluatePiece(const Gomoku& g,int x,int y,Player p);
    int evaluateEmpty(const Gomoku& g,int x,int y);
    int evaluate(const Gomoku& g);
    std::vector<std::pair<int,int>> getCandidateMoves(const Gomoku& g);
    std::vector<std::pair<int,int>> getThreatMoves(const Gomoku& g);
    int quiescenceSearch(Gomoku& g,int alpha,int beta,bool isMaximizing,int qDepth);
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

inline std::vector<std::pair<int,int>> Minimax::getThreatMoves(const Gomoku& g) {
    int size=g.getSize();
    std::vector<bool> visited(size*size,false);
    std::vector<std::pair<int,int>> threats;
    std::vector<std::pair<int,int>> others;

    // 收集所有候选空位
    for (int x=1;x<=size;x++)
    for (int y=1;y<=size;y++)
    if (g.getColor(x,y)!=NONE)
    for (int dx=-2;dx<=2;dx++)
    for (int dy=-2;dy<=2;dy++){
        int nx=x+dx,ny=y+dy;
        if (!g.outOfRange(nx,ny)&&g.getColor(nx,ny)==NONE){
            int idx=(nx-1)*size+(ny-1);
            if (!visited[idx]){
                visited[idx]=true;
                // 评估此点的威胁等级
                int maxSelfThreat=0;
                int maxOppThreat=0;
                for (auto& [ddx,ddy]:Config::directions){
                    auto pSelf=g.analyzeForm(nx,ny,ddx,ddy,self);
                    auto pOpp=g.analyzeForm(nx,ny,ddx,ddy,opponent);
                    int selfLevel=(pSelf.isFlex4()||pSelf.isBlock4())?2:
                                  pSelf.isFlex3()?1:0;
                    int oppLevel=(pOpp.isFlex4()||pOpp.isBlock4())?2:
                                 pOpp.isFive()?3:
                                 pOpp.isFlex3()?1:0;
                    if (selfLevel>maxSelfThreat) maxSelfThreat=selfLevel;
                    if (oppLevel>maxOppThreat) maxOppThreat=oppLevel;
                }
                int totalLevel=maxSelfThreat+maxOppThreat;
                if (totalLevel>=2) threats.push_back({nx,ny});
                else if (totalLevel>=1) others.push_back({nx,ny});
            }
        }
    }

    // 威胁点在前，其他点在后，总共最多5个
    std::vector<std::pair<int,int>> result;
    for (auto& pt:threats)  { result.push_back(pt); if (result.size()>=5) return result; }
    for (auto& pt:others)   { result.push_back(pt); if (result.size()>=5) return result; }
    if (result.empty()) result.push_back({(size+1)/2,(size+1)/2});
    return result;
}

inline int Minimax::alphaBeta(Gomoku& g,int depth,int alpha,int beta,bool isMaximizing) {
    nodeCount++;
    if (depth==0||g.GameOver()){
        if (depth==0&&!g.GameOver()) return quiescenceSearch(g,alpha,beta,isMaximizing,0);
        return evaluate(g);
    }
    auto moves=getCandidateMoves(g);
    if (moves.empty()) return 0;

    // 杀手启发式：将本层杀手着法提到候选列表最前
    for (int ki=0;ki<2;ki++){
        auto& km=killerMoves[depth*2+ki];
        if (km.first!=-1){
            auto it=std::find_if(moves.begin(),moves.end(),
                [&](const auto& m){return m.first==km.first&&m.second==km.second;});
            if (it!=moves.end()&&it!=moves.begin()){
                std::rotate(moves.begin(),it,it+1);
            }
        }
    }

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
            if (beta<=alpha){
                killerMoves[depth*2+1]=killerMoves[depth*2];
                killerMoves[depth*2]={x,y};
                break;
            }
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
            if (beta<=alpha){
                killerMoves[depth*2+1]=killerMoves[depth*2];
                killerMoves[depth*2]={x,y};
                break;
            }
        }
        return minValue;
    }
}

inline int Minimax::quiescenceSearch(Gomoku& g,int alpha,int beta,bool isMaximizing,int qDepth) {
    nodeCount++;
    int standPat=evaluate(g);
    if (isMaximizing){
        if (standPat>=beta) return beta;
        if (standPat>alpha) alpha=standPat;
    } else {
        if (standPat<=alpha) return alpha;
        if (standPat<beta) beta=standPat;
    }
    if (qDepth>=6) return standPat;
    auto threats=getThreatMoves(g);
    for (auto& [x,y]:threats){
        Player p=isMaximizing?self:opponent;
        if (!g.validPosition(x,y,p)) continue;
        g.set(x,y,p);
        if (g.Win(x,y,p)){
            g.undo(x,y);
            return isMaximizing?(10000000+qDepth):-(10000000+qDepth);
        }
        int value=quiescenceSearch(g,alpha,beta,!isMaximizing,qDepth+1);
        g.undo(x,y);
        if (isMaximizing){
            if (value>alpha) alpha=value;
            if (alpha>=beta) return beta;
        } else {
            if (value<beta) beta=value;
            if (beta<=alpha) return alpha;
        }
    }
    return isMaximizing?alpha:beta;
}

inline Minimax::Minimax(const Player self_value,const int maxDepth_value,const double defendWeight_value):
    self(self_value),
    opponent(self_value==BLACK? WHITE:BLACK),
    maxDepth(maxDepth_value),
    nodeCount(0),
    defendWeight(defendWeight_value) {
    killerMoves.assign(maxDepth*2,{-1,-1});
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
