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
    std::vector<std::pair<int,int>> getCandidateMoves(const Gomoku& g) const;
    void addNeighbors(const Gomoku& g,int x,int y,std::vector<bool>& inSet,std::vector<std::pair<int,int>>& candidates) const;

public:
    MCTS(const Player self_value,const int iterations_value=1000,const double exploration_value=1.414);
    std::pair<int,int> getBestMove(Gomoku& g);
    void setIterations(int iter) { iterations=iter; }
    int getIterations() const { return iterations; }
    int getNodesSearched() const { return nodeCount; }
    Player getSelf() const { return self; }
};

// 添加某位置周围1格的空位到候选集
inline void MCTS::addNeighbors(const Gomoku& g,int x,int y,std::vector<bool>& inSet,std::vector<std::pair<int,int>>& candidates) const {
    int size=g.getSize();
    for (int dx=-1;dx<=1;dx++)
    for (int dy=-1;dy<=1;dy++) {
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

// 获取候选移动（展开用）
inline std::vector<std::pair<int,int>> MCTS::getCandidateMoves(const Gomoku& g) const {
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
        node=best;
    }
    return node;
}

// 展开：在叶节点下添加可行子节点
inline void MCTS::expand(Node* node,Gomoku& g) {
    Player nextPlayer;
    if (node->player==NONE) {
        nextPlayer=self;
    } else {
        nextPlayer=(node->player==BLACK)? WHITE:BLACK;
    }

    auto moves=getCandidateMoves(g);

    for (auto& [x,y]:moves) {
        if (g.validPosition(x,y,nextPlayer)) {
            node->children.push_back(std::make_unique<Node>(x,y,nextPlayer,node));
        }
    }
}

// 模拟：增量更新候选，随机落子到结束
inline int MCTS::simulate(Gomoku& g,Node* node) {
    Gomoku sim=g;
    Player currentPlayer;
    if (node->player==NONE) {
        currentPlayer=self;
    } else {
        currentPlayer=(node->player==BLACK)? WHITE:BLACK;
    }

    // 检查当前节点是否已结束
    if (node->player!=NONE&&sim.Win(node->x,node->y,node->player)) {
        return node->player==self? 1:0;
    }
    if (sim.GameOver()) return 0;

    int size=sim.getSize();
    std::vector<bool> inCandidates(size*size,false);
    std::vector<std::pair<int,int>> candidates;

    // 初始化候选集：所有已有棋子周围1格
    for (int x=1;x<=size;x++)
    for (int y=1;y<=size;y++)
    if (sim.getColor(x,y)!=NONE) {
        addNeighbors(sim,x,y,inCandidates,candidates);
    }

    if (candidates.empty()) {
        candidates.push_back({(size+1)/2,(size+1)/2});
    }

    int maxSteps=size*size;
    for (int step=0;step<maxSteps&&!sim.GameOver();step++) {
        // 过滤合法位置
        std::vector<std::pair<int,int>> legalMoves;
        for (auto& [mx,my]:candidates) {
            if (sim.validPosition(mx,my,currentPlayer)) {
                legalMoves.push_back({mx,my});
            }
        }

        if (legalMoves.empty()) break;

        // 随机选一个
        std::uniform_int_distribution<int> dist(0,legalMoves.size()-1);
        auto [mx,my]=legalMoves[dist(rng)];

        sim.set(mx,my,currentPlayer);

        // 赢棋检查
        if (sim.Win(mx,my,currentPlayer)) {
            return currentPlayer==self? 1:0;
        }

        // 从候选集移除已落子位置
        int idx=(mx-1)*size+(my-1);
        inCandidates[idx]=false;

        // 增量添加新落子周围的空位
        addNeighbors(sim,mx,my,inCandidates,candidates);

        // 清理 dead 候选（已被占用），用 swap+pop 高效删除
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

// 回传：沿路径更新访问次数和胜场
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

// 获取最优移动
inline std::pair<int,int> MCTS::getBestMove(Gomoku& g) {
    nodeCount=0;
    Node root;

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
                    std::uniform_int_distribution<int> dist(0,node->children.size()-1);
                    node=node->children[dist(rng)].get();
                    sim.set(node->x,node->y,node->player);
                }
            }
        }

        int result=simulate(sim,node);
        backpropagate(node,result);
        nodeCount++;
    }

    if (root.children.empty()) return {-1,-1};

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