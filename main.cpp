/*
主函数，server_init，启动 epoll 主循环。只做启动和初始化，不写业务逻辑。
*/

#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <thread>
#include "types.h"
#include "network.h"

int main() {
    std::cout << "服务器启动中..." << std::endl;
    
    // 服务器初始化
    std::vector<std::thread> threads = server_init();
    for(auto& t : threads)
    {
        t.detach();// TODO: 给这些线程加信号，实现安全退出
    }

    epoll_loop(8080);

    /* 以下部分改到network.cpp里面，main只负责
    // 创建socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "创建socket失败" << std::endl;
        return 1;
    }
    
    // 设置socket选项，避免TIME_WAIT状态
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "设置socket选项失败" << std::endl;
        close(server_fd);
        return 1;
    }
    
    // 绑定地址
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(8888);
    
    if (bind(server_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        std::cerr << "绑定socket失败" << std::endl;
        close(server_fd);
        return 1;
    }
    
    // 监听
    if (listen(server_fd, 10) == -1) {
        std::cerr << "监听失败" << std::endl;
        close(server_fd);
        return 1;
    }
    
    // 设置非阻塞
    set_nonblocking(server_fd);
    
    // 创建epoll实例
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cerr << "创建epoll失败" << std::endl;
        close(server_fd);
        return 1;
    }
    
    // 添加server_fd到epoll
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET; // 边缘触发模式
    ev.data.fd = server_fd;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        std::cerr << "添加server_fd到epoll失败" << std::endl;
        close(epoll_fd);
        close(server_fd);
        return 1;
    }
    
    std::cout << "服务器启动成功，监听端口 8888" << std::endl;
    
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
                // 新连接
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
    
    */
    std::cout << "服务器已关闭" << std::endl;

    return 0;
}