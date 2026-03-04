/*
业务逻辑处理函数

dispatch_task 和全部 handle_* 函数
*/

#include "handler.h"
#include "types.h"
#include "room.h"
#include "dealer.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

// 新增：worker取到Task后的总路由入口，按msgType分发
void dispatch_task(Task task) {
    switch (task.msgType) {
        case CONN_REQ:
            handle_conn_req(std::to_string(task.uid), task.payload);
            break;
        case RECONNECT_REQ:
            // 注意：RECONNECT_REQ需要fd信息，这里需要特殊处理
            break;
        case HEARTBEAT:
            handle_heartbeat(std::to_string(task.uid));
            break;
        case CREATE_ROOM:
            handle_create_room(std::to_string(task.uid), task.payload);
            break;
        case JOIN_ROOM:
            handle_join_room(std::to_string(task.uid), task.payload);
            break;
        case LEAVE_ROOM:
            handle_leave_room(std::to_string(task.uid));
            break;
        case START_GAME:
            handle_start_game(std::to_string(task.uid));
            break;
        case PLAYER_ACTION:
            handle_player_action(std::to_string(task.uid), task.payload);
            break;
        case REBUY_REQUEST:
            handle_rebuy(std::to_string(task.uid), task.payload);
            break;
        case REBUY_DONE:
            handle_rebuy_done(std::to_string(task.uid));
            break;
        case END_GAME:
            handle_end_game(std::to_string(task.uid));
            break;
        case TIMEOUT:
            handle_timeout(std::to_string(task.uid));
            break;
        case DISCONNECT:
            handle_disconnect_task(std::to_string(task.uid));
            break;
        default:
            std::cerr << "Unknown message type: " << static_cast<int>(task.msgType) << std::endl;
            break;
    }
}

// 新增：处理CONN_REQ，更新nickname
void handle_conn_req(const std::string& token, const std::vector<uint8_t>& payload) {
    // TODO: 实现连接请求处理
    std::cout << "处理连接请求，token: " << token << std::endl;
}

// 新增：处理RECONNECT_REQ，验证token，替换fd，发GAME_STATE快照
void handle_reconnect(int new_fd, const std::vector<uint8_t>& payload) {
    // TODO: 实现重连请求处理
    std::cout << "处理重连请求，fd: " << new_fd << std::endl;
}

// 新增：处理HEARTBEAT，更新最后活跃时间戳，回HEARTBEAT_ACK
void handle_heartbeat(const std::string& token) {
    // 更新最后活跃时间
    std::lock_guard<std::mutex> lock(fd_token_mutex);
    last_active_time[token] = std::chrono::steady_clock::now();
    
    // TODO: 发送HEARTBEAT_ACK
    std::cout << "处理心跳，token: " << token << std::endl;
}

// 新增：处理CREATE_ROOM
void handle_create_room(const std::string& token, const std::vector<uint8_t>& payload) {
    // 解析payload
    if (payload.size() < sizeof(CreateRoomPayload)) {
        std::cerr << "Invalid CREATE_ROOM payload size" << std::endl;
        return;
    }
    
    const CreateRoomPayload* createPayload = reinterpret_cast<const CreateRoomPayload*>(payload.data());
    uint32_t roomId = createPayload->roomId;
    int32_t smallBlind = createPayload->smallBlind;
    
    // 创建房间
    room_create(token, roomId, smallBlind);
    
    std::cout << "房间创建，token: " << token << ", roomId: " << roomId 
              << ", smallBlind: " << smallBlind << std::endl;
}

// 新增：处理JOIN_ROOM
void handle_join_room(const std::string& token, const std::vector<uint8_t>& payload) {
    // 解析payload获取roomId
    if (payload.size() < sizeof(uint32_t)) {
        std::cerr << "Invalid JOIN_ROOM payload size" << std::endl;
        return;
    }
    
    uint32_t roomId = *reinterpret_cast<const uint32_t*>(payload.data());
    
    // 加入房间
    room_join(token, roomId);
    
    std::cout << "玩家加入房间，token: " << token << ", roomId: " << roomId << std::endl;
}

// 新增：处理LEAVE_ROOM，含房主顺延和房间销毁逻辑
void handle_leave_room(const std::string& token) {
    room_leave(token);
    
    std::cout << "玩家离开房间，token: " << token << std::endl;
}

// 新增：处理START_GAME，校验房主，创建DealerInstance，提交初始化Task
void handle_start_game(const std::string& token) {
    // 查找玩家所在的房间
    std::lock_guard<std::mutex> lock(room_map_mutex);
    
    for (auto& pair : room_map) {
        Room* room = pair.second;
        const auto& players = room->get_players();
        
        if (std::find(players.begin(), players.end(), token) != players.end()) {
            // 检查是否是房主
            if (room->is_owner(token)) {
                // 开始游戏
                room->start_game();
                std::cout << "游戏开始，房间: " << pair.first << std::endl;
            } else {
                std::cerr << "非房主尝试开始游戏，token: " << token << std::endl;
            }
            break;
        }
    }
}

// 新增：处理轮内行动（fold/check/call/raise/allin），校验当前行动者
void handle_player_action(const std::string& token, const std::vector<uint8_t>& payload) {
    // TODO: 实现玩家行动处理
    // 需要解析行动类型和金额，验证当前行动者，调用Dealer的相应方法
    std::cout << "处理玩家行动，token: " << token << std::endl;
}

// 新增：处理REBUY_REQUEST
void handle_rebuy(const std::string& token, const std::vector<uint8_t>& payload) {
    // TODO: 实现重购请求处理
    std::cout << "处理重购请求，token: " << token << std::endl;
}

// 新增：处理REBUY_DONE，所有玩家确认后触发新局
void handle_rebuy_done(const std::string& token) {
    // TODO: 实现重购完成处理
    std::cout << "处理重购完成，token: " << token << std::endl;
}

// 新增：处理END_GAME（房主强制结束），立即结算并销毁DealerInstance
void handle_end_game(const std::string& token) {
    // 查找玩家所在的房间
    std::lock_guard<std::mutex> lock(room_map_mutex);
    
    for (auto& pair : room_map) {
        Room* room = pair.second;
        const auto& players = room->get_players();
        
        if (std::find(players.begin(), players.end(), token) != players.end()) {
            // 检查是否是房主
            if (room->is_owner(token)) {
                // 结束游戏
                room->end_game();
                std::cout << "游戏结束，房间: " << pair.first << std::endl;
            } else {
                std::cerr << "非房主尝试结束游戏，token: " << token << std::endl;
            }
            break;
        }
    }
}

// 新增：处理TIMEOUT Task，执行自动check或fold，更新连续超时计数
void handle_timeout(const std::string& token) {
    // TODO: 实现超时处理
    // 自动执行check或fold，更新超时计数
    std::cout << "处理超时，token: " << token << std::endl;
}

// 新增：处理DISCONNECT Task，按玩家当前状态执行离线逻辑
void handle_disconnect_task(const std::string& token) {
    // TODO: 实现断线处理
    // 根据玩家当前状态执行相应逻辑
    std::cout << "处理断线，token: " << token << std::endl;
}
