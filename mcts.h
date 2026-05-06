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
            if (visits==0) return 1e9;  // 未访问节点优先探索
            return wins/visits+exploration*sqrt(2*log(parent->visits+1)/visits);
        }
    };

    Node* select(Node* node,Gomoku& g);
    void expand(Node* node,Gomoku& g);
    int simulate(Gomoku& g,Node* node);
    void backpropagate(Node* node,int result);
    std::vector<std::pair<int,int>> getCandidateMoves(const Gomoku& g,int range,int maxCount) const;
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

// 获取候选移动（按评分排序，限制数量）
inline std::vector<std::pair<int,int>> MCTS::getCandidateMoves(const Gomoku& g,int range,int maxCount) const {
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
        return candidates;
    }

    // 快速评估并排序
    std::sort(candidates.begin(),candidates.end(),
        [&](const std::pair<int,int>& a,const std::pair<int,int>& b) {
            return quickEval(g,a.first,a.second,self)+quickEval(g,a.first,a.second,opponent) >
                   quickEval(g,b.first,b.second,self)+quickEval(g,b.first,b.second,opponent);
        });

    if ((int)candidates.size()>maxCount) candidates.resize(maxCount);
    return candidates;
}

// 选择：沿UCB最大路径走到叶节点
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

// 展开：一次展开多个子节点
inline void MCTS::expand(Node* node,Gomoku& g) {
    Player nextPlayer;
    if (node->player==NONE) {
        nextPlayer=self;
    } else {
        nextPlayer=(node->player==BLACK)? WHITE:BLACK;
    }

    auto moves=getCandidateMoves(g,2,20);  // 每层展开20个候选
    for (auto& [x,y]:moves) {
        if (!g.validPosition(x,y,nextPlayer)) continue;

        bool alreadyChild=false;
        for (auto& child:node->children) {
            if (child->x==x&&child->y==y) {
                alreadyChild=true;
                break;
            }
        }
        if (!alreadyChild) {
            node->children.push_back(std::make_unique<Node>(x,y,nextPlayer,node));
        }
    }
}

// 模拟：使用带权随机，增加搜索多样性
inline int MCTS::simulate(Gomoku& g,Node* node) {
    Player currentPlayer;
    if (node->player==NONE) {
        currentPlayer=self;
    } else {
        currentPlayer=(node->player==BLACK)? WHITE:BLACK;
    }

    // 先检查当前状态是否已经结束
    if (node->player!=NONE && g.Win(node->x,node->y,node->player)) {
        return node->player==self? 1:0;
    }
    if (g.GameOver()) return 0;

    Gomoku sim=g;
    int size=sim.getSize();
    int maxSteps=std::min(size*size,80);  // 限制模拟步数

    for (int step=0;step<maxSteps&&!sim.GameOver();step++) {
        auto moves=getCandidateMoves(sim,2,35);  // 每步35个候选
        if (moves.empty()) break;

        // 构建权重数组
        std::vector<int> weights;
        for (auto& [mx,my]:moves) {
            int s=0;
            // 攻防评分
            s+=quickEval(sim,mx,my,currentPlayer);
            Player oppPlayer=(currentPlayer==BLACK)? WHITE:BLACK;
            s+=quickEval(sim,mx,my,oppPlayer)/2;
            weights.push_back(s+1);
        }

        // 带权随机选择
        std::discrete_distribution<int> dist(weights.begin(),weights.end());
        int idx=dist(rng);
        auto [mx,my]=moves[idx];

        // 检查位置是否有效（包括禁手规则）
        if (!sim.validPosition(mx, my, currentPlayer)) {
            continue;
        }

        sim.set(mx,my,currentPlayer);

        if (sim.Win(mx,my,currentPlayer)) {
            return currentPlayer==self? 1:0;
        }

        currentPlayer=(currentPlayer==BLACK)? WHITE:BLACK;
    }

    return 0;  // 平局
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

// 获取最优移动
inline std::pair<int,int> MCTS::getBestMove(Gomoku& g) {
    nodeCount=0;
    Node root;

    // 根节点获取候选并按评分排序
    auto rootMoves=getCandidateMoves(g,3,60);  // 根节点60个候选

    // 全部展开为根节点的子节点
    for (auto& [x,y]:rootMoves) {
        if (g.validPosition(x,y,self)) {
            root.children.push_back(std::make_unique<Node>(x,y,self,&root));
        }
    }

    if (root.children.empty()) return {-1,-1};

    // MCTS主循环
    for (int i=0;i<iterations;i++) {
        Gomoku sim=g;

        // 选择
        Node* node=select(&root,sim);

        // 扩展和模拟
        if (!sim.GameOver()) {
            bool terminal=(node->player!=NONE && sim.Win(node->x,node->y,node->player));
            if (!terminal) {
                expand(node,sim);
                // 从新扩展的节点中随机选一个进行模拟
                if (!node->children.empty()) {
                    std::uniform_int_distribution<int> childDist(0,node->children.size()-1);
                    node=node->children[childDist(rng)].get();
                    sim.set(node->x,node->y,node->player);
                }
            }
        }

        // 模拟
        int result=simulate(sim,node);
        // 回传
        backpropagate(node,result);
        nodeCount++;
    }

    // 选择访问次数最多的子节点
    int bestX=-1,bestY=-1;
    int maxVisits=-1;
    double bestWinRate=-1;

    for (auto& child:root.children) {
        if (child->visits>0) {
            double winRate=child->wins/child->visits;
            // 综合考虑访问次数和胜率
            if (child->visits>maxVisits ||
                (child->visits==maxVisits && winRate>bestWinRate)) {
                maxVisits=child->visits;
                bestWinRate=winRate;
                bestX=child->x;
                bestY=child->y;
            }
        }
    }

    // 如果没有任何节点被访问过，选择评分最高的
    if (bestX==-1) {
        bestX=root.children[0]->x;
        bestY=root.children[0]->y;
    }

    return {bestX,bestY};
}

#endif //GOMOKU_MCTS_H