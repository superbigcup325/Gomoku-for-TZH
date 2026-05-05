#ifndef OOP_BOARD_H
#define OOP_BOARD_H

#include <iostream>
#include <vector>
#include <cstdint>

namespace Config {
    static constexpr std::pair<int,int> directions[]={
        {0,1},{1,0},{1,1},{1,-1}
    };
}

enum Player {NONE,BLACK,WHITE};
enum chessFormType {OVERLINE,FIVE,FLEX4,BLOCK4,FLEX3,BLOCK3,FLEX2,BLOCK2,FLEX1,BLOCK1,FORBIDDEN,DEFAULT};

inline Player CurrentPlayer(const int currentTurn) {
    return (currentTurn&1? BLACK:WHITE);
}

class Gomoku {
public:
    struct Pattern {
        int8_t count=1;
        int8_t openEnds=0;

        inline int form() const {
            if (count>5) return OVERLINE;
            if (count==5) return FIVE;
            if (count==4) {
                if (openEnds==2) return FLEX4;
                if (openEnds==1) return BLOCK4;
            }
            if (count==3) {
                if (openEnds==2) return FLEX3;
                if (openEnds==1) return BLOCK3;
            }
            if (count==2) {
                if (openEnds==2) return FLEX2;
                if (openEnds==1) return BLOCK2;
            }
            if (count==1) {
                if (openEnds==2) return FLEX1;
                if (openEnds==1) return BLOCK1;
            }
            return DEFAULT;
        }
        inline bool isOverline() const { return count > 5; }
        inline bool isFive() const { return count == 5; }
        inline bool isFlex4() const { return count == 4 && openEnds == 2; }
        inline bool isBlock4() const { return count == 4 && openEnds == 1; }
        inline bool isFlex3() const { return count == 3 && openEnds == 2; }
        inline bool isBlock3() const { return count == 3 && openEnds == 1; }
        inline bool isActive() const { return openEnds > 0; }
    };
    Gomoku(const int size_value=38);
    void set(const int x,const int y,const Player currentPlayer); // 落子
    bool outOfRange(const int x,const int y) const; // 超出范围
    bool isForbidden(const int x,const int y,Player player) const; // 判断禁手
    bool validPosition(const int x,const int y,Player player); // 坐标是否合法
    bool Win(const int x,const int y,const Player currentPlayer) const; // 判断是否有胜利
    bool GameOver() const; // 判断是否结束 平局？
    void show() const; // 显示棋盘
    const std::vector<int8_t>& getGraph() const; // 获取棋盘状态
    inline int getSize() const { return size; }
    void undo(const int x,const int y); // 撤销
    Player getPlayer(const int x,const int y) const; // 获取指定位置的子色
    inline size_t pos(int x,int y) const noexcept { return static_cast<size_t>((x-1)*size+(y-1)); } // 位置转换
    Pattern analyzeForm(const int x,int y,int dx,int dy,Player player) const;    
private:
    std::vector<int8_t> graph;
    // 0 none, 1 black, 2 white
    int maxCount;
    int currentCount;
    int size;
    struct Formation {
        Pattern patterns[4];
        Player player;
        bool isEvaluated=false;

        inline int countForm(chessFormType targetForm) const {
            int cnt = 0;
            for (int i = 0; i < 4; i++) {
                if (patterns[i].form() == targetForm) cnt++;
            }
            return cnt;
        }
        inline int countFlex4() const {
            int cnt = 0;
            for (int i = 0; i < 4; i++) {
                if (patterns[i].isFlex4()) cnt++;
            }
            return cnt;
        } 
        inline int countBlock4() const {
            int cnt = 0;
            for (int i = 0; i < 4; i++) {
                if (patterns[i].isBlock4()) cnt++;
            }
            return cnt;
        }
        inline int countFlex3() const {
            int cnt = 0;
            for (int i = 0; i < 4; i++) {
                if (patterns[i].isFlex3()) cnt++;
            }
            return cnt;
        }
        inline bool hasFive() const {
            for (auto& pattern:patterns) {
                if (pattern.isFive() || pattern.isOverline()) return true;
            }
            return false;
        }
    };
    Gomoku::Formation analyzeAll(const int x,const int y,Player player) const;
};

Gomoku::Formation Gomoku::analyzeAll(const int x,const int y,Player player) const {
    Formation formation;
    const_cast<std::vector<int8_t>&>(graph)[pos(x,y)]=player;
    formation.player=player;


    int idx=0;
    for (auto& [dx,dy]:Config::directions) {
        formation.patterns[idx++]=analyzeForm(x,y,dx,dy,player);
    }

    formation.isEvaluated=true;
    const_cast<std::vector<int8_t>&>(graph)[pos(x,y)]=NONE;
    return formation;
}
    
Gomoku::Pattern Gomoku::analyzeForm(const int x,const int y,int dx,int dy,Player player) const {
    Pattern pattern;
    // 正向
    int nx=x+dx,ny=y+dy;
    while (!outOfRange(nx,ny)&&getPlayer(nx,ny)==player) {
        pattern.count++;
        nx+=dx,ny+=dy;
    }
    if (!outOfRange(nx,ny)&&getPlayer(nx,ny)==NONE) {
        pattern.openEnds++;
    }
    //反向
    nx=x-dx,ny=y-dy;
    while (!outOfRange(nx,ny)&&getPlayer(nx,ny)==player) {
        pattern.count++;
        nx-=dx,ny-=dy;
    }
    if (!outOfRange(nx,ny)&&getPlayer(nx,ny)==NONE) {
        pattern.openEnds++;
    }

    return pattern;
}

Gomoku::Gomoku(const int size_value):
    size(size_value),
    currentCount(0),
    maxCount(size_value*size_value) {
    graph.resize(size*size,0);
}

void Gomoku::set(const int x,const int y,const Player player) {
    if (!validPosition(x,y,player)) {
        std::cout<<"error: invalid position"<<std::endl;
        return;
    }
    graph[pos(x,y)]=player;
    currentCount++;
}

bool Gomoku::outOfRange(const int x,const int y) const {
    return (x<1||x>size||y<1||y>size);
}

bool Gomoku::isForbidden(const int x,const int y,Player player) const {
    if (player!=BLACK) return false;

    Formation formation=analyzeAll(x,y,player);

    // 长连
    for (auto& pattern:formation.patterns) {
        if (pattern.isOverline()) return true;
    }

    // 是否有五连
    if (formation.hasFive()) return false;

    int flex4=formation.countFlex4();
    int block4=formation.countBlock4();
    int flex3=formation.countFlex3();
    // 双四
    if (flex4>=2||flex4+block4>=2) return true;
    // 双活三
    if (flex3>=2) return true;
    return false;
}

bool Gomoku::validPosition(const int x,const int y,Player player) {
    // 是否出界
    if (outOfRange(x,y)) return false;
    // 是否下过
    if (graph[pos(x,y)]!=0) return false;
    // 是否禁手
    if (isForbidden(x,y,player)) return false;
    return true;
}

bool Gomoku::Win(const int x,const int y,const Player currentPlayer) const {
    for (auto& [dx,dy]:Config::directions) {
        Pattern pattern;
        int nx=x+dx,ny=y+dy;
        while (!outOfRange(nx,ny)&&graph[pos(nx,ny)]==currentPlayer) {
            pattern.count++;
            nx+=dx,ny+=dy;
        }
        nx=x-dx,ny=y-dy;
        while (!outOfRange(nx,ny)&&graph[pos(nx,ny)]==currentPlayer) {
            pattern.count++;
            nx-=dx,ny-=dy;
        }
        if (pattern.form()==FIVE||pattern.form()==OVERLINE) return true;
    }
    return false;
}

bool Gomoku::GameOver() const {
    return currentCount>=maxCount;
}

void Gomoku::show() const {
    std::cout<<"   ";
    for (int j=1;j<=size;j++) {
        std::cout.width(2);
        std::cout<<j<<" ";
    }
    std::cout<<"\n";

    for (int i=1;i<=size;i++) {
        std::cout.width(2);
        std::cout<<i<<" ";
        for (int j=1;j<=size;j++) {
            if (graph[pos(i,j)]==BLACK) std::cout<<" ● ";
            else if (graph[pos(i,j)]==WHITE) std::cout<<" ○ ";
            else std::cout<<" · ";
        }
        std::cout<<"\n";
    }
}

const std::vector<int8_t>& Gomoku::getGraph() const {
    return graph;
}

void Gomoku::undo(const int x,const int y) {
    if (outOfRange(x,y)) return;
    if (currentCount==0) return;
    if (graph[pos(x,y)]==NONE) return;
    currentCount--;
    graph[pos(x,y)]=0;
}

Player Gomoku::getPlayer(const int x,const int y) const {
    if (outOfRange(x,y)) return NONE;
    return graph[pos(x,y)]==BLACK? BLACK:graph[pos(x,y)]==WHITE? WHITE:NONE;
}

#endif //OOP_BOARD_H
