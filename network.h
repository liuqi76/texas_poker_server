#pragma once
/*
网络IO相关函数声明
*/

#ifndef NETWORK_H
#define NETWORK_H

#include <cstdint>
#include <vector>
#include <string>
#include <thread>

// 设置非阻塞
void set_nonblocking(int fd);

// 生成token，处理新玩家连接事件
std::string generate_token();

// ~读取数据并加入任务队列
// ->需要改到protocol.h中
//void read_and_enqueue(int fd);

// 发送序列化完成的帧
void send_frame(int fd, const std::vector<uint8_t>& frame);

// 广播序列化完成的帧
void broadcast(uint32_t roomId, const std::vector<uint8_t>& frame);

// 处理新连接
void handle_new_connection(int server_fd, int epoll_fd);

// 处理断开连接
void handle_disconnect(int fd);

// 服务器初始化，并返回线程
std::vector<std::thread> server_init();

void epoll_loop(int port);

#endif // NETWORK_H