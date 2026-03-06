#pragma once
/*
业务逻辑处理函数声明
*/

#ifndef HANDLER_H
#define HANDLER_H

#include "types.h"
#include <vector>
#include <string>

// 任务分发
void dispatch_task(Task task);

// 转自network.cpp，处理新连接、处理加入队列、处理重连等
void handle_handshake(int client_fd, Frame frame);//判断新连接还是重连，以及玩家状态初始化
void handle_disconnect(int fd);


// 各种处理函数
Task frame_to_task(const Frame& frame, int client_fd);
void handle_conn_req(const std::string& token, const std::vector<uint8_t>& payload);
void handle_reconnect(int new_fd, const std::vector<uint8_t>& payload);
void handle_heartbeat(const std::string& token);
void handle_create_room(const std::string& token, const std::vector<uint8_t>& payload);
void handle_join_room(const std::string& token, const std::vector<uint8_t>& payload);
void handle_leave_room(const std::string& token);
void handle_start_game(const std::string& token);
void handle_player_action(const std::string& token, const std::vector<uint8_t>& payload);
void handle_rebuy(const std::string& token, const std::vector<uint8_t>& payload);
void handle_rebuy_done(const std::string& token);
void handle_end_game(const std::string& token);
void handle_timeout(const std::string& token);
void handle_disconnect_task(const std::string& token);

#endif // HANDLER_H