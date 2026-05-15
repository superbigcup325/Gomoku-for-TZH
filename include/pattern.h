#ifndef GOMOKU_PATTERN_H
#define GOMOKU_PATTERN_H

#include<cstdint>
#include<unordered_map>

static constexpr int PATTERN_RADIUS=4;
static constexpr int PATTERN_WINDOW=9;
static constexpr int PATTERN_CENTER=4;

enum Player:int8_t { NONE=0,BLACK=1,WHITE=2 };

enum PatternType:uint8_t {
    OVERLINE,FIVE,FLEX4,BLOCK4,FLEX3,BLOCK3,FLEX2,BLOCK2,FLEX1,BLOCK1,FORBIDDEN,DEFAULT
};

struct PatternInfo {
    int8_t count=1;
    int8_t openEnds=0;
    int8_t gapSegments=0;
    int8_t gapSpaces=0;
    PatternType type=DEFAULT;
};

class PatternDB {
private:
    std::unordered_map<uint32_t,PatternInfo> table;

    inline uint32_t makeKey(const int cells[PATTERN_WINDOW]) const {
        uint32_t k=0;
        for (int i=0;i<PATTERN_WINDOW;i++){
            k|=(static_cast<uint32_t>(cells[i]&3)<<(i*2));
        }
        return k;
    }

    PatternDB() {
        initialize();
    }

    void addPattern(uint32_t key,int8_t count,int8_t openEnds,int8_t gapSegments,int8_t gapSpaces,PatternType type) {
        if (table.find(key)==table.end()){
            PatternInfo info;
            info.count=count;
            info.openEnds=openEnds;
            info.gapSegments=gapSegments;
            info.gapSpaces=gapSpaces;
            info.type=type;
            table[key]=info;
        }
    }

    void addPattern(const int cells[PATTERN_WINDOW],int8_t count,int8_t openEnds,int8_t gapSegments,int8_t gapSpaces,PatternType type) {
        addPattern(makeKey(cells),count,openEnds,gapSegments,gapSpaces,type);
    }

    void initialize() {
        generateFivePatterns();
        generateOverlinePatterns();
        addFlex4Patterns();
        addBlock4Patterns();
        addFlex3Patterns();
        addBlock3Patterns();
        addFlex2Patterns();
        addBlock2Patterns();
    }

    void generateFivePatterns() {
        const int positions[5][5]={
            {0,1,2,3,4},
            {1,2,3,4,5},
            {2,3,4,5,6},
            {3,4,5,6,7},
            {4,5,6,7,8}
        };
        for (int p=0;p<5;p++){
            int remaining[4];
            int ri=0;
            for (int i=0;i<PATTERN_WINDOW;i++){
                bool isInPos=false;
                for (int j=0;j<5;j++){
                    if (i==positions[p][j]){ isInPos=true; break; }
                }
                if (!isInPos) remaining[ri++]=i;
            }
            for (int mask=0;mask<16;mask++){
                int cells[PATTERN_WINDOW]={0};
                for (int j=0;j<5;j++) cells[positions[p][j]]=1;
                for (int b=0;b<4;b++){
                    cells[remaining[b]]=(mask>>b)&1? 2:0;
                }
                addPattern(cells,5,0,0,0,FIVE);
            }
        }
    }

    void generateOverlinePatterns() {
        for (int len=6;len<=PATTERN_WINDOW;len++){
            for (int start=0;start<=PATTERN_WINDOW-len;start++){
                if (start>PATTERN_CENTER||start+len<=PATTERN_CENTER) continue;
                int remaining[PATTERN_WINDOW-len];
                int ri=0;
                for (int i=0;i<PATTERN_WINDOW;i++){
                    if (i<start||i>=start+len) remaining[ri++]=i;
                }
                int combos=1<<ri;
                for (int mask=0;mask<combos;mask++){
                    int cells[PATTERN_WINDOW]={0};
                    for (int j=start;j<start+len;j++) cells[j]=1;
                    for (int b=0;b<ri;b++){
                        cells[remaining[b]]=(mask>>b)&1? 2:0;
                    }
                    addPattern(cells,len,0,0,0,OVERLINE);
                }
            }
        }
    }

    void addFlex4Patterns() {
        const int positions[4][4]={
            {1,2,3,4},
            {2,3,4,5},
            {3,4,5,6},
            {4,5,6,7}
        };
        const int leftOpen[4]={0,1,2,3};
        const int rightOpen[4]={5,6,7,8};
        for (int p=0;p<4;p++){
            int remaining[3];
            int ri=0;
            for (int i=0;i<PATTERN_WINDOW;i++){
                bool isInPos=false;
                for (int j=0;j<4;j++){
                    if (i==positions[p][j]){ isInPos=true; break; }
                }
                if (!isInPos&&i!=leftOpen[p]&&i!=rightOpen[p]) remaining[ri++]=i;
            }
            for (int mask=0;mask<(1<<ri);mask++){
                int cells[PATTERN_WINDOW]={0};
                for (int j=0;j<4;j++) cells[positions[p][j]]=1;
                cells[leftOpen[p]]=0;
                cells[rightOpen[p]]=0;
                for (int b=0;b<ri;b++){
                    cells[remaining[b]]=(mask>>b)&1? 2:0;
                }
                addPattern(cells,4,2,0,0,FLEX4);
            }
        }
    }

    void addBlock4Patterns() {
        auto addBlock4=[&](int a0,int a1,int a2,int a3,int a4,int a5,int a6,int a7,int a8,int openEnds,int gapSeg,int gapSp){
            int c[9]={a0,a1,a2,a3,a4,a5,a6,a7,a8};
            addPattern(c,4,openEnds,gapSeg,gapSp,BLOCK4);
        };

        // 4连一端被堵
        addBlock4(0,1,1,1,1,2,0,0,0, 1,0,0);
        addBlock4(0,1,1,1,1,2,0,0,2, 1,0,0);
        addBlock4(0,1,1,1,1,2,0,2,0, 1,0,0);
        addBlock4(0,1,1,1,1,2,0,2,2, 1,0,0);
        addBlock4(0,1,1,1,1,2,2,0,0, 1,0,0);
        addBlock4(0,1,1,1,1,2,2,0,2, 1,0,0);
        addBlock4(0,1,1,1,1,2,2,2,0, 1,0,0);
        addBlock4(0,1,1,1,1,2,2,2,2, 1,0,0);

        addBlock4(2,1,1,1,1,0,0,0,0, 1,0,0);
        addBlock4(2,1,1,1,1,0,0,0,2, 1,0,0);
        addBlock4(2,1,1,1,1,0,0,2,0, 1,0,0);
        addBlock4(2,1,1,1,1,0,0,2,2, 1,0,0);
        addBlock4(2,1,1,1,1,0,2,0,0, 1,0,0);
        addBlock4(2,1,1,1,1,0,2,0,2, 1,0,0);
        addBlock4(2,1,1,1,1,0,2,2,0, 1,0,0);
        addBlock4(2,1,1,1,1,0,2,2,2, 1,0,0);

        // 4连两端被堵
        addBlock4(2,1,1,1,1,2,0,0,0, 0,0,0);
        addBlock4(2,1,1,1,1,2,0,0,2, 0,0,0);
        addBlock4(2,1,1,1,1,2,0,2,0, 0,0,0);
        addBlock4(2,1,1,1,1,2,0,2,2, 0,0,0);
        addBlock4(2,1,1,1,1,2,2,0,0, 0,0,0);
        addBlock4(2,1,1,1,1,2,2,0,2, 0,0,0);
        addBlock4(2,1,1,1,1,2,2,2,0, 0,0,0);
        addBlock4(2,1,1,1,1,2,2,2,2, 0,0,0);

        // 4连偏移 [2,3,4,5]
        addBlock4(0,0,1,1,1,1,2,0,0, 1,0,0);
        addBlock4(0,0,1,1,1,1,2,0,2, 1,0,0);
        addBlock4(0,0,1,1,1,1,2,2,0, 1,0,0);
        addBlock4(0,0,1,1,1,1,2,2,2, 1,0,0);
        addBlock4(2,0,1,1,1,1,2,0,0, 1,0,0);
        addBlock4(2,0,1,1,1,1,2,0,2, 1,0,0);
        addBlock4(2,0,1,1,1,1,2,2,0, 1,0,0);
        addBlock4(2,0,1,1,1,1,2,2,2, 1,0,0);

        addBlock4(0,2,1,1,1,1,0,0,0, 1,0,0);
        addBlock4(0,2,1,1,1,1,0,0,2, 1,0,0);
        addBlock4(0,2,1,1,1,1,0,2,0, 1,0,0);
        addBlock4(0,2,1,1,1,1,0,2,2, 1,0,0);
        addBlock4(2,2,1,1,1,1,0,0,0, 1,0,0);
        addBlock4(2,2,1,1,1,1,0,0,2, 1,0,0);
        addBlock4(2,2,1,1,1,1,0,2,0, 1,0,0);
        addBlock4(2,2,1,1,1,1,0,2,2, 1,0,0);

        addBlock4(0,2,1,1,1,1,2,0,0, 0,0,0);
        addBlock4(0,2,1,1,1,1,2,0,2, 0,0,0);
        addBlock4(0,2,1,1,1,1,2,2,0, 0,0,0);
        addBlock4(0,2,1,1,1,1,2,2,2, 0,0,0);
        addBlock4(2,2,1,1,1,1,2,0,0, 0,0,0);
        addBlock4(2,2,1,1,1,1,2,0,2, 0,0,0);
        addBlock4(2,2,1,1,1,1,2,2,0, 0,0,0);
        addBlock4(2,2,1,1,1,1,2,2,2, 0,0,0);

        // 4连偏移 [3,4,5,6]
        addBlock4(0,0,0,1,1,1,1,2,0, 1,0,0);
        addBlock4(0,0,0,1,1,1,1,2,2, 1,0,0);
        addBlock4(0,0,2,1,1,1,1,2,0, 1,0,0);
        addBlock4(0,0,2,1,1,1,1,2,2, 1,0,0);
        addBlock4(2,0,0,1,1,1,1,2,0, 1,0,0);
        addBlock4(2,0,0,1,1,1,1,2,2, 1,0,0);
        addBlock4(2,0,2,1,1,1,1,2,0, 1,0,0);
        addBlock4(2,0,2,1,1,1,1,2,2, 1,0,0);

        addBlock4(0,0,2,1,1,1,1,0,0, 1,0,0);
        addBlock4(0,0,2,1,1,1,1,0,2, 1,0,0);
        addBlock4(0,0,2,1,1,1,1,2,0, 1,0,0);
        addBlock4(0,0,2,1,1,1,1,2,2, 1,0,0);
        addBlock4(2,0,2,1,1,1,1,0,0, 1,0,0);
        addBlock4(2,0,2,1,1,1,1,0,2, 1,0,0);
        addBlock4(2,0,2,1,1,1,1,2,0, 1,0,0);
        addBlock4(2,0,2,1,1,1,1,2,2, 1,0,0);

        addBlock4(0,0,2,1,1,1,1,2,0, 0,0,0);
        addBlock4(0,0,2,1,1,1,1,2,2, 0,0,0);
        addBlock4(2,0,2,1,1,1,1,2,0, 0,0,0);
        addBlock4(2,0,2,1,1,1,1,2,2, 0,0,0);

        // 4连偏移 [4,5,6,7]
        addBlock4(0,0,0,0,1,1,1,1,2, 1,0,0);
        addBlock4(0,0,0,2,1,1,1,1,2, 1,0,0);
        addBlock4(0,0,2,0,1,1,1,1,2, 1,0,0);
        addBlock4(0,0,2,2,1,1,1,1,2, 1,0,0);
        addBlock4(2,0,0,0,1,1,1,1,2, 1,0,0);
        addBlock4(2,0,0,2,1,1,1,1,2, 1,0,0);
        addBlock4(2,0,2,0,1,1,1,1,2, 1,0,0);
        addBlock4(2,0,2,2,1,1,1,1,2, 1,0,0);

        addBlock4(0,0,0,2,1,1,1,1,0, 1,0,0);
        addBlock4(0,0,0,2,1,1,1,1,2, 1,0,0);
        addBlock4(0,0,2,2,1,1,1,1,0, 1,0,0);
        addBlock4(0,0,2,2,1,1,1,1,2, 1,0,0);
        addBlock4(2,0,0,2,1,1,1,1,0, 1,0,0);
        addBlock4(2,0,0,2,1,1,1,1,2, 1,0,0);
        addBlock4(2,0,2,2,1,1,1,1,0, 1,0,0);
        addBlock4(2,0,2,2,1,1,1,1,2, 1,0,0);

        addBlock4(0,0,0,2,1,1,1,1,2, 0,0,0);
        addBlock4(0,0,2,2,1,1,1,1,2, 0,0,0);
        addBlock4(2,0,0,2,1,1,1,1,2, 0,0,0);
        addBlock4(2,0,2,2,1,1,1,1,2, 0,0,0);

        // 跳冲四 11011
        addBlock4(0,0,1,1,0,1,1,0,0, 1,1,1);
        addBlock4(0,0,1,1,0,1,1,0,2, 1,1,1);
        addBlock4(0,0,1,1,0,1,1,2,0, 1,1,1);
        addBlock4(0,0,1,1,0,1,1,2,2, 1,1,1);
        addBlock4(0,2,1,1,0,1,1,0,0, 1,1,1);
        addBlock4(0,2,1,1,0,1,1,0,2, 1,1,1);
        addBlock4(0,2,1,1,0,1,1,2,0, 1,1,1);
        addBlock4(0,2,1,1,0,1,1,2,2, 1,1,1);
        addBlock4(2,0,1,1,0,1,1,0,0, 1,1,1);
        addBlock4(2,0,1,1,0,1,1,0,2, 1,1,1);
        addBlock4(2,0,1,1,0,1,1,2,0, 1,1,1);
        addBlock4(2,0,1,1,0,1,1,2,2, 1,1,1);
        addBlock4(2,2,1,1,0,1,1,0,0, 1,1,1);
        addBlock4(2,2,1,1,0,1,1,0,2, 1,1,1);
        addBlock4(2,2,1,1,0,1,1,2,0, 1,1,1);
        addBlock4(2,2,1,1,0,1,1,2,2, 1,1,1);

        // 跳冲四 10111
        addBlock4(0,0,1,0,1,1,1,0,0, 1,1,1);
        addBlock4(0,0,1,0,1,1,1,0,2, 1,1,1);
        addBlock4(0,0,1,0,1,1,1,2,0, 1,1,1);
        addBlock4(0,0,1,0,1,1,1,2,2, 1,1,1);
        addBlock4(0,2,1,0,1,1,1,0,0, 1,1,1);
        addBlock4(0,2,1,0,1,1,1,0,2, 1,1,1);
        addBlock4(0,2,1,0,1,1,1,2,0, 1,1,1);
        addBlock4(0,2,1,0,1,1,1,2,2, 1,1,1);
        addBlock4(2,0,1,0,1,1,1,0,0, 1,1,1);
        addBlock4(2,0,1,0,1,1,1,0,2, 1,1,1);
        addBlock4(2,0,1,0,1,1,1,2,0, 1,1,1);
        addBlock4(2,0,1,0,1,1,1,2,2, 1,1,1);
        addBlock4(2,2,1,0,1,1,1,0,0, 1,1,1);
        addBlock4(2,2,1,0,1,1,1,0,2, 1,1,1);
        addBlock4(2,2,1,0,1,1,1,2,0, 1,1,1);
        addBlock4(2,2,1,0,1,1,1,2,2, 1,1,1);

        // 跳冲四 11101
        addBlock4(0,0,1,1,1,0,1,0,0, 1,1,1);
        addBlock4(0,0,1,1,1,0,1,0,2, 1,1,1);
        addBlock4(0,0,1,1,1,0,1,2,0, 1,1,1);
        addBlock4(0,0,1,1,1,0,1,2,2, 1,1,1);
        addBlock4(0,2,1,1,1,0,1,0,0, 1,1,1);
        addBlock4(0,2,1,1,1,0,1,0,2, 1,1,1);
        addBlock4(0,2,1,1,1,0,1,2,0, 1,1,1);
        addBlock4(0,2,1,1,1,0,1,2,2, 1,1,1);
        addBlock4(2,0,1,1,1,0,1,0,0, 1,1,1);
        addBlock4(2,0,1,1,1,0,1,0,2, 1,1,1);
        addBlock4(2,0,1,1,1,0,1,2,0, 1,1,1);
        addBlock4(2,0,1,1,1,0,1,2,2, 1,1,1);
        addBlock4(2,2,1,1,1,0,1,0,0, 1,1,1);
        addBlock4(2,2,1,1,1,0,1,0,2, 1,1,1);
        addBlock4(2,2,1,1,1,0,1,2,0, 1,1,1);
        addBlock4(2,2,1,1,1,0,1,2,2, 1,1,1);

        // 跳冲四偏移
        addBlock4(0,0,0,1,1,0,1,1,0, 1,1,1);
        addBlock4(0,0,0,1,1,0,1,1,2, 1,1,1);
        addBlock4(0,0,2,1,1,0,1,1,0, 1,1,1);
        addBlock4(0,0,2,1,1,0,1,1,2, 1,1,1);
        addBlock4(2,0,0,1,1,0,1,1,0, 1,1,1);
        addBlock4(2,0,0,1,1,0,1,1,2, 1,1,1);
        addBlock4(2,0,2,1,1,0,1,1,0, 1,1,1);
        addBlock4(2,0,2,1,1,0,1,1,2, 1,1,1);

        addBlock4(0,0,0,1,0,1,1,1,0, 1,1,1);
        addBlock4(0,0,0,1,0,1,1,1,2, 1,1,1);
        addBlock4(0,0,2,1,0,1,1,1,0, 1,1,1);
        addBlock4(0,0,2,1,0,1,1,1,2, 1,1,1);
        addBlock4(2,0,0,1,0,1,1,1,0, 1,1,1);
        addBlock4(2,0,0,1,0,1,1,1,2, 1,1,1);
        addBlock4(2,0,2,1,0,1,1,1,0, 1,1,1);
        addBlock4(2,0,2,1,0,1,1,1,2, 1,1,1);

        addBlock4(0,0,0,1,1,1,0,1,0, 1,1,1);
        addBlock4(0,0,0,1,1,1,0,1,2, 1,1,1);
        addBlock4(0,0,2,1,1,1,0,1,0, 1,1,1);
        addBlock4(0,0,2,1,1,1,0,1,2, 1,1,1);
        addBlock4(2,0,0,1,1,1,0,1,0, 1,1,1);
        addBlock4(2,0,0,1,1,1,0,1,2, 1,1,1);
        addBlock4(2,0,2,1,1,1,0,1,0, 1,1,1);
        addBlock4(2,0,2,1,1,1,0,1,2, 1,1,1);

        // 跳冲四两端被堵
        addBlock4(2,2,1,1,0,1,1,2,2, 0,1,1);
        addBlock4(2,2,1,0,1,1,1,2,2, 0,1,1);
        addBlock4(2,2,1,1,1,0,1,2,2, 0,1,1);
    }

    void addFlex3Patterns() {
        auto addFlex3=[&](int a0,int a1,int a2,int a3,int a4,int a5,int a6,int a7,int a8,int gapSeg,int gapSp){
            int c[9]={a0,a1,a2,a3,a4,a5,a6,a7,a8};
            addPattern(c,3,2,gapSeg,gapSp,FLEX3);
        };

        // 3连 [2,3,4]
        addFlex3(0,0,1,1,1,0,0,0,0, 0,0);
        addFlex3(0,0,1,1,1,0,0,0,2, 0,0);
        addFlex3(0,0,1,1,1,0,0,2,0, 0,0);
        addFlex3(0,0,1,1,1,0,0,2,2, 0,0);
        addFlex3(2,0,1,1,1,0,0,0,0, 0,0);
        addFlex3(2,0,1,1,1,0,0,0,2, 0,0);
        addFlex3(2,0,1,1,1,0,0,2,0, 0,0);
        addFlex3(2,0,1,1,1,0,0,2,2, 0,0);

        // 3连 [3,4,5]
        addFlex3(0,0,0,1,1,1,0,0,0, 0,0);
        addFlex3(0,0,0,1,1,1,0,0,2, 0,0);
        addFlex3(0,0,2,1,1,1,0,0,0, 0,0);
        addFlex3(0,0,2,1,1,1,0,0,2, 0,0);
        addFlex3(2,0,0,1,1,1,0,0,0, 0,0);
        addFlex3(2,0,0,1,1,1,0,0,2, 0,0);
        addFlex3(2,0,2,1,1,1,0,0,0, 0,0);
        addFlex3(2,0,2,1,1,1,0,0,2, 0,0);

        // 3连 [4,5,6]
        addFlex3(0,0,0,0,1,1,1,0,0, 0,0);
        addFlex3(0,0,0,2,1,1,1,0,0, 0,0);
        addFlex3(2,0,0,0,1,1,1,0,0, 0,0);
        addFlex3(2,0,0,2,1,1,1,0,0, 0,0);

        // 跳活三 1011
        addFlex3(0,0,1,0,1,1,0,0,0, 1,1);
        addFlex3(0,0,1,0,1,1,0,0,2, 1,1);
        addFlex3(2,0,1,0,1,1,0,0,0, 1,1);
        addFlex3(2,0,1,0,1,1,0,0,2, 1,1);

        // 跳活三 1101
        addFlex3(0,0,1,1,0,1,0,0,0, 1,1);
        addFlex3(0,0,1,1,0,1,0,0,2, 1,1);
        addFlex3(2,0,1,1,0,1,0,0,0, 1,1);
        addFlex3(2,0,1,1,0,1,0,0,2, 1,1);

        // 跳活三 1011偏移
        addFlex3(0,0,0,1,0,1,1,0,0, 1,1);
        addFlex3(0,0,0,1,0,1,1,0,2, 1,1);
        addFlex3(2,0,0,1,0,1,1,0,0, 1,1);
        addFlex3(2,0,0,1,0,1,1,0,2, 1,1);

        // 跳活三 1101偏移
        addFlex3(0,0,0,1,1,0,1,0,0, 1,1);
        addFlex3(0,0,0,1,1,0,1,0,2, 1,1);
        addFlex3(2,0,0,1,1,0,1,0,0, 1,1);
        addFlex3(2,0,0,1,1,0,1,0,2, 1,1);
    }

    void addBlock3Patterns() {
        auto addBlock3=[&](int a0,int a1,int a2,int a3,int a4,int a5,int a6,int a7,int a8,int openEnds,int gapSeg,int gapSp){
            int c[9]={a0,a1,a2,a3,a4,a5,a6,a7,a8};
            addPattern(c,3,openEnds,gapSeg,gapSp,BLOCK3);
        };

        // 3连一端被堵 [2,3,4]
        addBlock3(0,0,1,1,1,2,0,0,0, 1,0,0);
        addBlock3(0,0,1,1,1,2,0,0,2, 1,0,0);
        addBlock3(0,0,1,1,1,2,0,2,0, 1,0,0);
        addBlock3(0,0,1,1,1,2,0,2,2, 1,0,0);
        addBlock3(2,0,1,1,1,2,0,0,0, 1,0,0);
        addBlock3(2,0,1,1,1,2,0,0,2, 1,0,0);
        addBlock3(2,0,1,1,1,2,0,2,0, 1,0,0);
        addBlock3(2,0,1,1,1,2,0,2,2, 1,0,0);

        addBlock3(0,2,1,1,1,0,0,0,0, 1,0,0);
        addBlock3(0,2,1,1,1,0,0,0,2, 1,0,0);
        addBlock3(0,2,1,1,1,0,0,2,0, 1,0,0);
        addBlock3(0,2,1,1,1,0,0,2,2, 1,0,0);
        addBlock3(2,2,1,1,1,0,0,0,0, 1,0,0);
        addBlock3(2,2,1,1,1,0,0,0,2, 1,0,0);
        addBlock3(2,2,1,1,1,0,0,2,0, 1,0,0);
        addBlock3(2,2,1,1,1,0,0,2,2, 1,0,0);

        // 3连两端被堵 [2,3,4]
        addBlock3(0,2,1,1,1,2,0,0,0, 0,0,0);
        addBlock3(0,2,1,1,1,2,0,0,2, 0,0,0);
        addBlock3(0,2,1,1,1,2,0,2,0, 0,0,0);
        addBlock3(0,2,1,1,1,2,0,2,2, 0,0,0);
        addBlock3(2,2,1,1,1,2,0,0,0, 0,0,0);
        addBlock3(2,2,1,1,1,2,0,0,2, 0,0,0);
        addBlock3(2,2,1,1,1,2,0,2,0, 0,0,0);
        addBlock3(2,2,1,1,1,2,0,2,2, 0,0,0);

        // 3连一端被堵 [3,4,5]
        addBlock3(0,0,0,1,1,1,2,0,0, 1,0,0);
        addBlock3(0,0,0,1,1,1,2,0,2, 1,0,0);
        addBlock3(0,0,0,1,1,1,2,2,0, 1,0,0);
        addBlock3(0,0,0,1,1,1,2,2,2, 1,0,0);
        addBlock3(2,0,0,1,1,1,2,0,0, 1,0,0);
        addBlock3(2,0,0,1,1,1,2,0,2, 1,0,0);
        addBlock3(2,0,0,1,1,1,2,2,0, 1,0,0);
        addBlock3(2,0,0,1,1,1,2,2,2, 1,0,0);

        addBlock3(0,0,2,1,1,1,0,0,0, 1,0,0);
        addBlock3(0,0,2,1,1,1,0,0,2, 1,0,0);
        addBlock3(0,0,2,1,1,1,0,2,0, 1,0,0);
        addBlock3(0,0,2,1,1,1,0,2,2, 1,0,0);
        addBlock3(2,0,2,1,1,1,0,0,0, 1,0,0);
        addBlock3(2,0,2,1,1,1,0,0,2, 1,0,0);
        addBlock3(2,0,2,1,1,1,0,2,0, 1,0,0);
        addBlock3(2,0,2,1,1,1,0,2,2, 1,0,0);

        // 3连两端被堵 [3,4,5]
        addBlock3(0,0,2,1,1,1,2,0,0, 0,0,0);
        addBlock3(0,0,2,1,1,1,2,0,2, 0,0,0);
        addBlock3(0,0,2,1,1,1,2,2,0, 0,0,0);
        addBlock3(0,0,2,1,1,1,2,2,2, 0,0,0);
        addBlock3(2,0,2,1,1,1,2,0,0, 0,0,0);
        addBlock3(2,0,2,1,1,1,2,0,2, 0,0,0);
        addBlock3(2,0,2,1,1,1,2,2,0, 0,0,0);
        addBlock3(2,0,2,1,1,1,2,2,2, 0,0,0);

        // 3连一端被堵 [4,5,6]
        addBlock3(0,0,0,0,1,1,1,2,0, 1,0,0);
        addBlock3(0,0,0,0,1,1,1,2,2, 1,0,0);
        addBlock3(0,0,0,2,1,1,1,2,0, 1,0,0);
        addBlock3(0,0,0,2,1,1,1,2,2, 1,0,0);
        addBlock3(2,0,0,0,1,1,1,2,0, 1,0,0);
        addBlock3(2,0,0,0,1,1,1,2,2, 1,0,0);
        addBlock3(2,0,0,2,1,1,1,2,0, 1,0,0);
        addBlock3(2,0,0,2,1,1,1,2,2, 1,0,0);

        addBlock3(0,0,0,2,1,1,1,0,0, 1,0,0);
        addBlock3(0,0,0,2,1,1,1,0,2, 1,0,0);
        addBlock3(0,0,0,2,1,1,1,2,0, 1,0,0);
        addBlock3(0,0,0,2,1,1,1,2,2, 1,0,0);
        addBlock3(2,0,0,2,1,1,1,0,0, 1,0,0);
        addBlock3(2,0,0,2,1,1,1,0,2, 1,0,0);
        addBlock3(2,0,0,2,1,1,1,2,0, 1,0,0);
        addBlock3(2,0,0,2,1,1,1,2,2, 1,0,0);

        // 3连两端被堵 [4,5,6]
        addBlock3(0,0,0,2,1,1,1,2,0, 0,0,0);
        addBlock3(0,0,0,2,1,1,1,2,2, 0,0,0);
        addBlock3(2,0,0,2,1,1,1,2,0, 0,0,0);
        addBlock3(2,0,0,2,1,1,1,2,2, 0,0,0);

        // 跳眠三 1011一端被堵
        addBlock3(0,2,1,0,1,1,0,0,0, 1,1,1);
        addBlock3(0,2,1,0,1,1,0,0,2, 1,1,1);
        addBlock3(0,2,1,0,1,1,0,2,0, 1,1,1);
        addBlock3(0,2,1,0,1,1,0,2,2, 1,1,1);
        addBlock3(2,2,1,0,1,1,0,0,0, 1,1,1);
        addBlock3(2,2,1,0,1,1,0,0,2, 1,1,1);
        addBlock3(2,2,1,0,1,1,0,2,0, 1,1,1);
        addBlock3(2,2,1,0,1,1,0,2,2, 1,1,1);

        // 跳眠三 1101一端被堵
        addBlock3(0,2,1,1,0,1,0,0,0, 1,1,1);
        addBlock3(0,2,1,1,0,1,0,0,2, 1,1,1);
        addBlock3(0,2,1,1,0,1,0,2,0, 1,1,1);
        addBlock3(0,2,1,1,0,1,0,2,2, 1,1,1);
        addBlock3(2,2,1,1,0,1,0,0,0, 1,1,1);
        addBlock3(2,2,1,1,0,1,0,0,2, 1,1,1);
        addBlock3(2,2,1,1,0,1,0,2,0, 1,1,1);
        addBlock3(2,2,1,1,0,1,0,2,2, 1,1,1);

        // 跳眠三 1011右端被堵
        addBlock3(0,0,1,0,1,1,2,0,0, 1,1,1);
        addBlock3(0,0,1,0,1,1,2,0,2, 1,1,1);
        addBlock3(0,0,1,0,1,1,2,2,0, 1,1,1);
        addBlock3(0,0,1,0,1,1,2,2,2, 1,1,1);
        addBlock3(2,0,1,0,1,1,2,0,0, 1,1,1);
        addBlock3(2,0,1,0,1,1,2,0,2, 1,1,1);
        addBlock3(2,0,1,0,1,1,2,2,0, 1,1,1);
        addBlock3(2,0,1,0,1,1,2,2,2, 1,1,1);

        // 跳眠三 1101右端被堵
        addBlock3(0,0,1,1,0,1,2,0,0, 1,1,1);
        addBlock3(0,0,1,1,0,1,2,0,2, 1,1,1);
        addBlock3(0,0,1,1,0,1,2,2,0, 1,1,1);
        addBlock3(0,0,1,1,0,1,2,2,2, 1,1,1);
        addBlock3(2,0,1,1,0,1,2,0,0, 1,1,1);
        addBlock3(2,0,1,1,0,1,2,0,2, 1,1,1);
        addBlock3(2,0,1,1,0,1,2,2,0, 1,1,1);
        addBlock3(2,0,1,1,0,1,2,2,2, 1,1,1);

        // 跳眠三两端被堵
        addBlock3(2,2,1,0,1,1,2,0,0, 0,1,1);
        addBlock3(2,2,1,0,1,1,2,0,2, 0,1,1);
        addBlock3(2,2,1,0,1,1,2,2,0, 0,1,1);
        addBlock3(2,2,1,0,1,1,2,2,2, 0,1,1);
        addBlock3(2,2,1,1,0,1,2,0,0, 0,1,1);
        addBlock3(2,2,1,1,0,1,2,0,2, 0,1,1);
        addBlock3(2,2,1,1,0,1,2,2,0, 0,1,1);
        addBlock3(2,2,1,1,0,1,2,2,2, 0,1,1);

        // 伪活三(只能形成冲四)
        addBlock3(0,0,1,1,1,0,2,0,0, 0,0,0);
        addBlock3(0,0,1,1,1,0,2,0,2, 0,0,0);
        addBlock3(0,0,1,1,1,0,2,2,0, 0,0,0);
        addBlock3(0,0,1,1,1,0,2,2,2, 0,0,0);
        addBlock3(2,0,1,1,1,0,2,0,0, 0,0,0);
        addBlock3(2,0,1,1,1,0,2,0,2, 0,0,0);
        addBlock3(2,0,1,1,1,0,2,2,0, 0,0,0);
        addBlock3(2,0,1,1,1,0,2,2,2, 0,0,0);

        addBlock3(0,0,0,1,1,1,0,2,0, 0,0,0);
        addBlock3(0,0,0,1,1,1,0,2,2, 0,0,0);
        addBlock3(0,0,2,1,1,1,0,2,0, 0,0,0);
        addBlock3(0,0,2,1,1,1,0,2,2, 0,0,0);
        addBlock3(2,0,0,1,1,1,0,2,0, 0,0,0);
        addBlock3(2,0,0,1,1,1,0,2,2, 0,0,0);
        addBlock3(2,0,2,1,1,1,0,2,0, 0,0,0);
        addBlock3(2,0,2,1,1,1,0,2,2, 0,0,0);

        addBlock3(0,0,0,0,1,1,1,0,2, 0,0,0);
        addBlock3(0,0,0,0,1,1,1,2,0, 0,0,0);
        addBlock3(0,0,0,0,1,1,1,2,2, 0,0,0);
        addBlock3(0,0,0,2,1,1,1,0,2, 0,0,0);
        addBlock3(0,0,0,2,1,1,1,2,0, 0,0,0);
        addBlock3(0,0,0,2,1,1,1,2,2, 0,0,0);
        addBlock3(2,0,0,0,1,1,1,0,2, 0,0,0);
        addBlock3(2,0,0,0,1,1,1,2,0, 0,0,0);
        addBlock3(2,0,0,0,1,1,1,2,2, 0,0,0);
        addBlock3(2,0,0,2,1,1,1,0,2, 0,0,0);
        addBlock3(2,0,0,2,1,1,1,2,0, 0,0,0);
        addBlock3(2,0,0,2,1,1,1,2,2, 0,0,0);

        // 跳眠三 1011右端受限
        addBlock3(0,0,1,0,1,1,0,2,0, 1,1,1);
        addBlock3(0,0,1,0,1,1,0,2,2, 1,1,1);
        addBlock3(2,0,1,0,1,1,0,2,0, 1,1,1);
        addBlock3(2,0,1,0,1,1,0,2,2, 1,1,1);

        // 跳眠三 1101右端受限
        addBlock3(0,0,1,1,0,1,0,2,0, 1,1,1);
        addBlock3(0,0,1,1,0,1,0,2,2, 1,1,1);
        addBlock3(2,0,1,1,0,1,0,2,0, 1,1,1);
        addBlock3(2,0,1,1,0,1,0,2,2, 1,1,1);

        // 跳眠三 1011偏移右端受限
        addBlock3(0,0,0,1,0,1,1,2,0, 1,1,1);
        addBlock3(0,0,0,1,0,1,1,2,2, 1,1,1);
        addBlock3(2,0,0,1,0,1,1,2,0, 1,1,1);
        addBlock3(2,0,0,1,0,1,1,2,2, 1,1,1);

        // 跳眠三 1101偏移右端受限
        addBlock3(0,0,0,1,1,0,1,2,0, 1,1,1);
        addBlock3(0,0,0,1,1,0,1,2,2, 1,1,1);
        addBlock3(2,0,0,1,1,0,1,2,0, 1,1,1);
        addBlock3(2,0,0,1,1,0,1,2,2, 1,1,1);

        // 跳眠三 10101两端开放
        addBlock3(0,0,1,0,1,0,1,0,0, 0,2,2);
        addBlock3(0,0,1,0,1,0,1,0,2, 0,2,2);
        addBlock3(2,0,1,0,1,0,1,0,0, 0,2,2);
        addBlock3(2,0,1,0,1,0,1,0,2, 0,2,2);

        // 跳眠三 10101偏移两端开放
        addBlock3(0,0,0,1,0,1,0,1,0, 0,2,2);
        addBlock3(0,0,0,1,0,1,0,1,2, 0,2,2);
        addBlock3(2,0,0,1,0,1,0,1,0, 0,2,2);
        addBlock3(2,0,0,1,0,1,0,1,2, 0,2,2);

        // 跳眠三 10011两端开放
        addBlock3(0,0,1,0,0,1,1,0,0, 0,1,2);
        addBlock3(0,0,1,0,0,1,1,0,2, 0,1,2);
        addBlock3(2,0,1,0,0,1,1,0,0, 0,1,2);
        addBlock3(2,0,1,0,0,1,1,0,2, 0,1,2);

        // 跳眠三 11001两端开放
        addBlock3(0,0,1,1,0,0,1,0,0, 0,1,2);
        addBlock3(0,0,1,1,0,0,1,0,2, 0,1,2);
        addBlock3(2,0,1,1,0,0,1,0,0, 0,1,2);
        addBlock3(2,0,1,1,0,0,1,0,2, 0,1,2);

        // 跳眠三 10101右端受限
        addBlock3(0,0,1,0,1,0,1,2,0, 0,2,2);
        addBlock3(0,0,1,0,1,0,1,2,2, 0,2,2);
        addBlock3(2,0,1,0,1,0,1,2,0, 0,2,2);
        addBlock3(2,0,1,0,1,0,1,2,2, 0,2,2);

        // 跳眠三 10101偏移左端受限
        addBlock3(0,0,2,1,0,1,0,1,0, 0,2,2);
        addBlock3(0,0,2,1,0,1,0,1,2, 0,2,2);
        addBlock3(2,0,2,1,0,1,0,1,0, 0,2,2);
        addBlock3(2,0,2,1,0,1,0,1,2, 0,2,2);

        // 跳眠三 10011右端受限
        addBlock3(0,0,1,0,0,1,1,2,0, 0,1,2);
        addBlock3(0,0,1,0,0,1,1,2,2, 0,1,2);
        addBlock3(2,0,1,0,0,1,1,2,0, 0,1,2);
        addBlock3(2,0,1,0,0,1,1,2,2, 0,1,2);

        // 跳眠三 11001右端受限
        addBlock3(0,0,1,1,0,0,1,2,0, 0,1,2);
        addBlock3(0,0,1,1,0,0,1,2,2, 0,1,2);
        addBlock3(2,0,1,1,0,0,1,2,0, 0,1,2);
        addBlock3(2,0,1,1,0,0,1,2,2, 0,1,2);

        // ===== P0 FIX: 跳活三间隙被填后的BLOCK3模式 =====
        // 跳活三 1011 (X_XX) - 间隙[3]被填充 → 分散为 XO XX 或 X OXX
        // 形态: _ X O X X _ _ _
        addBlock3(0,0,1,2,1,1,0,0,0, 0,0,0);  // 基本形态
        addBlock3(0,0,1,2,1,1,0,0,2, 0,0,0);  // 远位右端[8]
        addBlock3(0,0,1,2,1,1,0,2,0, 0,0,0);  // 右远位[7]
        addBlock3(0,0,1,2,1,1,0,2,2, 0,0,0);  // 双远位[7,8]
        addBlock3(0,0,1,2,1,1,2,0,0, 0,0,0);  // 右紧邻[6]被封
        addBlock3(0,0,1,2,1,1,2,0,2, 0,0,0);
        addBlock3(0,0,1,2,1,1,2,2,0, 0,0,0);
        addBlock3(0,0,1,2,1,1,2,2,2, 0,0,0);
        addBlock3(2,0,1,2,1,1,0,0,0, 0,0,0);  // 左端[1]被封
        addBlock3(2,0,1,2,1,1,0,0,2, 0,0,0);
        addBlock3(2,0,1,2,1,1,0,2,0, 0,0,0);
        addBlock3(2,0,1,2,1,1,0,2,2, 0,0,0);
        addBlock3(2,0,1,2,1,1,2,0,0, 0,0,0);  // 左+右紧邻
        addBlock3(2,0,1,2,1,1,2,0,2, 0,0,0);
        addBlock3(2,0,1,2,1,1,2,2,0, 0,0,0);
        addBlock3(2,0,1,2,1,1,2,2,2, 0,0,0);

        // 跳活三 1101 (XX_X) - 间隙[4]被填充 → 分散为 XX OX 或 XXO X
        // 形态: _ X X O X _ _ _
        addBlock3(0,0,1,1,2,1,0,0,0, 0,0,0);  // 基本形态
        addBlock3(0,0,1,1,2,1,0,0,2, 0,0,0);
        addBlock3(0,0,1,1,2,1,0,2,0, 0,0,0);
        addBlock3(0,0,1,1,2,1,0,2,2, 0,0,0);
        addBlock3(0,0,1,1,2,1,2,0,0, 0,0,0);  // 右紧邻[6]被封
        addBlock3(0,0,1,1,2,1,2,0,2, 0,0,0);
        addBlock3(0,0,1,1,2,1,2,2,0, 0,0,0);
        addBlock3(0,0,1,1,2,1,2,2,2, 0,0,0);
        addBlock3(2,0,1,1,2,1,0,0,0, 0,0,0);
        addBlock3(2,0,1,1,2,1,0,0,2, 0,0,0);
        addBlock3(2,0,1,1,2,1,0,2,0, 0,0,0);
        addBlock3(2,0,1,1,2,1,0,2,2, 0,0,0);
        addBlock3(2,0,1,1,2,1,2,0,0, 0,0,0);  // 左+右紧邻
        addBlock3(2,0,1,1,2,1,2,0,2, 0,0,0);
        addBlock3(2,0,1,1,2,1,2,2,0, 0,0,0);
        addBlock3(2,0,1,1,2,1,2,2,2, 0,0,0);

        // 跳活三 1011 偏移 - 间隙被填
        // 形态: _ _ X O X X _ _
        addBlock3(0,0,0,1,2,1,1,0,0, 0,0,0);
        addBlock3(0,0,0,1,2,1,1,0,2, 0,0,0);
        addBlock3(2,0,0,1,2,1,1,0,0, 0,0,0);
        addBlock3(2,0,0,1,2,1,1,0,2, 0,0,0);

        // 跳活三 1101 偏移 - 间隙被填
        // 形态: _ _ X X O X _ _
        addBlock3(0,0,0,1,1,2,1,0,0, 0,0,0);
        addBlock3(0,0,0,1,1,2,1,0,2, 0,0,0);
        addBlock3(2,0,0,1,1,2,1,0,0, 0,0,0);
        addBlock3(2,0,0,1,1,2,1,0,2, 0,0,0);

        // ===== P2 FIX: BLOCK3 分散棋子组合补充 =====
    }

    void addFlex2Patterns() {
        auto addFlex2=[&](int a0,int a1,int a2,int a3,int a4,int a5,int a6,int a7,int a8,int gapSeg,int gapSp){
            int c[9]={a0,a1,a2,a3,a4,a5,a6,a7,a8};
            addPattern(c,2,2,gapSeg,gapSp,FLEX2);
        };

        // 2连 [3,4]
        addFlex2(0,0,0,1,1,0,0,0,0, 0,0);
        addFlex2(0,0,0,1,1,0,0,0,2, 0,0);
        addFlex2(0,0,0,1,1,0,0,2,0, 0,0);
        addFlex2(0,0,0,1,1,0,0,2,2, 0,0);
        addFlex2(2,0,0,1,1,0,0,0,0, 0,0);
        addFlex2(2,0,0,1,1,0,0,0,2, 0,0);
        addFlex2(2,0,0,1,1,0,0,2,0, 0,0);
        addFlex2(2,0,0,1,1,0,0,2,2, 0,0);

        // 2连 [4,5]
        addFlex2(0,0,0,0,1,1,0,0,0, 0,0);
        addFlex2(0,0,0,0,1,1,0,0,2, 0,0);
        addFlex2(0,0,0,0,1,1,0,2,0, 0,0);
        addFlex2(0,0,0,0,1,1,0,2,2, 0,0);
        addFlex2(2,0,0,0,1,1,0,0,0, 0,0);
        addFlex2(2,0,0,0,1,1,0,0,2, 0,0);
        addFlex2(2,0,0,0,1,1,0,2,0, 0,0);
        addFlex2(2,0,0,0,1,1,0,2,2, 0,0);

        // 跳活二 101
        addFlex2(0,0,0,1,0,1,0,0,0, 1,1);
        addFlex2(0,0,0,1,0,1,0,0,2, 1,1);
        addFlex2(0,0,0,1,0,1,0,2,0, 1,1);
        addFlex2(0,0,0,1,0,1,0,2,2, 1,1);
        addFlex2(2,0,0,1,0,1,0,0,0, 1,1);
        addFlex2(2,0,0,1,0,1,0,0,2, 1,1);
        addFlex2(2,0,0,1,0,1,0,2,0, 1,1);
        addFlex2(2,0,0,1,0,1,0,2,2, 1,1);

        // 跳活二 1001
        addFlex2(0,0,0,1,0,0,1,0,0, 1,2);
        addFlex2(0,0,0,1,0,0,1,0,2, 1,2);
        addFlex2(0,0,0,1,0,0,1,2,0, 1,2);
        addFlex2(0,0,0,1,0,0,1,2,2, 1,2);
        addFlex2(2,0,0,1,0,0,1,0,0, 1,2);
        addFlex2(2,0,0,1,0,0,1,0,2, 1,2);
        addFlex2(2,0,0,1,0,0,1,2,0, 1,2);
        addFlex2(2,0,0,1,0,0,1,2,2, 1,2);
    }

    void addBlock2Patterns() {
        auto addBlock2=[&](int a0,int a1,int a2,int a3,int a4,int a5,int a6,int a7,int a8,int openEnds,int gapSeg,int gapSp){
            int c[9]={a0,a1,a2,a3,a4,a5,a6,a7,a8};
            addPattern(c,2,openEnds,gapSeg,gapSp,BLOCK2);
        };

        // 2连一端被堵 [3,4]
        addBlock2(0,0,0,1,1,2,0,0,0, 1,0,0);
        addBlock2(0,0,0,1,1,2,0,0,2, 1,0,0);
        addBlock2(0,0,0,1,1,2,0,2,0, 1,0,0);
        addBlock2(0,0,0,1,1,2,0,2,2, 1,0,0);
        addBlock2(2,0,0,1,1,2,0,0,0, 1,0,0);
        addBlock2(2,0,0,1,1,2,0,0,2, 1,0,0);
        addBlock2(2,0,0,1,1,2,0,2,0, 1,0,0);
        addBlock2(2,0,0,1,1,2,0,2,2, 1,0,0);

        addBlock2(0,0,2,1,1,0,0,0,0, 1,0,0);
        addBlock2(0,0,2,1,1,0,0,0,2, 1,0,0);
        addBlock2(0,0,2,1,1,0,0,2,0, 1,0,0);
        addBlock2(0,0,2,1,1,0,0,2,2, 1,0,0);
        addBlock2(2,0,2,1,1,0,0,0,0, 1,0,0);
        addBlock2(2,0,2,1,1,0,0,0,2, 1,0,0);
        addBlock2(2,0,2,1,1,0,0,2,0, 1,0,0);
        addBlock2(2,0,2,1,1,0,0,2,2, 1,0,0);

        // 2连两端被堵 [3,4]
        addBlock2(0,0,2,1,1,2,0,0,0, 0,0,0);
        addBlock2(0,0,2,1,1,2,0,0,2, 0,0,0);
        addBlock2(0,0,2,1,1,2,0,2,0, 0,0,0);
        addBlock2(0,0,2,1,1,2,0,2,2, 0,0,0);
        addBlock2(2,0,2,1,1,2,0,0,0, 0,0,0);
        addBlock2(2,0,2,1,1,2,0,0,2, 0,0,0);
        addBlock2(2,0,2,1,1,2,0,2,0, 0,0,0);
        addBlock2(2,0,2,1,1,2,0,2,2, 0,0,0);

        // 2连一端被堵 [4,5]
        addBlock2(0,0,0,0,1,1,2,0,0, 1,0,0);
        addBlock2(0,0,0,0,1,1,2,0,2, 1,0,0);
        addBlock2(0,0,0,0,1,1,2,2,0, 1,0,0);
        addBlock2(0,0,0,0,1,1,2,2,2, 1,0,0);
        addBlock2(2,0,0,0,1,1,2,0,0, 1,0,0);
        addBlock2(2,0,0,0,1,1,2,0,2, 1,0,0);
        addBlock2(2,0,0,0,1,1,2,2,0, 1,0,0);
        addBlock2(2,0,0,0,1,1,2,2,2, 1,0,0);

        addBlock2(0,0,0,2,1,1,0,0,0, 1,0,0);
        addBlock2(0,0,0,2,1,1,0,0,2, 1,0,0);
        addBlock2(0,0,0,2,1,1,0,2,0, 1,0,0);
        addBlock2(0,0,0,2,1,1,0,2,2, 1,0,0);
        addBlock2(2,0,0,2,1,1,0,0,0, 1,0,0);
        addBlock2(2,0,0,2,1,1,0,0,2, 1,0,0);
        addBlock2(2,0,0,2,1,1,0,2,0, 1,0,0);
        addBlock2(2,0,0,2,1,1,0,2,2, 1,0,0);

        // 2连两端被堵 [4,5]
        addBlock2(0,0,0,2,1,1,2,0,0, 0,0,0);
        addBlock2(0,0,0,2,1,1,2,0,2, 0,0,0);
        addBlock2(0,0,0,2,1,1,2,2,0, 0,0,0);
        addBlock2(0,0,0,2,1,1,2,2,2, 0,0,0);
        addBlock2(2,0,0,2,1,1,2,0,0, 0,0,0);
        addBlock2(2,0,0,2,1,1,2,0,2, 0,0,0);
        addBlock2(2,0,0,2,1,1,2,2,0, 0,0,0);
        addBlock2(2,0,0,2,1,1,2,2,2, 0,0,0);

        // ===== P1 FIX: 跳活二间隙被填后的BLOCK2模式 =====
        // 跳活二 101 (X_X) - 间隙[4]被填充 → 分散为 XO X 或 X OX
        // 形态: _ _ X O X _ _ _
        addBlock2(0,0,0,1,2,1,0,0,0, 0,0,0);  // 基本形态
        addBlock2(0,0,0,1,2,1,0,0,2, 0,0,0);
        addBlock2(0,0,0,1,2,1,0,2,0, 0,0,0);
        addBlock2(0,0,0,1,2,1,0,2,2, 0,0,0);
        addBlock2(0,0,0,1,2,1,2,0,0, 0,0,0);  // 右紧邻[6]被封
        addBlock2(0,0,0,1,2,1,2,0,2, 0,0,0);
        addBlock2(0,0,0,1,2,1,2,2,0, 0,0,0);
        addBlock2(0,0,0,1,2,1,2,2,2, 0,0,0);
        addBlock2(2,0,0,1,2,1,0,0,0, 0,0,0);
        addBlock2(2,0,0,1,2,1,0,0,2, 0,0,0);
        addBlock2(2,0,0,1,2,1,0,2,0, 0,0,0);
        addBlock2(2,0,0,1,2,1,0,2,2, 0,0,0);
        addBlock2(2,0,0,1,2,1,2,0,0, 0,0,0);  // 左+右紧邻
        addBlock2(2,0,0,1,2,1,2,0,2, 0,0,0);
        addBlock2(2,0,0,1,2,1,2,2,0, 0,0,0);
        addBlock2(2,0,0,1,2,1,2,2,2, 0,0,0);

        // 跳活二 1001 (X__X) - 间隙[4,5]任填其一 → 分散形态
        // 形态: _ _ X O _ X _ _ (填第一个间隙)
        addBlock2(0,0,0,1,2,0,1,0,0, 0,0,1);
        addBlock2(0,0,0,1,2,0,1,0,2, 0,0,1);
        addBlock2(0,0,0,1,2,0,1,2,0, 0,0,1);
        addBlock2(0,0,0,1,2,0,1,2,2, 0,0,1);
        addBlock2(2,0,0,1,2,0,1,0,0, 0,0,1);
        addBlock2(2,0,0,1,2,0,1,0,2, 0,0,1);
        addBlock2(2,0,0,1,2,0,1,2,0, 0,0,1);
        addBlock2(2,0,0,1,2,0,1,2,2, 0,0,1);

        // 跳活二 101 偏移 - 间隙被填
        // 形态: _ _ _ X O X _ _
        addBlock2(0,0,0,0,1,2,1,0,0, 0,0,0);
        addBlock2(0,0,0,0,1,2,1,0,2, 0,0,0);
        addBlock2(2,0,0,0,1,2,1,0,0, 0,0,0);
        addBlock2(2,0,0,0,1,2,1,0,2, 0,0,0);

        // ===== P3 FIX: BLOCK2 跳眠二完整定义补充 =====
    }

public:
    PatternDB(const PatternDB&)=delete;
    PatternDB& operator=(const PatternDB&)=delete;

    static PatternDB& instance() {
        static PatternDB db;
        return db;
    }

    uint32_t encodeLine(const int line[PATTERN_WINDOW],int playerValue) const {
        int cells[PATTERN_WINDOW];
        for (int i=0;i<PATTERN_WINDOW;i++){
            if (playerValue==1){
                cells[i]=line[i];
            } else {
                if (line[i]==1) cells[i]=2;
                else if (line[i]==2) cells[i]=1;
                else cells[i]=line[i];
            }
        }
        return makeKey(cells);
    }

    const PatternInfo* lookup(uint32_t key) const {
        auto it=table.find(key);
        if (it!=table.end()) return &(it->second);
        return nullptr;
    }
};

#endif