#pragma once
/*
Dealer类声明
*/

#ifndef DEALER_H
#define DEALER_H

#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

// 前向声明
class Player;
struct Pot;
class Dealer;

class typeTable {
private:
    static std::unordered_map<long long, short> typeMap;

    friend class Dealer;
    friend void init_table();
};

// 初始化牌型表
void init_table();

class Dealer {
private:
    int dealerId;
    int playerCount;
    int activePlayerCount;
    std::vector<bool> activePlayer;
    std::vector<bool> allinPlayer;
    std::vector<Pot> pots;
    std::vector<int> allinSequence;
    int currentBet;
    int sbId;
    std::vector<short> privateCards;
    std::vector<int> playerfdString;
    
    static constexpr int oddList[52] = {
    2,   3,   5,   7,   // 2  (花色0-3)
    11,  13,  17,  19,  // 3
    23,  29,  31,  37,  // 4
    41,  43,  47,  53,  // 5
    59,  61,  67,  71,  // 6
    73,  79,  83,  89,  // 7
    97,  101, 103, 107, // 8
    109, 113, 127, 131, // 9
    137, 139, 149, 151, // 10
    157, 163, 167, 173, // J
    179, 181, 191, 193, // Q
    197, 199, 211, 223, // K
    227, 229, 233, 239  // A
};

    void count();
    short rank5in7(short hand[2], std::vector<short> community_cards);
    void player_rank(std::vector<Player> players);
    void distribute();
    void rotate_blinds();

public:
    void init();
    void distribute();
    void count();
    void player_rank();
    void settle();
    void advance_phase();
    bool check_early_end();
    bool betting_round_over();
    void advance_actor();
    std::vector<uint8_t> serialize_state();

    int32_t player_call(const std::string& token);
    void player_allin(const std::string& token, int32_t amount);
    void player_fold(const std::string& token);
};

#endif // DEALER_H