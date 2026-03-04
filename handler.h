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

// 各种处理函数
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