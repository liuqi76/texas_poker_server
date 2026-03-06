/*
Player类成员和实现
*/

#include "player.h"
#include "types.h"
#include <string>

class Player {
public:
    int playerid;
    std::string name;
    
private:
    short chips;
    short hand[2]; // 手牌
    short rebuycount;
    short currentBet; // 当前轮已经下注的金额
    enum status { WAITING, ACTIVE, FOLDED, ALL_IN } playerStatus; // 这里的waiting表示在局内但是还未轮到，active表示正在行动
    uint16_t roomId;

public:
    int rebuy(int amount) {
        // 只请求，校验移到handle_rebuy
        // TODO: 实现重购逻辑
        return 0;
    }

    // 获取玩家状态
    status get_status() const {
        return playerStatus;
    }

    // 设置玩家状态
    void set_status(status newStatus) {
        playerStatus = newStatus;
    }

    // 获取房号
    uint16_t get_roomId() const {
        return roomId;
    }

    // 获取筹码数量
    short get_chips() const {
        return chips;
    }

    // 设置筹码数量
    void set_chips(short newChips) {
        chips = newChips;
    }

    // 获取当前下注金额
    short get_current_bet() const {
        return currentBet;
    }

    // 设置当前下注金额
    void set_current_bet(short bet) {
        currentBet = bet;
    }

    // 获取手牌
    const short* get_hand() const {
        return hand;
    }

    // 设置手牌
    void set_hand(short card1, short card2) {
        hand[0] = card1;
        hand[1] = card2;
    }

    // 获取重购次数
    short get_rebuycount() const {
        return rebuycount;
    }

    // 增加重购次数
    void increment_rebuycount() {
        rebuycount++;
    }
};
