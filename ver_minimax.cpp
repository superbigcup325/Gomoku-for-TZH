#include"board.h"
#include"minimax.h"
#include<iostream>

void show(int currentTurn,Player p) {
    std::cout<<"current turn is "<<currentTurn<<std::endl;
    std::cout<<"current player is "<<(p==BLACK? "black":"white")<<std::endl;
}

int main () {
    Gomoku g(15);
    Minimax minimax(WHITE,4,1.12);
    int currentTurn=0;
    while (!g.GameOver()) {
        g.show();
        currentTurn++;
        int x,y;
        Player p=CurrentPlayer(currentTurn);
        show(currentTurn,p);
        if (p==minimax.getSelf()){
            auto move=minimax.getBestMove(g);
            x=move.first,y=move.second;
            if (x==-1||y==-1){
                std::cout<<"cant find"<<std::endl;
                break;
            }
            std::cout<<"placed at ("<<x<<", "<<y<<")"<<std::endl;
            g.set(x,y,p);
        }
        else {
            // 由人工输入
            // std::cin>>x>>y;
            // while(!g.validPosition(x,y,p)) {
            //     if (g.outOfRange(x,y)) std::cout<<"out of range"<<std::endl;
            //     else if (g.getPlayer(x,y)!=NONE) std::cout<<"is placed"<<std::endl;
            //     else if (g.isForbidden(x,y,p)) std::cout<<"forbiddened"<<std::endl;
            //     std::cout<<"please try again: ";
            //     std::cin>>x>>y;
            // }
            // g.set(x,y,p);
            // ai博弈
            auto move=minimax.getBestMove(g);
            x=move.first,y=move.second;
            if (x==-1||y==-1){
                std::cout<<"cant find"<<std::endl;
            }
            g.set(x,y,p);
        }
        if (g.Win(x,y,p)){
            std::cout<<(p==BLACK? "BLACK":"WHITE")<<" win"<<std::endl;
            g.show();
            break;
        }
    }
    return 0;
}
