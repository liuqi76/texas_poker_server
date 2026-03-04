/*
Player类声明
*/

#ifndef PLAYER_H
#define PLAYER_H

#include <string>

class Player {
public:
    int playerid;
    std::string name;
    
private:
    short chips;
    short hand[2];
    short rebuycount;
    short currentBet;
    enum status { WAITING, ACTIVE, FOLDED, ALL_IN } playerStatus;

public:
    int rebuy(int amount);
    
    status get_status() const;
    void set_status(status newStatus);
    short get_chips() const;
    void set_chips(short newChips);
    short get_current_bet() const;
    void set_current_bet(short bet);
    const short* get_hand() const;
    void set_hand(short card1, short card2);
    short get_rebuycount() const;
    void increment_rebuycount();
};

#endif // PLAYER_H