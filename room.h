#pragma once
/*
Room类声明
*/

#ifndef ROOM_H
#define ROOM_H

#include <cstdint>
#include <string>
#include <vector>
#include "types.h"

// 前向声明
class Dealer;

class Room {
private:
    uint32_t   roomId;
    std::string ownerToken;
    std::vector<std::string> players;
    RoomStatus status;
    int32_t    smallBlind;
    Dealer*    dealer;

public:
    Room(const std::string& ownerToken, uint32_t roomId, int32_t smallBlind);
    
    bool join(const std::string& token);
    bool leave(const std::string& token);
    void transfer_owner();
    void start_game();
    void end_game();
    void broadcast_room_update();
    
    bool is_owner(const std::string& token) const;
    bool is_full() const;
    RoomStatus get_status() const;
    uint32_t get_room_id() const;
    const std::string& get_owner_token() const;
    const std::vector<std::string>& get_players() const;
    int32_t get_small_blind() const;
    Dealer* get_dealer() const;
};

// 全局房间管理函数
void room_create(const std::string& token, uint32_t roomId, int32_t smallBlind);
void room_join(const std::string& token, uint32_t roomId);
void room_leave(const std::string& token);
void room_destroy(uint32_t roomId);

#endif // ROOM_H