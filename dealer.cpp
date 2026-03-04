/*
Dealer类成员和实现

包含 rank5in7、distribute、settle、count 等
*/

#include "dealer.h"
#include "types.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <random>

// typeTable 类实现
class typeTable {
private:
    static std::map<long long, short> typeMap;

    friend class Dealer;
    friend void init_table();
};

std::map<long long, short> typeTable::typeMap;

void init_table(){
    // 从table.txt中读取牌型表，初始化typeTable::typeMap
    // 牌型表格式：牌型id（素数积） 牌型排名
    std::fstream infile("./table.txt");
    int prime_product, rank;
    while (infile >> prime_product >> rank) {
        typeTable::typeMap[prime_product] = rank;
    }
}

// Dealer 类实现
class Dealer {
private:
    int dealerId;
    int playerCount;                 // 用于轮换sb和bb
    int activePlayerCount;           // 表示当前还未folded、all in的玩家数量，用于计算底池和边池
    std::vector<bool> activePlayer;  // 表示玩家是否还未folded
    std::vector<bool> allinPlayer;   // 表示玩家是否已经all in
    std::vector<Pot> pots;           // 池串，第一个是底池，默认所有人瓜分，后面是边池，需要参考exempt_players来分配
    std::vector<int> allinSequence;  // all in顺序串，记录每个玩家all in的顺序，用于计算边池分配
    int currentBet;                  // 当前轮的最高下注金额
    int sbId;                        // 小盲玩家id（其实是index）
    std::vector<short> privateCards; // 手牌串，01是玩家一的，23是玩家二的，以此类推。每局开始时清理，不会造成因玩家退出导致错位
    std::vector<int> playerfdString; // 记录所有玩家的fd，一局结束之前不会
    
    static constexpr int oddList[52] = {
        1, 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 
        59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 
        137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 
        227, 229, 233
    };//质数表，用于生成牌型id

    void count() {
        // 当一轮结束时，计算新的底池金额和边池金额
        // TODO: 实现边池计算逻辑
    }

    short rank5in7(short hand[2], std::vector<short> community_cards) {
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
    
    void player_rank(std::vector<Player> players) {
        // 结算函数，根据玩家的牌型生成排名串
        // TODO: 实现玩家排名逻辑
    }

    void distribute() {
        // 发牌，发公共牌和玩家数*2的手牌
        // TODO: 实现发牌逻辑
    }

    void rotate_blinds() {
        // 轮换小盲和大盲
        sbId = (sbId + 1) % playerCount;
    }

public:
    // 改：初始化牌桌，Fisher-Yates洗牌，重置所有局内状态
    void init() {
        // TODO: 实现初始化逻辑
    }

    // 改：发手牌，逐玩家send DEAL_HOLECARDS（只发给对应fd）
    void distribute() {
        // TODO: 实现发牌逻辑
    }

    // 改：边池计算，按allin金额从小到大切割，处理fold玩家contribution
    void count() {
        // TODO: 实现边池计算逻辑
    }

    // 改：对所有alivePlayer调用rank5in7，生成排名列表
    void player_rank() {
        // TODO: 实现玩家排名逻辑
    }

    // 改：逐pot找胜者分配筹码，处理平局余量，广播SETTLE_RESULT
    void settle() {
        // TODO: 实现结算逻辑
    }

    // 新增：推进到下一个GamePhase（揭牌或进入SHOWDOWN）
    void advance_phase() {
        // TODO: 实现阶段推进逻辑
    }

    // 新增：检查提前结束条件（A: 仅剩1活跃无allin；B: 有allin且活跃≤1）
    bool check_early_end() {
        // TODO: 实现提前结束检查逻辑
        return false;
    }

    // 新增：下注轮终止条件判断（所有activePlayer下注相等且均已行动）
    bool betting_round_over() {
        // TODO: 实现下注轮结束检查逻辑
        return false;
    }

    // 新增：推进currentActor到下一个activePlayer，发ACTION_REQUIRED，设定时器
    void advance_actor() {
        // TODO: 实现行动者推进逻辑
    }

    // 新增：构造并返回当前完整GameState快照的序列化字节流（重连时使用）
    std::vector<uint8_t> serialize_state() {
        // TODO: 实现状态序列化逻辑
        return std::vector<uint8_t>();
    }

    // 改：player_call，补全实际跟注金额计算逻辑，返回int32_t
    int32_t player_call(const std::string& token) {
        // TODO: 实现跟注逻辑
        return 0;
    }

    // 改：player_allin，触发边池重新计算
    void player_allin(const std::string& token, int32_t amount) {
        // TODO: 实现全押逻辑
    }

    // 改：标记fold，手牌作废
    void player_fold(const std::string& token) {
        // TODO: 实现弃牌逻辑
    }
};
