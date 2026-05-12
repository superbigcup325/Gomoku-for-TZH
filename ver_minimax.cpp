#include"board.h"
#include"minimax.h"
#include<iostream>

void show(int currentTurn,Player p) {
    std::cout<<"current turn is "<<currentTurn<<std::endl;
    std::cout<<"current player is "<<(p==BLACK? "black":"white")<<std::endl;
}

int main () {
    Gomoku g(15);
    // Minimax white(WHITE,4,1.12);
    Minimax black(BLACK,4,1.12);
    int currentTurn=0;
    int lastX=0,lastY=0;
    Player lastPlayer=NONE;
    while (!g.GameOver()) {
        g.show();
        currentTurn++;
        int x,y;
        Player p=CurrentPlayer(currentTurn);
        show(currentTurn,p);
        if (p==black.getSelf()){
            auto move=black.getBestMove(g);
            x=move.first,y=move.second;
            if (x==-1||y==-1){
                std::cout<<"cant find"<<std::endl;
                break;
            }
            std::cout<<"placed at ("<<x<<", "<<y<<")"<<std::endl;
            g.set(x,y,p);
            lastX=x; lastY=y; lastPlayer=p;
        }
        else {
            // 由人工输入
            std::cin>>x>>y;
            while(!g.validPosition(x,y,p)) {
                if (g.outOfRange(x,y)) std::cout<<"out of range"<<std::endl;
                else if (g.getColor(x,y)!=NONE) std::cout<<"is placed"<<std::endl;
                else if (g.isForbidden(x,y,p)) std::cout<<"forbiddened"<<std::endl;
                std::cout<<"please try again: ";
                std::cin>>x>>y;
            }
            g.set(x,y,p);
            // ai博弈
            // auto move=white.getBestMove(g);
            // x=move.first,y=move.second;
            // if (x==-1||y==-1){
            //     std::cout<<"cant find"<<std::endl;
            // }
	        // std::cout<<"placed at ("<<x<<", "<<y<<")"<<std::endl;
            // g.set(x,y,p);
            // lastX=x; lastY=y; lastPlayer=p;
        }
        if (g.Win(x,y,p)){
            std::cout<<(p==BLACK? "BLACK":"WHITE")<<" win"<<std::endl;
            break;
        }
    }
    if (g.GameOver() && lastPlayer!=NONE && !g.Win(lastX,lastY,lastPlayer)) {
        std::cout<<"draw"<<std::endl;
    }
    return 0;
}
