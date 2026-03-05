/*
网络IO事件相关函数，仅做tcp层的收发和维护
收到申请后调用protocol解包然后发任务队列
收到连接后加入epoll

send_frame、broadcast、handle_new_connection、
handle_disconnect、read_and_enqueue、epoll 主循环
*/

#include "network.h"
#include "types.h"
#include "room.h"
#include "timer.h"
#include "dealer.h"
#include "threadpool.h"
#include "handler.h"
#include "protocol.h"

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
#include <thread>

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

std::string generate_token() {// 已check并更改
    // 生成32字节随机token，返回64字符hex string
    std::random_device rd;
    // 使用 rd 生成两个真随机32字节，然后拼接成为64字节token
    uint64_t part1 = ((uint64_t)rd() << 32) | rd();
    uint64_t part2 = ((uint64_t)rd() << 32) | rd();
    uint64_t part3 = ((uint64_t)rd() << 32) | rd();
    uint64_t part4 = ((uint64_t)rd() << 32) | rd();

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
        //std::cout << "从fd " << fd << " 读取了 " << bytes_read << " 字节数据" << std::endl;
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

void send_frame(int fd, const std::vector<uint8_t>& frame) {// 已完成，组帧逻辑已经改到protocol.cpp
    // ~新增：序列化并发送一个S2C帧给指定fd
    // ->只需要发送帧即可，帧的构建应该放在protocol.cpp
    

    /*
    // ~构建帧头：帧总长度(4字节) + 消息类型(1字节) + 序列号(4字节)
    uint32_t total_len = 1 + 4 + payload.size(); // msgType + seqId + payload
    uint32_t seqId = 0; // TODO: 需要维护序列号
    
    std::vector<uint8_t> frame;
    frame.reserve(4 + 1 + 4 + payload.size());
    
    // ~添加总长度（网络字节序）
    uint32_t net_len = htonl(total_len);
    frame.insert(frame.end(), reinterpret_cast<uint8_t*>(&net_len), 
                 reinterpret_cast<uint8_t*>(&net_len) + 4);
    
    // ~添加消息类型
    frame.push_back(msgType);
    
    // ~添加序列号（网络字节序）
    uint32_t net_seq = htonl(seqId);
    frame.insert(frame.end(), reinterpret_cast<uint8_t*>(&net_seq), 
                 reinterpret_cast<uint8_t*>(&net_seq) + 4);
    
    // ~添加payload
    frame.insert(frame.end(), payload.begin(), payload.end());
    */
    
    // 保留：发送数据
    ssize_t sent = send(fd, frame.data(), frame.size(), 0);
    if (sent < 0) {
        std::cerr << "发送数据到fd " << fd << " 失败: " << strerror(errno) << std::endl;
    }
}

void broadcast(uint32_t roomId, const std::vector<uint8_t>& S2C_frame) {// 已完成
    // 向room内所有玩家广播一个序列化的S2C帧

    std::vector<std::string> tokens;
    
    {
        std::lock_guard<std::mutex> lock(room_map_mutex);
        auto it = room_map.find(roomId);
        if (it == room_map.end()) return;
        tokens = it->second->get_players(); // 通过找到的Room实例拷贝玩家列表
    } // room_map_mutex 退出作用域释放

    std::lock_guard<std::mutex> lock(fd_token_mutex);
    for (const auto& token : tokens) {
        auto fd_it = token_to_fd.find(token);
        if (fd_it != token_to_fd.end()) {
            send_frame(fd_it->second, S2C_frame);
        }
    }
}

void epoll_loop(int port){

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "创建socket失败" << std::endl;
        return;
    }
    
    // 设置socket选项，避免TIME_WAIT状态
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "设置socket选项失败" << std::endl;
        close(server_fd);
        return;
    }
    
    // 绑定地址
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);
    
    if (bind(server_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        std::cerr << "绑定socket失败" << std::endl;
        close(server_fd);
        return;
    }
    
    // 监听
    if (listen(server_fd, 10) == -1) {
        std::cerr << "监听失败" << std::endl;
        close(server_fd);
        return;
    }
    
    // 设置非阻塞
    set_nonblocking(server_fd);
    
    // 创建epoll实例
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cerr << "创建epoll失败" << std::endl;
        close(server_fd);
        return;
    }
    
    // 添加server_fd到epoll
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET; // 边缘触发模式，关注可读事件
    ev.data.fd = server_fd;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        std::cerr << "添加server_fd到epoll失败" << std::endl;
        close(epoll_fd);
        close(server_fd);
        return;
    }
    
    std::cout << "服务器启动成功，监听端口： " << port << std::endl;
    
    // epoll事件循环
    struct epoll_event events[256];
    while (true) {
        int n = epoll_wait(epoll_fd, events, 256, -1);
        if (n == -1) {
            std::cerr << "epoll_wait失败" << std::endl;
            break;
        }
        
        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == server_fd) {
                // 新连接或重新连接
                handle_new_connection(server_fd, epoll_fd);
            } else {
                // 现有连接有数据可读
                read_and_enqueue(events[i].data.fd);
            }
        }
    }
    
    // 清理资源
    close(epoll_fd);
    close(server_fd);
    
    std::cout << "epoll循环跳出" << std::endl;
}

void handle_new_connection(int server_fd, int epoll_fd) {
    // ~新增：accept新连接，生成token，建立fd→token映射，发送ack_frame
    // ->只accept并加入epoll，然后执行握手逻辑
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    
    if (client_fd != -1) {
        // 设置非阻塞
        set_nonblocking(client_fd);
        
        /* ~生成token
        // ->等逻辑改到handler的握手里面
        //std::string token = generate_token();
        
        // ~建立映射关系
        std::lock_guard<std::mutex> lock(fd_token_mutex);
        fd_to_token[client_fd] = token;
        token_to_fd[token] = client_fd;

        // ~发送连接确认
        std::vector<uint8_t> ack_frame;
        // TODO: ack_frame = make_ack_frame();
        // TODO: 添加必要的连接信息到payload
        send_frame(client_fd, ack_frame);
        
        // ~更新最后活跃时间
        last_active_time[token] = std::chrono::steady_clock::now();
        */

        // 将新连接加入epoll监控
        struct epoll_event client_ev;
        client_ev.events = EPOLLIN | EPOLLET; // 边缘触发模式
        client_ev.data.fd = client_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_ev);
        
        printf("新玩家连接: %s:%d, 等待握手。\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        send_frame(client_fd,make_ack_frame(client_fd));
        handle_handshake(client_fd);
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
        
        enqueue_task(disconnect_task);//未声明，不知道啥情况
        
        std::cout << "玩家断开连接，fd: " << fd << ", token: " << token << std::endl;
    }
}

std::vector<std::thread> server_init() {
    // 服务器启动时的全局初始化（init_table、线程池、定时器线程等）
    // 返回是为了在main那边join

    // 读取牌型表文件
    init_table();
    
    // 初始化线程池
    threadpool_init(4); // 假设4个工作线程
    
    std::vector<std::thread> threads;
    threads.emplace_back(timer_loop);
    threads.emplace_back(heartbeat_scan_loop);
    
    std::cout << "服务器初始化完成" << std::endl;

    return threads;
}
