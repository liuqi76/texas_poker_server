#pragma once
/*
全局项目使用的基础类型

包含所有结构体（PlayerInstance、Task、Frame、Card、Pot等）、
枚举（PlayerStatus、RoomStatus、GamePhase、MsgType等）、
全局 map 和 extern 声明、全局 mutex 和 extern 声明
*/

#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <chrono>

// 投入任务队列的待处理请求任务单元
// 接收到的原始帧（解包后）
struct Frame {
    MsgType  msgType;
    uint32_t seqId;
    std::vector<uint8_t> payload;  // 用 vector 代替 string，不截断二进制
};

// 投入任务队列的待处理请求任务单元
struct Task {
    MsgType  msgType;
    uint32_t uid;
    uint32_t dealerId;   // 0 = 不属于任何局
    std::vector<uint8_t> payload;
};

// 各类 C2S payload 需定义，例如：
struct CreateRoomPayload {//done
    //创建房间功能，需要房间id
    uint16_t roomId;
    //int32_t  smallBlind;      不需要，默认房主+1开始
};

// ... existing code ...
// 待定义的消息类型payload

struct ActionRaisePayload {//done
    uint32_t TotalBet;//raise之后的量
};

// 其他 C2S/S2C payload 结构体定义
struct CreateRoomAckPayload {//done
    //如果没有冲突则是原房号
    uint16_t actualRoomId; // 实际分配的房间ID（避免房号冲突）
};

struct JoinRoomPayload {
    uint16_t roomId; // 要加入的房间ID
};

#pragma pack(push, 1)
struct PlayerActionPayload {
    //不加 #pragma pack(1) 时，编译器会在 uint8_t 后面悄悄填充 3 个字节，让 uint32_t 对齐到 4 字节边界
    // 玩家行动类型：0=弃牌, 1=过牌, 2=跟注, 3=加注, 4=全押
    uint32_t actionType:3;
    // 如果是加注，表示加注后的总下注量
    // 如果是全押，表示全押的筹码量
    uint32_t amount:29;
};

struct DealHolecardsPayload {
    // 底牌信息，包含2张牌
    // 直接发Id，
    uint8_t holeCards1;
    uint8_t holeCards2;
};

struct DealCommunityPayload {
    // 公共牌信息，根据游戏阶段不同揭示3、4、5张牌，未揭示的牌是-1
    // card communityCards[5]; // 最多5张公共牌
    uint8_t phase; // 游戏阶段：0=FLOP, 1=TURN, 2=RIVER
    uint8_t cardId1;
    uint8_t cardId2;
    uint8_t cardId3;
    uint8_t cardId4 = -1;//后两张先默认-1
    uint8_t cardId5 = -1;
};
#pragma pack(pop)

struct SettleResultPayload {
    // 结算结果，包含赢家数组和金额数组
    std::vector<uint32_t> winnerUid;
    std::vector<uint32_t> winAmount;
};

struct RebuyRequestPayload {
    uint32_t requestAmount; // 请求重买的筹码数量
};

struct RebuyDonePayload {
    uint32_t actualAmount; // 实际重买的筹码数量（可能不是希望的数量）
};

struct ConnectReqPayload {
    std::string nickname;
};

struct ReconnectReqPayload {
    uint64_t oldToken;

};

struct Pot {
    int amount; // 底池金额
    std::vector<int> excluded_players; // 没有资格参与这个底池分配的玩家列表（已经all in的玩家）
};

// ... rest of code ...


enum suit { HEARTS = 0, DIAMONDS, CLUBS, SPADES }; // 花色，'H', 'D', 'C', 'S'

struct card
{
    enum suit color;
    int rank; // 牌面值，0-12分别代表2、3...10、J、Q、K、A
    int Id; // 52张的独立id，牌序也是2、3...
};

enum class RoomStatus : uint8_t {
    WAITING = 0,
    IN_GAME = 1
};

// 玩家状态枚举
enum PlayerStatus { //已完成
    WAITING,    // 在局内但是还未轮到
    ACTIVE,     // 正在行动
    FOLDED,     // 已弃牌
    ALL_IN      // 全押
};

// 游戏阶段枚举
enum GamePhase {
    PREFLOP,    // 翻牌前
    FLOP,       // 翻牌
    TURN,       // 第四张
    RIVER,      // 第五张
    SHOWDOWN    // 结算展示
};

// 消息类型枚举
enum MsgType {
    // 连接相关
    CONN_REQ = 0x01,//握手类型1，表示是新连接done
    CONN_ACK = 0x02,//确认新连接握手，无payload
    RECONNECT_REQ = 0x03,//握手类型2，表示是重新连接done
    RECONNECT_ACK = 0x04,//确认重连接受，无payload
    HEARTBEAT = 0x05,//C2S定时心跳，无payload
    HEARTBEAT_ACK = 0x06,//S2C心跳确认，无payload
    ACCEPT = 0x07,//连接建立，仅用于S2C的ack帧（通知客户端发握手包），无payload
    
    // 房间相关
    CREATE_ROOM = 0x10,//大厅建房间，payload是房号done
    CREATE_ROOM_ACK = 0x11,//建房成功，payload是实际房号（避免房号冲突）done
    JOIN_ROOM = 0x12,//加入房间，payload是房号done
    JOIN_ROOM_ACK = 0x13,//加入成功，无payload
    LEAVE_ROOM = 0x14,//退出房间，无payload
    ROOM_UPDATE = 0x15,//新玩家加入时广播，payload结构待定义!!
    START_GAME = 0x16,//房主开始游戏，无payload
    END_GAME = 0x17,//房主结束游戏，无payload
    
    // 游戏相关
    ACTION_REQUIRED = 0x20,//S2C提示行动，无payload
    PLAYER_ACTION = 0x21,//玩家行动，payload结构待定义done
    GAME_STATE = 0x22,//S2C广播房间状态更新
    DEAL_HOLECARDS = 0x23,//S2C发送底牌，payload结构待定义done
    DEAL_COMMUNITY = 0x24,//S2C广播公共牌done
    SETTLE_RESULT = 0x25,//S2C广播结算结果，payload结构待定义done
    
    // rebuy相关
    REBUY_REQUEST = 0x30,//C2S，请求重买，payload是数量done
    REBUY_DONE = 0x31,//S2C，重买完成，payload是实际数量（可能不是希望的数量）done
    
    // 系统相关
    TIMEOUT = 0x40,//S2C，提醒回合超时，无payload
    DISCONNECT = 0x41//暂时未定义
};

// Room前向声明
class Room;

// 全局映射表声明
extern std::map<int, std::string> fd_to_token;      // fd -> token
extern std::map<std::string, int> token_to_fd;      // token -> fd
extern std::map<std::string, uint32_t> token_to_uid; // token -> uid
extern std::map<uint32_t, Room*> room_map;          // roomId -> Room*
extern std::map<std::string, std::chrono::steady_clock::time_point> last_active_time;

// 全局互斥锁声明
extern std::mutex fd_token_mutex;
extern std::mutex room_map_mutex;
extern std::mutex task_queue_mutex;
extern std::mutex timer_mutex;

#endif // TYPES_H
