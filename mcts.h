#ifndef GOMOKU_MCTS_H
#define GOMOKU_MCTS_H

#include "board.h"
#include <vector>
#include <cmath>
#include <random>
#include <memory>
#include <algorithm>

class MCTS {
private:
    Player self;
    Player opponent;
    int iterations;
    double exploration;
    int nodeCount;
    std::mt19937 rng;

    struct Node {
        int x,y;
        Player player;
        int visits;
        double wins;
        std::vector<std::unique_ptr<Node>> children;
        Node* parent;

        Node(int x_=-1,int y_=-1,Player p=NONE,Node* parent_=nullptr):
            x(x_),y(y_),player(p),visits(0),wins(0),parent(parent_) {}

        inline bool isLeaf() const { return children.empty(); }

        inline double ucb(double exploration) const {
            if (visits==0) return std::numeric_limits<double>::max();
            return wins/visits+exploration*sqrt(2*log(parent->visits)/visits);
        }
    };

    Node* select(Node* node,Gomoku& g);
    void expand(Node* node,Gomoku& g);
    int simulate(Gomoku& g,Node* node);
    void backpropagate(Node* node,int result);
    std::vector<std::pair<int,int>> getCandidateMoves(const Gomoku& g,int range=2) const;
    void addNeighbors(const Gomoku& g,int x,int y,std::vector<bool>& inSet,std::vector<std::pair<int,int>>& candidates) const;
    int quickEval(const Gomoku& g,int x,int y,Player player) const;

public:
    MCTS(const Player self_value,const int iterations_value=1000,const double exploration_value=1.414);
    std::pair<int,int> getBestMove(Gomoku& g);
    void setIterations(int iter) { iterations=iter; }
    int getIterations() const { return iterations; }
    int getNodesSearched() const { return nodeCount; }
    Player getSelf() const { return self; }
};

// 快速评估函数
inline int MCTS::quickEval(const Gomoku& g,int x,int y,Player player) const {
    int score=0;
    Player opp=(player==BLACK)? WHITE:BLACK;
    for (auto& [dx,dy]:Config::directions) {
        Gomoku::Pattern p=g.analyzeForm(x,y,dx,dy,player);
        Gomoku::Pattern oppP=g.analyzeForm(x,y,dx,dy,opp);

        if (p.count>=5) score+=100000;
        else if (p.count==4&&p.openEnds==2) score+=10000;
        else if (p.count==4&&p.openEnds==1) score+=1000;
        else if (p.count==3&&p.openEnds==2) score+=500;
        else if (p.count==3&&p.openEnds==1) score+=100;
        else if (p.count==2&&p.openEnds==2) score+=50;
        else if (p.count==2&&p.openEnds==1) score+=10;
        else if (p.count==1&&p.openEnds==2) score+=5;

        if (oppP.count>=5) score+=50000;
        else if (oppP.count==4&&oppP.openEnds==2) score+=5000;
        else if (oppP.count==4&&oppP.openEnds==1) score+=800;
        else if (oppP.count==3&&oppP.openEnds==2) score+=300;
        else if (oppP.count==3&&oppP.openEnds==1) score+=50;
    }
    return score;
}

// 添加某位置周围1格的空位到候选集
inline void MCTS::addNeighbors(const Gomoku& g,int x,int y,std::vector<bool>& inSet,std::vector<std::pair<int,int>>& candidates) const {
    int size=g.getSize();
    for (int dx=-1;dx<=1;dx++)
    for (int dy=-1;dy<=1;dy++) {
        if (dx==0&&dy==0) continue;
        int nx=x+dx,ny=y+dy;
        if (g.outOfRange(nx,ny)) continue;
        if (g.getColor(nx,ny)!=NONE) continue;
        int idx=(nx-1)*size+(ny-1);
        if (!inSet[idx]) {
            inSet[idx]=true;
            candidates.push_back({nx,ny});
        }
    }
}

// 获取候选移动（range=2:周围2格，range=3:周围3格）
inline std::vector<std::pair<int,int>> MCTS::getCandidateMoves(const Gomoku& g,int range) const {
    int size=g.getSize();
    std::vector<bool> visited(size*size,false);
    std::vector<std::pair<int,int>> candidates;

    for (int x=1;x<=size;x++)
    for (int y=1;y<=size;y++)
    if (g.getColor(x,y)!=NONE)
    for (int dx=-range;dx<=range;dx++)
    for (int dy=-range;dy<=range;dy++) {
        if (dx==0&&dy==0) continue;
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
    }

    return candidates;
}

// 选择：沿 UCB 最大路径走到叶节点
inline MCTS::Node* MCTS::select(Node* node,Gomoku& g) {
    while (!node->isLeaf()) {
        Node* best=nullptr;
        double bestUCB=-1;

        for (auto& child:node->children) {
            double ucb=child->ucb(exploration);
            if (ucb>bestUCB) {
                bestUCB=ucb;
                best=child.get();
            }
        }

        if (best==nullptr) return node;
        g.set(best->x,best->y,best->player);

        if (g.Win(best->x,best->y,best->player)) {
            return best;
        }
        node=best;
    }
    return node;
}

// 展开：只加一个最好的未访问子节点
inline void MCTS::expand(Node* node,Gomoku& g) {
    Player nextPlayer;
    if (node->player==NONE) {
        nextPlayer=self;
    } else {
        nextPlayer=(node->player==BLACK)? WHITE:BLACK;
    }

    int bestScore=-1;
    int bestX=-1,bestY=-1;

    auto moves=getCandidateMoves(g,2);
    for (auto& [x,y]:moves) {
        if (!g.validPosition(x,y,nextPlayer)) continue;

        bool alreadyChild=false;
        for (auto& child:node->children) {
            if (child->x==x&&child->y==y) {
                alreadyChild=true;
                break;
            }
        }
        if (alreadyChild) continue;

        int s=quickEval(g,x,y,nextPlayer);
        if (s>bestScore) {
            bestScore=s;
            bestX=x;
            bestY=y;
        }
    }

    if (bestX!=-1) {
        node->children.push_back(std::make_unique<Node>(bestX,bestY,nextPlayer,node));
    }
}

// 模拟：启发式引导的随机走子，扩大搜索范围
inline int MCTS::simulate(Gomoku& g,Node* node) {
    Gomoku sim=g;
    Player currentPlayer;
    if (node->player==NONE) {
        currentPlayer=self;
    } else {
        currentPlayer=(node->player==BLACK)? WHITE:BLACK;
    }

    if (node->player!=NONE&&sim.Win(node->x,node->y,node->player)) {
        return node->player==self? 1:0;
    }
    if (sim.GameOver()) return 0;

    int size=sim.getSize();
    std::vector<bool> inCandidates(size*size,false);
    std::vector<std::pair<int,int>> candidates;

    // 初始化：周围2格
    for (int x=1;x<=size;x++)
    for (int y=1;y<=size;y++)
    if (sim.getColor(x,y)!=NONE) {
        for (int dx=-2;dx<=2;dx++)
        for (int dy=-2;dy<=2;dy++) {
            if (dx==0&&dy==0) continue;
            int nx=x+dx,ny=y+dy;
            if (sim.outOfRange(nx,ny)) continue;
            if (sim.getColor(nx,ny)!=NONE) continue;
            int idx=(nx-1)*size+(ny-1);
            if (!inCandidates[idx]) {
                inCandidates[idx]=true;
                candidates.push_back({nx,ny});
            }
        }
    }

    if (candidates.empty()) {
        candidates.push_back({(size+1)/2,(size+1)/2});
    }

    int maxSteps=std::min(size*size,120);
    for (int step=0;step<maxSteps&&!sim.GameOver();step++) {
        std::vector<std::pair<int,int>> legalMoves;
        std::vector<int> weights;

        for (auto& [mx,my]:candidates) {
            if (sim.validPosition(mx,my,currentPlayer)) {
                legalMoves.push_back({mx,my});
                int s=quickEval(sim,mx,my,currentPlayer);
                weights.push_back(s+1);
            }
        }

        if (legalMoves.empty()) break;

        std::discrete_distribution<int> dist(weights.begin(),weights.end());
        int idx=dist(rng);
        auto [mx,my]=legalMoves[idx];

        sim.set(mx,my,currentPlayer);

        if (sim.Win(mx,my,currentPlayer)) {
            return currentPlayer==self? 1:0;
        }

        int posIdx=(mx-1)*size+(my-1);
        inCandidates[posIdx]=false;

        // 添加新落子周围2格的空位
        for (int dx=-2;dx<=2;dx++)
        for (int dy=-2;dy<=2;dy++) {
            if (dx==0&&dy==0) continue;
            int nx=mx+dx,ny=my+dy;
            if (sim.outOfRange(nx,ny)) continue;
            if (sim.getColor(nx,ny)!=NONE) continue;
            int nIdx=(nx-1)*size+(ny-1);
            if (!inCandidates[nIdx]) {
                inCandidates[nIdx]=true;
                candidates.push_back({nx,ny});
            }
        }

        candidates.erase(
            std::remove_if(candidates.begin(),candidates.end(),
                [&](const std::pair<int,int>& pos) {
                    return !inCandidates[(pos.first-1)*size+(pos.second-1)];
                }),
            candidates.end()
        );

        currentPlayer=(currentPlayer==BLACK)? WHITE:BLACK;
    }

    return 0;
}

// 回传
inline void MCTS::backpropagate(Node* node,int result) {
    while (node!=nullptr) {
        node->visits++;
        if (node->player==self) {
            node->wins+=result;
        }
        else if (node->player==opponent) {
            node->wins+=(1-result);
        }
        node=node->parent;
    }
}

// 构造函数
inline MCTS::MCTS(const Player self_value,const int iterations_value,const double exploration_value):
    self(self_value),
    opponent(self_value==BLACK? WHITE:BLACK),
    iterations(iterations_value),
    exploration(exploration_value),
    nodeCount(0),
    rng(std::random_device{}()) {}

// 获取最优移动：根节点展开所有候选
inline std::pair<int,int> MCTS::getBestMove(Gomoku& g) {
    nodeCount=0;
    Node root;

    // 根节点获取候选并全部展开
    auto rootMoves=getCandidateMoves(g,3);

    // 按评分排序
    std::vector<std::pair<int,std::pair<int,int>>> scored;
    for (auto& [x,y]:rootMoves) {
        if (g.validPosition(x,y,self)) {
            int s=quickEval(g,x,y,self);
            scored.push_back({s,{x,y}});
        }
    }
    std::sort(scored.begin(),scored.end(),std::greater<>());

    // 取前50个候选
    if (scored.size()>50) scored.resize(50);

    for (auto& [score,pos]:scored) {
        root.children.push_back(std::make_unique<Node>(pos.first,pos.second,self,&root));
    }

    // 如果没有候选，返回-1
    if (root.children.empty()) return {-1,-1};

    for (int i=0;i<iterations;i++) {
        Gomoku sim=g;
        Node* node=select(&root,sim);

        if (!sim.GameOver()) {
            bool terminal=false;
            if (node->player!=NONE) {
                terminal=sim.Win(node->x,node->y,node->player);
            }
            if (!terminal) {
                expand(node,sim);
                if (!node->children.empty()) {
                    node=node->children.back().get();
                    sim.set(node->x,node->y,node->player);
                }
            }
        }

        int result=simulate(sim,node);
        backpropagate(node,result);
        nodeCount++;
    }

    int bestX=-1,bestY=-1;
    int maxVisits=-1;
    for (auto& child:root.children) {
        if (child->visits>maxVisits) {
            maxVisits=child->visits;
            bestX=child->x;
            bestY=child->y;
        }
    }
    return {bestX,bestY};
}

#endif //GOMOKU_MCTS_H