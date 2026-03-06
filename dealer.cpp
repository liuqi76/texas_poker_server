/*
Dealer类成员和实现

包含 rank5in7、distribute、settle、count 等
*/

#include "dealer.h"
#include "types.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <random>

// 全局唯一
std::unordered_map<long long, short> typeTable::typeMap;

void init_table(){
    // 从table.txt中读取牌型表，初始化typeTable::typeMap
    // 牌型表格式：牌型id（素数积） 牌型排名
    std::fstream infile("./table.txt");
    int prime_product, rank;
    while (infile >> prime_product >> rank) {
        typeTable::typeMap[prime_product] = rank;
    }
}

// Dealer 类函数实现
void Dealer::count() {
        // 当一轮结束时，计算新的底池金额和边池金额
        // TODO: 实现边池计算逻辑
}

short Dealer::rank5in7(short hand[2], short community_cards[5]) {
    // 计算玩家最大可能牌型，并转换为牌型排名
    std::vector<int> all_cards = {hand[0], hand[1], community_cards[0], community_cards[1], 
                                    community_cards[2], community_cards[3], community_cards[4]};
    short max_rank = 1;
    for(int i = 0; i < 3; i++) {
        for(int j = i + 1; j < 4; j++) {
            for(int k = j + 1; k < 5; k++) {
                for(int l = k + 1; l < 6; l++) {
                    for(int m = l + 1; m < 7; m++) {
                        long long composeId = oddList[all_cards[i]] * oddList[all_cards[j]] * 
                                                oddList[all_cards[k]] * oddList[all_cards[l]] * 
                                                oddList[all_cards[m]];
                        short rank = typeTable::typeMap[composeId];

                        if (rank > max_rank) {
                            max_rank = rank;
                        }
                    }
                }
            }
        }
    }
    return max_rank;
}

std::vector<Player*> Dealer::rank_player(std::vector<Player*> players) {
    // 给每个玩家算牌力
    std::vector<std::pair<short, int>> rank_index;
    
    for (int i = 0; i < players.size(); i++) {
        const short* hand = players[i]->get_hand();
        short hand_arr[2] = {hand[0], hand[1]};
        short rank = rank5in7(hand_arr, community_cards);
        rank_index.push_back({rank, i});
    }
    
    // 按牌力从大到小排序
    std::sort(rank_index.begin(), rank_index.end(),
        [](const auto& a, const auto& b) {
            return a.first > b.first;
        });
    
    // 读取玩家排序，得到大->小
    std::vector<Player*> sorted_players;
    for (const auto& [rank, idx] : rank_index) {
        sorted_players.push_back(players[idx]);
    }
    
    return sorted_players;
}

void Dealer::distribute() {

    // 发牌，发公共牌和玩家数*2的手牌
    // TODO: 实现发牌逻辑
    // 初始化52张牌
    std::vector<int> deck(52);
    for (int i = 0; i < 52; i++) deck[i] = i;
    
    // Fisher-Yates 洗牌
    std::mt19937 rng(std::random_device{}());
    for (int i = 51; i > 0; i--) {
        std::uniform_int_distribution<int> dis(0, i);
        int j = dis(rng);
        std::swap(deck[i], deck[j]);
    }
}

void Dealer::rotate_blinds() {//done
    // 轮换小盲
    sbId = (sbId + 1) % playerCount;
}

// 改：初始化牌桌，Fisher-Yates洗牌，重置所有局内状态
void Dealer::init() {
// 重置玩家状态
    activePlayerCount = playerCount;
    activePlayer.assign(playerCount, true);
    allinPlayer.assign(playerCount, false);
    allinSequence.clear();

    // 重置下注状态
    currentBet = 0;

    // 重置底池
    pots.clear();
    pots.push_back({0, {}}); // 初始化一个空底池

    // 重置手牌和公共牌
    privateCards.clear();
    privateCards.resize(playerCount * 2);
    memset(community_cards, -1, sizeof(community_cards));

    // Fisher-Yates 洗牌
    std::vector<short> deck(52);
    for (int i = 0; i < 52; i++) deck[i] = i;

    std::mt19937 rng(std::random_device{}());
    for (int i = 51; i > 0; i--) {
        std::uniform_int_distribution<int> dis(0, i);
        int j = dis(rng);
        std::swap(deck[i], deck[j]);
    }

    // 发手牌，2n张存入privateCards
    for (int i = 0; i < playerCount * 2; i++) {
        privateCards[i] = deck[i];
    }

    // 发公共牌，取接下来5张备用
    for (int i = 0; i < 5; i++) {
        community_cards[i] = deck[playerCount * 2 + i];
    }

    // 轮换小盲
    rotate_blinds();
}

// 改：发手牌，逐玩家send DEAL_HOLECARDS（只发给对应fd）
void Dealer::distribute() {
    // TODO: 实现发牌逻辑
}

// 改：每一轮有人allin触发，边池计算，按allin金额从小到大切割，需要处理fold玩家contribution
void Dealer::count() {
    // TODO: 实现边池计算逻辑
}

// 改：逐pot找胜者分筹码，处理平局余量
void Dealer::settle(std::vector<Player*> ranked_player, std::vector<Pot*> pots) {
    // TODO: 实现结算逻辑
}

// 新增：推进到下一个GamePhase（揭牌或进入SHOWDOWN）
void Dealer::advance_phase() {
    // TODO: 实现阶段推进逻辑
}

// 新增：检查提前结束条件（A: 仅剩1活跃无allin；B: 有allin且活跃≤1）
bool Dealer::check_early_end() {
    // TODO: 实现提前结束检查逻辑
    return false;
}

// 新增：下注轮终止条件判断（所有activePlayer下注相等且均已行动）
bool Dealer::betting_round_over() {
    // TODO: 实现下注轮结束检查逻辑
    return false;
}

// 新增：推进currentActor到下一个activePlayer，发ACTION_REQUIRED，设定时器
void Dealer::advance_actor() {
    // TODO: 实现行动者推进逻辑
}

// 新增：构造并返回当前完整GameState快照的序列化字节流（重连时使用）
std::vector<uint8_t> Dealer::serialize_state() {
    // TODO: 实现状态序列化逻辑
    return std::vector<uint8_t>();
}

// 改：player_call，补全实际跟注金额计算逻辑，返回int32_t
int32_t Dealer::player_call(const std::string& token) {
    // TODO: 实现跟注逻辑
    return 0;
}

// 改：player_allin，触发边池重新计算
void Dealer::player_allin(const std::string& token, int32_t amount) {
    // TODO: 实现全押逻辑
}

// 改：标记fold，手牌作废
void Dealer::player_fold(const std::string& token) {
    // TODO: 实现弃牌逻辑
}
