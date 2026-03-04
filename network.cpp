/*
网络IO事件相关函数

send_frame、broadcast、handle_new_connection、
handle_disconnect、read_and_enqueue、epoll 主循环
*/

#include "network.h"
#include "types.h"
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <vector>
#include <random>
#include <sstream>
#include <iomanip>

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

std::string generate_token() {
    // 生成32字节随机token，返回64字符hex string
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    
    uint64_t part1 = dis(gen);
    uint64_t part2 = dis(gen);
    uint64_t part3 = dis(gen);
    uint64_t part4 = dis(gen);
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0') 
       << std::setw(16) << part1
       << std::setw(16) << part2
       << std::setw(16) << part3
       << std::setw(16) << part4;
    
    return ss.str();
}

void read_and_enqueue(int fd) {
    // 新增：循环read直到EAGAIN，按Header拼接完整帧，完整帧投入任务队列
    // TODO: 实现读取和入队逻辑
    char buffer[1024];
    ssize_t bytes_read;
    
    while ((bytes_read = recv(fd, buffer, sizeof(buffer), 0)) > 0) {
        // 处理接收到的数据
        // 需要解决粘包和分包问题
        std::cout << "从fd " << fd << " 读取了 " << bytes_read << " 字节数据" << std::endl;
    }
    
    if (bytes_read == 0) {
        // 连接关闭
        handle_disconnect(fd);
    } else if (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        // 读取错误
        std::cerr << "读取fd " << fd << " 时发生错误: " << strerror(errno) << std::endl;
        handle_disconnect(fd);
    }
}

void send_frame(int fd, uint8_t msgType, const std::vector<uint8_t>& payload) {
    // 新增：序列化并发送一个S2C帧给指定fd
    // TODO: 实现帧发送逻辑
    
    // 构建帧头：总长度(4字节) + 消息类型(1字节) + 序列号(4字节)
    uint32_t total_len = 1 + 4 + payload.size(); // msgType + seqId + payload
    uint32_t seqId = 0; // TODO: 需要维护序列号
    
    std::vector<uint8_t> frame;
    frame.reserve(4 + 1 + 4 + payload.size());
    
    // 添加总长度（网络字节序）
    uint32_t net_len = htonl(total_len);
    frame.insert(frame.end(), reinterpret_cast<uint8_t*>(&net_len), 
                 reinterpret_cast<uint8_t*>(&net_len) + 4);
    
    // 添加消息类型
    frame.push_back(msgType);
    
    // 添加序列号（网络字节序）
    uint32_t net_seq = htonl(seqId);
    frame.insert(frame.end(), reinterpret_cast<uint8_t*>(&net_seq), 
                 reinterpret_cast<uint8_t*>(&net_seq) + 4);
    
    // 添加payload
    frame.insert(frame.end(), payload.begin(), payload.end());
    
    // 发送数据
    ssize_t sent = send(fd, frame.data(), frame.size(), 0);
    if (sent < 0) {
        std::cerr << "发送数据到fd " << fd << " 失败: " << strerror(errno) << std::endl;
    }
}

void broadcast(uint32_t roomId, uint8_t msgType, const std::vector<uint8_t>& payload) {
    // 新增：向room内所有玩家广播一个S2C帧
    std::lock_guard<std::mutex> lock(room_map_mutex);
    
    auto it = room_map.find(roomId);
    if (it != room_map.end()) {
        Room* room = it->second;
        const auto& players = room->get_players();
        
        std::lock_guard<std::mutex> lock2(fd_token_mutex);
        
        for (const auto& token : players) {
            auto fd_it = token_to_fd.find(token);
            if (fd_it != token_to_fd.end()) {
                send_frame(fd_it->second, msgType, payload);
            }
        }
    }
}

void handle_new_connection(int server_fd, int epoll_fd) {
    // 新增：accept新连接，生成token，建立fd→token映射，发送CONN_ACK
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    
    if (client_fd != -1) {
        // 设置非阻塞
        set_nonblocking(client_fd);
        
        // 生成token
        std::string token = generate_token();
        
        // 建立映射关系
        std::lock_guard<std::mutex> lock(fd_token_mutex);
        fd_to_token[client_fd] = token;
        token_to_fd[token] = client_fd;
        
        // 将新连接加入epoll监控
        struct epoll_event client_ev;
        client_ev.events = EPOLLIN | EPOLLET; // 边缘触发模式
        client_ev.data.fd = client_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_ev);
        
        // 发送连接确认
        std::vector<uint8_t> ack_payload;
        // TODO: 添加必要的连接信息到payload
        send_frame(client_fd, CONN_ACK, ack_payload);
        
        // 更新最后活跃时间
        last_active_time[token] = std::chrono::steady_clock::now();
        
        printf("新玩家连接: %s:%d, token: %s\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),
               token.c_str());
    }
}

void handle_disconnect(int fd) {
    // 新增：处理fd断开（主动或被动），更新映射表，触发断线流程
    std::lock_guard<std::mutex> lock(fd_token_mutex);
    
    auto it = fd_to_token.find(fd);
    if (it != fd_to_token.end()) {
        std::string token = it->second;
        
        // 从映射表中移除
        fd_to_token.erase(it);
        token_to_fd.erase(token);
        last_active_time.erase(token);
        
        // 关闭文件描述符
        close(fd);
        
        // 触发断线任务
        Task disconnect_task;
        disconnect_task.msgType = DISCONNECT;
        disconnect_task.uid = 0; // TODO: 需要获取uid
        disconnect_task.dealerId = 0;
        disconnect_task.payload = std::vector<uint8_t>();
        
        enqueue_task(disconnect_task);
        
        std::cout << "玩家断开连接，fd: " << fd << ", token: " << token << std::endl;
    }
}

void server_init() {
    // 新增：服务器启动时的全局初始化（init_table、线程池、定时器线程等）
    
    // 初始化牌型表
    init_table();
    
    // 初始化线程池
    threadpool_init(4); // 假设4个工作线程
    
    // 启动定时器线程
    std::thread timer_thread(timer_loop);
    timer_thread.detach();
    
    // 启动心跳扫描线程
    std::thread heartbeat_thread(heartbeat_scan_loop);
    heartbeat_thread.detach();
    
    std::cout << "服务器初始化完成" << std::endl;
}
