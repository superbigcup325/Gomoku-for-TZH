#ifndef OOP_BOARD_H
#define OOP_BOARD_H

#include<iostream>
#include<vector>
#include<cstdint>
#include"pattern.h"

namespace Config {
    static constexpr std::pair<int,int> directions[4]={
        {0,1},{1,0},{1,1},{1,-1}
    };
}

inline Player CurrentPlayer(const int currentTurn) {
    return (currentTurn&1? BLACK:WHITE);
}

class Gomoku {
public:
    struct Pattern {
        int8_t count=1;
        int8_t openEnds=0;
        int8_t gapSegments=0;
        int8_t gapSpaces=0;
        PatternType type=DEFAULT;
        inline int form() const { return type; }
        inline bool isOverline() const { return type==OVERLINE; }
        inline bool isFive() const { return type==FIVE; }
        inline bool isFlex4() const { return type==FLEX4; }
        inline bool isBlock4() const { return type==BLOCK4; }
        inline bool isFlex3() const { return type==FLEX3; }
        inline bool isBlock3() const { return type==BLOCK3; }
        inline bool isFlex2() const { return type==FLEX2; }
        inline bool isBlock2() const { return type==BLOCK2; }
        inline bool isActive() const { return openEnds>0; }
        inline bool hasGap() const { return gapSegments>0; }
        inline bool isJumpFlex2() const { return type==FLEX2&&gapSegments>=1; }
        inline bool isJumpFlex3() const { return type==FLEX3&&gapSegments>=1; }
        inline bool isJumpBlock4() const { return type==BLOCK4&&gapSegments>=1; }
    };
    Gomoku(const int size_val=38);
    void set(const int x,const int y,const Player currentPlayer);
    bool outOfRange(const int x,const int y) const;
    bool isForbidden(const int x,const int y,Player player);
    bool validPosition(const int x,const int y,Player player);
    bool Win(const int x,const int y,const Player currentPlayer) const;
    bool GameOver() const;
    void show() const;
    const std::vector<int8_t>& getGraph() const;
    inline int getSize() const { return size; }
    inline int getCurrentCount() const { return currentCount; }
    void undo(const int x,const int y);
    Player getColor(const int x,const int y) const;
    inline size_t pos(int x,int y) const noexcept { return static_cast<size_t>((x-1)*size+(y-1)); }
    Pattern analyzeForm(const int x,const int y,int dx,int dy,Player player) const;
private:
    std::vector<int8_t> graph; // 0:none 1:black 2:white
    int size;
    int currentCount;
    int maxCount;
    struct Formation {
        Pattern patterns[4];
        Player player;
        bool isEvaluated=false;
        inline int countForm(PatternType targetForm) const {
            int cnt=0;
            for (int i=0;i<4;i++){
                if (patterns[i].form()==targetForm) cnt++;
            }
            return cnt;
        }
        inline int countFlex4() const {
            int cnt=0;
            for (int i=0;i<4;i++){
                if (patterns[i].isFlex4()) cnt++;
            }
            return cnt;
        }
        inline int countBlock4() const {
            int cnt=0;
            for (int i=0;i<4;i++){
                if (patterns[i].isBlock4()) cnt++;
            }
            return cnt;
        }
        inline int countFlex3() const {
            int cnt=0;
            for (int i=0;i<4;i++){
                if (patterns[i].isFlex3()) cnt++;
            }
            return cnt;
        }
        inline bool hasFive() const {
            for (auto& pattern:patterns){
                if (pattern.isFive()) return true;
            }
            return false;
        }
    };
    inline Gomoku::Formation analyzeAll(const int x,const int y,Player player);
};

inline Gomoku::Formation Gomoku::analyzeAll(const int x,const int y,Player player) {
    Formation formation;
    graph[pos(x,y)]=player;
    formation.player=player;
    int idx=0;
    for (auto& [dx,dy]:Config::directions){
        formation.patterns[idx++]=analyzeForm(x,y,dx,dy,player);
    }
    formation.isEvaluated=true;
    graph[pos(x,y)]=NONE;
    return formation;
}

inline Gomoku::Pattern Gomoku::analyzeForm(const int x,const int y,int dx,int dy,Player player) const {
    Pattern pattern;
    int line[PATTERN_WINDOW]={0};
    line[PATTERN_CENTER]=static_cast<int>(player);
    static constexpr int BLOCK_MARKER=3;
    for (int step=1;step<=PATTERN_RADIUS;step++){
        int nx=x+dx*step,ny=y+dy*step;
        if (!outOfRange(nx,ny)){
            Player color=getColor(nx,ny);
            if (color==player) line[PATTERN_CENTER+step]=static_cast<int>(player);
            else if (color==NONE) line[PATTERN_CENTER+step]=0;
            else line[PATTERN_CENTER+step]=BLOCK_MARKER;
        } else {
            line[PATTERN_CENTER+step]=BLOCK_MARKER;
        }
        int px=x-dx*step,py=y-dy*step;
        if (!outOfRange(px,py)){
            Player color=getColor(px,py);
            if (color==player) line[PATTERN_CENTER-step]=static_cast<int>(player);
            else if (color==NONE) line[PATTERN_CENTER-step]=0;
            else line[PATTERN_CENTER-step]=BLOCK_MARKER;
        } else {
            line[PATTERN_CENTER-step]=BLOCK_MARKER;
        }
    }
    uint32_t key=PatternDB::instance().encodeLine(line,static_cast<int>(player));
    const PatternInfo* info=PatternDB::instance().lookup(key);
    if (info){
        pattern.count=info->count;
        pattern.openEnds=info->openEnds;
        pattern.gapSegments=info->gapSegments;
        pattern.gapSpaces=info->gapSpaces;
        pattern.type=info->type;
    } else {
        pattern.count=1;
        pattern.openEnds=0;
        pattern.gapSegments=0;
        pattern.gapSpaces=0;
        pattern.type=DEFAULT;
    }
    return pattern;
}

inline Gomoku::Gomoku(const int size_val):
    size(size_val),
    currentCount(0),
    maxCount(size_val*size_val) {
    graph.resize(size*size,0);
}

inline void Gomoku::set(const int x,const int y,const Player player) {
    if (!validPosition(x,y,player)){
        std::cout<<"error: invalid position"<<std::endl;
        return;
    }
    graph[pos(x,y)]=player;
    currentCount++;
}

inline bool Gomoku::outOfRange(const int x,const int y) const {
    return (x<1||x>size||y<1||y>size);
}

inline bool Gomoku::isForbidden(const int x,const int y,Player player) {
    if (player!=BLACK) return false;
    Formation formation=analyzeAll(x,y,player);
    if (formation.hasFive()) return false;
    for (auto& pattern:formation.patterns){
        if (pattern.isOverline()) return true;
    }
    int flex4=formation.countFlex4();
    int block4=formation.countBlock4();
    int flex3=formation.countFlex3();
    if (flex4>=2||flex4+block4>=2) return true;
    if (flex3>=2) return true;
    return false;
}

inline bool Gomoku::validPosition(const int x,const int y,Player player) {
    if (outOfRange(x,y)) return false;
    if (graph[pos(x,y)]!=0) return false;
    if (isForbidden(x,y,player)) return false;
    return true;
}

inline bool Gomoku::Win(const int x,const int y,const Player currentPlayer) const {
    for (auto& [dx,dy]:Config::directions){
        int count=1;
        int nx=x+dx,ny=y+dy;
        while (!outOfRange(nx,ny)&&graph[pos(nx,ny)]==currentPlayer){
            count++;
            nx+=dx,ny+=dy;
        }
        nx=x-dx,ny=y-dy;
        while (!outOfRange(nx,ny)&&graph[pos(nx,ny)]==currentPlayer){
            count++;
            nx-=dx,ny-=dy;
        }
        if (count>=5) return true;
    }
    return false;
}

inline bool Gomoku::GameOver() const {
    return currentCount>=maxCount;
}

inline void Gomoku::show() const {
    std::cout<<"   ";
    for (int j=1;j<=size;j++){
        std::cout.width(2);
        std::cout<<j<<" ";
    }
    std::cout<<"\n";
    for (int i=1;i<=size;i++){
        std::cout.width(2);
        std::cout<<i<<" ";
        for (int j=1;j<=size;j++){
            if (graph[pos(i,j)]==BLACK) std::cout<<" ● ";
            else if (graph[pos(i,j)]==WHITE) std::cout<<" ○ ";
            else std::cout<<" · ";
        }
        std::cout<<"\n";
    }
}

inline const std::vector<int8_t>& Gomoku::getGraph() const {
    return graph;
}

inline void Gomoku::undo(const int x,const int y) {
    if (outOfRange(x,y)) return;
    if (currentCount==0) return;
    if (graph[pos(x,y)]==NONE) return;
    currentCount--;
    graph[pos(x,y)]=0;
}

inline Player Gomoku::getColor(const int x,const int y) const {
    if (outOfRange(x,y)) return NONE;
    return graph[pos(x,y)]==BLACK? BLACK:graph[pos(x,y)]==WHITE? WHITE:NONE;
}

#endif
