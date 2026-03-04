/*
网络IO相关函数声明
*/

#ifndef NETWORK_H
#define NETWORK_H

#include <cstdint>
#include <vector>
#include <string>

// 设置非阻塞
void set_nonblocking(int fd);

// 生成token
std::string generate_token();

// 读取数据并加入任务队列
void read_and_enqueue(int fd);

// 发送帧
void send_frame(int fd, uint8_t msgType, const std::vector<uint8_t>& payload);

// 广播
void broadcast(uint32_t roomId, uint8_t msgType, const std::vector<uint8_t>& payload);

// 处理新连接
void handle_new_connection(int server_fd, int epoll_fd);

// 处理断开连接
void handle_disconnect(int fd);

// 服务器初始化
void server_init();

#endif // NETWORK_H