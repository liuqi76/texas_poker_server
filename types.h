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

// 接收到的原始帧（解包后）
struct Frame {
    uint8_t  msgType;
    uint32_t seqId;
    std::vector<uint8_t> payload;  // 用 vector 代替 string，不截断二进制
};

// 投入任务队列的待处理请求任务单元
struct Task {
    uint8_t  msgType;
    uint32_t uid;
    uint32_t dealerId;   // 0 = 不属于任何局
    std::vector<uint8_t> payload;
};

// 各类 C2S payload 需定义，例如：
struct CreateRoomPayload {
    uint32_t roomId;
    int32_t  smallBlind;
};

struct ActionRaisePayload {
    int32_t totalBet;
};

// ...其余类似

struct Pot {
    int amount; // 底池金额
    std::vector<int> excluded_players; // 没有资格参与这个底池分配的玩家列表（all in的玩家）
};

enum suit { HEARTS = 0, DIAMONDS, CLUBS, SPADES }; // 花色，'H', 'D', 'C', 'S'

struct card
{
    enum suit color;
    int rank; // 牌面值，0-12分别代表2、3...10、J、Q、K、A
};

enum class RoomStatus : uint8_t {
    WAITING = 0,
    IN_GAME = 1
};

// 玩家状态枚举
enum PlayerStatus { 
    WAITING,    // 在局内但是还未轮到
    ACTIVE,     // 正在行动
    FOLDED,     // 已弃牌
    ALL_IN      // 全押
};

// 游戏阶段枚举
enum GamePhase {
    PREFLOP,
    FLOP,
    TURN,
    RIVER,
    SHOWDOWN
};

// 消息类型枚举
enum MsgType {
    // 连接相关
    CONN_REQ = 0x01,
    CONN_ACK = 0x02,
    RECONNECT_REQ = 0x03,
    RECONNECT_ACK = 0x04,
    HEARTBEAT = 0x05,
    HEARTBEAT_ACK = 0x06,
    
    // 房间相关
    CREATE_ROOM = 0x10,
    CREATE_ROOM_ACK = 0x11,
    JOIN_ROOM = 0x12,
    JOIN_ROOM_ACK = 0x13,
    LEAVE_ROOM = 0x14,
    ROOM_UPDATE = 0x15,
    START_GAME = 0x16,
    END_GAME = 0x17,
    
    // 游戏相关
    ACTION_REQUIRED = 0x20,
    PLAYER_ACTION = 0x21,
    GAME_STATE = 0x22,
    DEAL_HOLECARDS = 0x23,
    DEAL_COMMUNITY = 0x24,
    SETTLE_RESULT = 0x25,
    
    // 筹码相关
    REBUY_REQUEST = 0x30,
    REBUY_DONE = 0x31,
    
    // 系统相关
    TIMEOUT = 0x40,
    DISCONNECT = 0x41
};

//前向声明
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
