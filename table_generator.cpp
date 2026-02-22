#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>

inline bool isStraight(int a, int b, int c, int d, int e){
    std::vector<int> cards = {a, b, c, d, e};
    std::sort(cards.begin(), cards.end());
    if(cards[0] == 0 && cards[1] == 9 && cards[2] == 10 && cards[3] == 11 && cards[4] == 12){
        return true;
    }
    for(int i=0; i<4; i++){
        if(cards[i] != cards[i+1]-1){
            return false;
        }
    }
    return true;
}

int main() 
{
    std::ofstream outFile("table");

    const std::vector<int> numTable = {1,2,3,5,7,11,13,17,19,23,29,31,37};
    std::vector<int> card(13,0);
    for(int a=0; a<13; a++)
    {
        if(a>0){card[a-1]--;}
        card[a]++;
        for(int b=a; b<13; b++)
        {
            if(b>a){card[b-1]--;}
            card[b]++;
            for(int c=b; c<13; c++)
            {
                if(c>b){card[c-1]--;}
                card[c]++;
                for(int d=c; d<13; d++)
                {
                    if(d>c){card[d-1]--;}
                    card[d]++;
                    for(int e=d; e<13; e++)
                    {
                        if(e>d){card[e-1]--;}
                        card[e]++;
                        if(card[e] > 4){
                            //不合法的牌型
                            outFile<<numTable[a]*numTable[b]*numTable[c]*numTable[d]*numTable[e]<<" "<<-1<<std::endl;
                            continue;
                        }
                        //同花和皇家同花是9和10，但是不在这里算，在服务器算同花，下面也是
                        if(card[d] == 4||card[e] == 4){
                            //四条
                            outFile<<numTable[a]*numTable[b]*numTable[c]*numTable[d]*numTable[e]<<" "<<8<<std::endl;
                        }
                        else if((card[a] == 3||card[b] == 3||card[c] == 3||card[d] == 3||card[e] == 3) && (card[a] == 2||card[b] == 2||card[c] == 2||card[d] == 2||card[e] == 2)){
                            //葫芦
                            outFile<<numTable[a]*numTable[b]*numTable[c]*numTable[d]*numTable[e]<<" "<<7<<std::endl;
                        }//同花是6
                        else if(isStraight(a, b, c, d, e)){
                            //顺子
                            outFile<<numTable[a]*numTable[b]*numTable[c]*numTable[d]*numTable[e]<<" "<<5<<std::endl;
                        }
                        else if(card[a] == 3||card[b] == 3||card[c] == 3||card[d] == 3||card[e] == 3){
                            //三条
                            outFile<<numTable[a]*numTable[b]*numTable[c]*numTable[d]*numTable[e]<<" "<<4<<std::endl;
                        }
                        else if((card[a] == 2&&card[b] == 2)||(card[a] == 2&&card[c] == 2)||(card[a] == 2&&card[d] == 2)||(card[a] == 2&&card[e] == 2)||(card[b] == 2&&card[c] == 2)||(card[b] == 2&&card[d] == 2)||(card[b] == 2&&card[e] == 2)||(card[c] == 2&&card[d] == 2)||(card[c] == 2&&card[e] == 2)||(card[d] == 2&&card[e] == 2)){
                            //两对
                            outFile<<numTable[a]*numTable[b]*numTable[c]*numTable[d]*numTable[e]<<" "<<3<<std::endl;
                        }
                        else if(card[a] == 2||card[b] == 2||card[c] == 2||card[d] == 2||card[e] == 2){
                            //一对
                            outFile<<numTable[a]*numTable[b]*numTable[c]*numTable[d]*numTable[e]<<" "<<2<<std::endl;
                        }
                        else if(card[a] == 1&&card[b] == 1&&card[c] == 1&&card[d] == 1&&card[e] == 1){

                            outFile<<numTable[a]*numTable[b]*numTable[c]*numTable[d]*numTable[e]<<" "<<1<<std::endl;
                            //高牌
                        }
                    }
                    card[12]--;
                }
                card[12]--;
            }
            card[12]--;
        }
        card[12]--;
    }
    for (int i = 0; i < 13; i++)
    {
        std::cout<<card[i]<<" ";
    }
    std::cout<<std::endl;
    std::cout<<"Finished "<<std::endl;
    



    return 0;
}