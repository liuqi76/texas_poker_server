#include <iostream>
#include <string.h>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <arpa/inet.h> 
#include <sys/epoll.h>
#include <fcntl.h>
#include <map>
#include <fstream>

class typeTable {
    private:
    static std::map<int, short> typeMap;

    friend class Dealer;
    friend void init_table();
};

struct Pot {
    int amount; // 底池金额
    std::vector<int> exempt_players; // 没有资格参与这个底池分配的玩家列表（all in的玩家）
};

enum suit { HEARTS = 0, DIAMONDS, CLUBS, SPADES }; // 花色，'H', 'D', 'C', 'S'

struct card
{
    enum suit color;
    int rank; // 牌面值，0-12分别代表2、3...10、J、Q、K、A
};


bool suited(std::vector<int> color) {
    if (color[0] > 4 || color[1] > 4 || color[2] > 4 || color[3] > 4) {
        return true; // 同花
    }
    return false; // 不同花
}

void init_table(){
    // 从table.txt中读取牌型表，初始化typeTable::typeMap
    // 牌型表格式：牌型id（素数积） 牌型排名
    std::fstream infile("./table.txt");
    int prime_product, rank;
    while (infile >> prime_product >> rank) {
        typeTable::typeMap[prime_product] = rank;
    }
}

class Dealer {
    int dealerId;
    int playerCount;//用于轮换sb和bb
    int activePlayerCount;//表示当前还未folded、all in的玩家数量，用于计算底池和边池
    std::vector<bool> activePlayer;//表示玩家是否还未folded
    std::vector<bool> allinPlayer;//表示玩家是否已经all in
    std::vector<Pot> pots;//池串，第一个是底池，默认所有人瓜分，后面是边池，需要参考exempt_players来分配
    std::vector<int> allinSequence;//all in顺序串，记录每个玩家all in的顺序，用于计算边池分配
    int currentBet;//当前轮的最高下注金额
    int sbId;//小盲玩家id
    std::vector<short> cards[5];//牌串，记录公共牌
    std::vector<short> color[4];//花色数量串，记录公共牌的四种花色数量，用于判断同花
    short currentPotIndex;//当前的底池索引

    void count(int AllinCount, int * AllinAmountList, int * AllinPlayerList) {//本轮allin人数、allin的金额、allin玩家的id列表
        // 当一轮结束时，计算底池金额和边池金额
        for (int i = 0; i < AllinCount; i++) {
            int allin_amount = AllinAmountList[i];
            int allin_player = AllinPlayerList[i];
            // 根据allin金额和玩家状态计算边池金额
            // 更新pot和sidepot
        }
    }
//0 1 2 3 4 5 6
    short rank5in7(std::vector<int> hand,  std::vector<int> community_cards, std::vector<short> odd_list[13]) {
        // 计算玩家最大可能牌型，并转换为牌型排名
        std::vector<int> all_cards={hand[0], hand[1], community_cards[0], community_cards[1], community_cards[2], community_cards[3], community_cards[4]};
        short max_rank = 1;
        for(int i=0;i<3;i++){
            for(int j=i+1;j<4;j++){
                for(int k=j+1;k<5;k++){
                    for(int l=k+1;l<6;l++){
                        for(int m=l+1;m<7;m++){
                            int composeId = odd_list[all_cards[i]] * odd_list[all_cards[j]] * odd_list[all_cards[k]] * odd_list[all_cards[l]] * odd_list[all_cards[m]];
                            short rank = typeTable::typeMap[composeId]; // 这里需要根据牌的具体编码来计算牌型排名

                            if (rank > max_rank) {
                                max_rank = rank;
                            }
                        }
                    }
                }
            }
        }
        return max_rank;
    }
    
    void player_rank(std::vector<Player> players) {
        // 结算函数，根据玩家的牌型生成排名串
        std::vector<short> odd_list={1,2,3,5,7,11,13,17,19,23,29,31,37};//质数串，用于生成牌型id
        for (int i = 0; i < playerCount; i++) {
            suited(color);
            int currentrank = rank5in7(players[i].hand, cards, odd_list);
        }

    }

    void distribute() {
        
    }

    void rotate_blinds() {
        // 轮换小盲和大盲
        sbId = (sbId + 1) % playerCount;
    }

    short player_call() {
        // 处理玩家的跟注、加注、全押等操作
    }

    void player_fold(int playerid) {
        // 玩家弃牌，更新状态
        activePlayer[playerid] = false;
    }

    short player_allin(int playerid, int amount) {
        // 玩家全押，更新状态和底池
        allinPlayer[playerid] = true;
        // 更新底池金额
        allinSequence.push_back(playerid);
        return 0;
    }

};

class Player {
    int playerid;
    std::string name;
    int chips;
    int hand[2]; // 手牌
    int rebuycount;
    int dealerid;// 这个玩家所在的牌局的dealer
    enum status { WAITING, ACTIVE, FOLDED, ALL_IN };//这里的waiting表示在局内但是还未轮到，active表示正在行动

    int rebuy(int amount) {
        if (amount <= 0||amount > 10) return-1; // 无效的买入金额
        chips += amount*100;
        rebuycount += amount;
        return 0; // 成功买入
    }

    void fold() {
        // 玩家弃牌
        this->status = FOLDED;
    }

    void raise(int amount) {
        //加注，需要判断加注金额是否合法
        if (amount <= 0 || amount > chips) return; // 无效的加注金额
        chips -= amount;
        this->status = WAITING; // 加注后进入等待状态，等待其他玩家行动
    }

    void call() {
        if (currentbet > chips) {
            // 玩家无法跟注，选择全押
            allin();//改用allin函数
        } else {
            chips -= currentbet;
            status = WAITING; // 跟注后进入等待状态，等待其他玩家行动
        }
    }

    void allin() {
        chips = 0;
        status = ALL_IN;
    }

};

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void send_hello(int client_fd) {//欢迎新玩家，并确认已连接，同时给出现在的牌局状态
    const char* message = "Welcome to the poker game!\n";
    send(client_fd, message, strlen(message), 0);
}

int main() {
    std::cout << "Server started" << std::endl;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);//为进程分配了一个文件描述符（空）
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr)); // 先清零

    serv_addr.sin_family = AF_INET;                // 使用 IPv4
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听所有 IP
    serv_addr.sin_port = htons(8888);            // 在这里填入正确的转换函数

        // bind 会返回一个整数，如果失败通常返回 -1
    if (bind(server_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        // 处理错误
        std::cerr << "Failed to bind socket" << std::endl;
        close(server_fd);
        return 1;
    }
    // 参数10代表排队等待连接的最大玩家数为10
    listen(server_fd, 10);
    set_nonblocking(server_fd);

    int epoll_fd = epoll_create1(0);
    //epoll_create1函数创建一个epoll实例，并返回实例的fd，参数0表示默认行为
    //以前用epoll_create函数创建epoll实例，其参数表示最大监控的socket数量，但现在这个参数作废了


    struct epoll_event ev, events[10]; 
    //epoll_event结构体用于描述一个事件，包含两个成员：events和data
    //events表示事件类型：EPOLLIN（fd可读），EPOLLOUT（fd可写），EPOLLET（边缘触发模式）

    ev.events = EPOLLIN; //
    ev.data.fd = server_fd;

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);
    //epoll_ctl函数用于增删改一个epoll实例的监控状态，也就是epoll_fd
    //这里的ev是一个过滤器，告诉epoll我们关心哪些事件
    //epoll_fd表示对这个实例进行操作
    //EPOLL_CTL_ADD表示行为是添加一个新的fd到监控列表中，也就是server_fd
    //&ev是我们设置的事件过滤器



    while (true) {
        //loop部分
        int n = epoll_wait(epoll_fd, events, 10, -1);
        //epoll_wait函数会阻塞当前线程，直到有事件发生或者超时
        //epoll_fd是创建的epoll实例，events是事件接收缓冲区
        //10是缓冲区大小，-1表示无限等待，如果设为正整数表示这个函数的等待时间
        //返回值说明就绪事件数量


        for (int i = 0; i < n; i++) {
            // 2. 这里的 events[i].data.fd 就是之前存入 ev.data.fd 的那个 Key
            if (events[i].data.fd == server_fd) {
                //此时是服务器监听事件，也就是新玩家申请连接
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                
                if (client_fd != -1) {
                    // 接下来需要将新玩家的 client_fd 也加入 epoll 监控池
                    struct epoll_event client_ev;
                    client_ev.events = EPOLLIN; // 监听玩家发来的指令
                    client_ev.data.fd = client_fd; // 标记这是哪位玩家
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_ev);
                    
                    // 调用欢迎
                    send_hello(client_fd);
                    printf("New player connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                }

            } else {
                //此时是server_fd以外的事件，也就是玩家发来的指令
                char buffer[1024] = {0};
                int bytes_read = recv(events[i].data.fd, buffer, sizeof(buffer), 0);
                //events[i].data.fd 是玩家的文件描述符
                //buffer 是接收数据的缓冲区，数据在这里读取
                //sizeof(buffer) 是读取的最大字节数
                //最后一个参数是标志位，0 表示默认行为，MSG_PEEK 表示查看数据但不从缓冲区移除
                //MSG_WAITALL 表示等待缓冲区大于等于前面读取的字节数才返回

                if (bytes_read == 0) {
                    // 玩家断开了连接
                    printf("Player %d disconnected.\n", events[i].data.fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                    close(events[i].data.fd);
                } else if (bytes_read > 0) {
                    // 收到玩家的指令（例如：“跟注”），可以在这里处理德州扑克逻辑
                    printf("Received from player %d: %s\n", events[i].data.fd, buffer);
                    
                
                
                
                }
            }
        }
    }


    return 0;
}