/*
Room类成员和实现
*/

//#include "room.cpp"
#include "types.h"
#include "dealer.h"
#include <string>
#include <vector>
#include <algorithm>

class Room {
private:
    uint16_t   roomId;
    std::string ownerToken;          // 房主token，退出时顺延
    std::vector<std::string> players;// 按座位顺序存token
    RoomStatus status;
    int32_t    smallBlind;
    Dealer*    dealer;               // 游戏中时非空，等待时为nullptr

public:
    // 构造时传入房主token、房间号、小盲注位置
    Room(const std::string& ownerToken, uint16_t roomId, int32_t smallBlind) 
        : ownerToken(ownerToken), roomId(roomId), smallBlind(smallBlind), 
          status(RoomStatus::WAITING), dealer(nullptr) {
        players.push_back(ownerToken);
    }

    // 新玩家加入：将玩家加入players列表，广播ROOM_UPDATE
    bool join(const std::string& token) {
        // 检查玩家是否已在房间中
        if (std::find(players.begin(), players.end(), token) != players.end()) {
            return false;
        }
        
        // 检查房间是否已满（这里假设最大玩家数为8）
        if (players.size() >= 8) {
            return false;
        }
        
        players.push_back(token);
        return true;
    }

    // 玩家退出：将玩家移出players列表，处理房主顺延，返回是否需要销毁房间
    bool leave(const std::string& token) {
        auto it = std::find(players.begin(), players.end(), token);
        if (it == players.end()) {
            return false;
        }
        
        players.erase(it);
        
        // 如果退出的是房主，需要顺延房主
        if (token == ownerToken && !players.empty()) {
            ownerToken = players[0];
        }
        
        // 如果房间为空，返回true表示需要销毁房间
        return players.empty();
    }

    // 房主退出：顺延房主给players[0]
    void transfer_owner() {
        if (!players.empty()) {
            ownerToken = players[0];
        }
    }

    // 游戏开始初始化：创建Dealer实例，提交init Task，status置为IN_GAME
    void start_game() {
        if (status == RoomStatus::WAITING && !players.empty()) {
            dealer = new Dealer();
            status = RoomStatus::IN_GAME;
            // TODO: 提交初始化Task
        }
    }

    // 游戏结束：销毁Dealer，房间的status置回WAITING
    void end_game() {
        if (dealer != nullptr) {
            delete dealer;
            dealer = nullptr;
        }
        status = RoomStatus::WAITING;
    }

    // 公共事件：构造并广播ROOM_UPDATE帧给房间内所有玩家
    void broadcast_room_update() {
        // TODO: 实现房间更新广播
    }

    // 供外部查询
    bool is_owner(const std::string& token) const {
        return token == ownerToken;
    }

    bool is_full() const {
        return players.size() >= 8; // 假设最大玩家数为8
    }

    RoomStatus get_status() const {
        return status;
    }

    // 获取房间ID
    uint16_t get_room_id() const {
        return roomId;
    }

    // 获取房主token
    const std::string& get_owner_token() const {
        return ownerToken;
    }

    // 获取玩家列表
    const std::vector<std::string>& get_players() const {
        return players;
    }

    // 获取小盲注
    int32_t get_small_blind() const {
        return smallBlind;
    }

    // 获取Dealer指针
    Dealer* get_dealer() const {
        return dealer;
    }
};

// 全局房间管理函数

// 新增：创建RoomInstance，写入全局map，绑定房主
void room_create(const std::string& token, uint16_t roomId, int32_t smallBlind) {
    std::lock_guard<std::mutex> lock(room_map_mutex);
    room_map[roomId] = new Room(token, roomId, smallBlind);
}

// 新增：将玩家加入房间，广播ROOM_UPDATE
void room_join(const std::string& token, uint16_t roomId) {
    std::lock_guard<std::mutex> lock(room_map_mutex);
    auto it = room_map.find(roomId);
    if (it != room_map.end()) {
        if (it->second->join(token)) {
            it->second->broadcast_room_update();
        }
    }
}

// 新增：将玩家移出房间，处理房主顺延，房间空则销毁
void room_leave(const std::string& token) {
    std::lock_guard<std::mutex> lock(room_map_mutex);
    
    // 查找玩家所在的房间
    for (auto it = room_map.begin(); it != room_map.end(); ) {
        Room* room = it->second;
        const auto& players = room->get_players();
        
        if (std::find(players.begin(), players.end(), token) != players.end()) {
            bool shouldDestroy = room->leave(token);
            
            if (shouldDestroy) {
                delete room;
                it = room_map.erase(it);
            } else {
                room->broadcast_room_update();
                ++it;
            }
            break;
        } else {
            ++it;
        }
    }
}

// 新增：销毁RoomInstance，清理相关资源
void room_destroy(uint16_t roomId) {
    std::lock_guard<std::mutex> lock(room_map_mutex);
    auto it = room_map.find(roomId);
    if (it != room_map.end()) {
        delete it->second;
        room_map.erase(it);
    }
}
