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
#include <thread>

class typeTable {
    private:
    static std::map<long long, short> typeMap;

    friend class Dealer;
    friend void init_table();
};

struct Msg {
    int attachedData1=-1;
    int attachedData2=-1;
};

struct DealerMsg : public Msg{
    int dealerId;
    enum outMsgType { DISTRIBUTE=0, ACTION_SUCCESS, ACTION_RES } type;
};//发牌信息、行动成功后的信息

struct otherMsg : public Msg{
    enum otherMsgType {UPDATE=0,STATUE_CHANGE} type;
    enum updateType { NEW_PLAYER=0, PLAYER_LEFT, NEW_BET, NEW_COMMUNITY_CARD,  } updateType;
};//服务器发给玩家的消息。
//通知其他玩家的行动、欢迎页面、提示等。

struct PlayerMsg : public Msg{
    short Msglen;//消息体长度

    int playerId;
    enum inMsgType { CREATE_ROOM=0, JOIN_ROOM, START_GAME, PLAYER_ACTION, END_GAME } type;
    std::string content;//玩家发来的消息体，可能是json、protobuf或者二进制串，具体格式由type决定
};//玩家发来的消息。当创建房间时，附加数据分别是房间id和人数上限，所有附加数据必填，-1为无效
//当加入房间时，附加数据1是房间id
//当开始游戏时，需要判断是否是房主
//当玩家行动时，附加数据1是行动类型（跟注=0、加注、全押、弃牌），附加数据2是操作对应的金额

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
    static constexpr int oddList[52] = {
        1, 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 
        59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 
        137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 
        227, 229, 233
    };//质数表，用于生成牌型id

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
    short rank5in7(int& hand[2],  std::vector<short> community_cards) {
        // 计算玩家最大可能牌型，并转换为牌型排名
        std::vector<int> all_cards={hand[0], hand[1], community_cards[0], community_cards[1], community_cards[2], community_cards[3], community_cards[4]};
        short max_rank = 1;
        for(int i=0;i<3;i++){
            for(int j=i+1;j<4;j++){
                for(int k=j+1;k<5;k++){
                    for(int l=k+1;l<6;l++){
                        for(int m=l+1;m<7;m++){
                            long long composeId = oddList[all_cards[i]] * oddList[all_cards[j]] * oddList[all_cards[k]] * oddList[all_cards[l]] * oddList[all_cards[m]];
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

    }

    void distribute() {
        
    }

    void rotate_blinds() {
        // 轮换小盲和大盲
        sbId = (sbId + 1) % playerCount;
    }
    public:
    short player_call(Player* player) {
        // 处理玩家的跟注，计算实际下注金额
       
        // 返回玩家当前的下注金额
        return 0;
    }

    void player_fold(Player* player) {
        // 玩家弃牌，更新状态
        activePlayer[player->playerid] = false;
    }

    short player_allin(int playerid, int amount) {
        // 玩家全押，更新状态和底池、边池
        allinPlayer[playerid] = true;
        // 更新底池金额
        allinSequence.push_back(playerid);
        return 0;
    }

};

class Player {
    public:
    int playerid;
    std::string name;
    private:
    short chips;
    short hand[2]; // 手牌
    short rebuycount;
    Dealer* dealer;// 这个玩家所在的牌局的dealer
    short currentBet;//当前轮已经下注的金额
    enum status { WAITING, ACTIVE, FOLDED, ALL_IN } playerStatus;//这里的waiting表示在局内但是还未轮到，active表示正在行动

    int rebuy(int amount) {
        if (amount <= 0||amount > 10) return-1; // 无效的买入金额
        chips += amount*100;
        rebuycount += amount;
        return 0; // 成功买入
    }

    void fold() {
        // 玩家弃牌
        dealer->player_fold(this);
        playerStatus = FOLDED;
    }

    void raise(short amount) {//amount是加注后的总下注金额
        //加注，需要判断加注金额是否合法
        if (amount <= currentBet || amount > chips) return; // 无效的加注金额
        chips = chips - amount + currentBet; // 扣除加注金额
        currentBet = amount;
        playerStatus = WAITING;
    }
    //玩家告知荷官要干什么->荷官告诉玩家需要扣除多少筹码->玩家扣除筹码并更新状态
    void call() {
        //玩家跟注，调用荷官的player_call函数，计算需要扣掉的筹码，荷官那边计算实际行动金额，可能是跟注金额，也可能是全押金额
        chips -= dealer->player_call(this);
    }

    void allin() {
        dealer->player_allin(playerid, chips);
        chips = 0;
        playerStatus = ALL_IN;
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

void game_loop() {
    // 这里是游戏的主循环，处理玩家的行动，更新游戏状态，发送更新给玩家等
    while(true){
        // 处理游戏逻辑，例如发牌、玩家行动、结算等
    }

    // 当房主选择结束游戏时，跳出循环，进行清理工作，例如关闭玩家连接、重置游戏状态等
}

void game_prepare_loop() {
    // 这里是房间的准备阶段，当某玩家开房间后，进入此循环，当其选择开始游戏后，将这些玩家放入游戏主循环game_loop中
    // 每当一个玩家选择创建房间，会新建一个该函数的线程，如果
    // 同一个房间的gameloop和game_prepare_loop由同一个线程执行，使用map<roomid,thread>来管理房间和线程的关系
    while(true){

    }// 房主选择开始游戏后，跳出循环，进入game_loop
    game_loop();

}

void addressMsg(PlayerMsg player_msg, struct epoll_event* events, int i) {
    // 这个函数用于处理玩家发来的消息，根据消息类型调用相应的游戏逻辑函数
    switch(player_msg.type) {
        case PlayerMsg::CREATE_ROOM:
            // 调用创建房间的函数
            break;
        case PlayerMsg::JOIN_ROOM:
            // 调用加入房间的函数
            break;
        case PlayerMsg::START_GAME:
            // 调用开始游戏的函数
            break;
        case PlayerMsg::PLAYER_ACTION:
            // 调用处理玩家行动的函数
            break;
        case PlayerMsg::END_GAME:
            // 调用结束游戏的函数
            break;
        default:
            // 处理未知消息类型
            break;
    }
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

    int epoll_fd = epoll_create1(0);//这个epoll仅监听新玩家和未进房间的玩家，进入房间的玩家由每个房间的epoll监听
    //epoll_create1函数创建一个epoll实例，并返回实例的fd，参数0表示默认行为
    //以前用epoll_create函数创建epoll实例，其参数表示最大监控的socket数量，但现在这个参数作废了


    struct epoll_event ev, events[20]; 
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
        int n = epoll_wait(epoll_fd, events, 20, -1);
        //epoll_wait函数会阻塞当前线程，直到有事件发生或者超时
        //epoll_fd是创建的epoll实例，events是事件接收缓冲区
        //10是缓冲区大小，-1表示无限等待，如果设为正整数表示这个函数的等待时间
        //返回值说明就绪事件数量


        for (int i = 0; i < n; i++) {
            // 2. 这里的 events[i].data.fd 就是之前存入 ev.data.fd 的那个 Key
            // 这个ev.data.fd用于辨认哪个过滤器触发了事件，也就是哪个连接（fd）发生了事件
            if (events[i].data.fd == server_fd) {
                //新玩家连接事件
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                //accept会为就绪的第一个连接创建一个描述符，并把这个连接剔除出监听队列，并返回这个连接的描述符
                if (client_fd != -1) {
                    // 接下来需要将新玩家的 client_fd 也加入 epoll 监控池
                    struct epoll_event client_ev;
                    client_ev.events = EPOLLIN;
                    client_ev.data.fd = client_fd;//过滤器：监听此新玩家的输入
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_ev);
                    
                    // 调用欢迎
                    send_hello(client_fd);
                    // 一个fd是一个连接的索引
                    printf("New player connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                }

            } else {
                //非新玩家连接事件
                char buffer[1024] = {0};
                int bytes_read = recv(events[i].data.fd, buffer, sizeof(buffer), 0);
                //返回值是实际读取的字节数，如果返回0表示连接被对方关闭，返回-1表示发生错误
                //events[i].data.fd 是玩家的文件描述符
                //buffer 是接收数据的缓冲区，数据在这里读取
                //sizeof(buffer) 是读取的最大字节数
                //最后一个参数是标志位，0 表示默认行为，MSG_PEEK 表示查看数据但不从缓冲区移除
                //MSG_WAITALL 表示等待缓冲区大于等于前面读取的字节数才返回
                if(bytes_read > 0) {
                    // 正常读取数据
                    //数据包结构：[包总长]4字节 + [消息类型]2字节 + [消息体（JSON 或 Protobuf 或 二进制串）]
                    
                    //
                    //
                    //拿到buffer之后，解决粘包和分包问题，解析出一个完整的消息体，构造PlayerMsg对象，扔给一个thread
                    //直到处理完所有本fd的消息（把所有本fd的请求都给thread，不需要阻塞，要detach）
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    //
                    auto t1 = new std::thread(addressMsg, player_msg, events, i);


                } else if (bytes_read == 0) {
                    // 玩家断开了连接
                    printf("Player %d disconnected.\n", events[i].data.fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                    close(events[i].data.fd);
                } else if (bytes_read == -1) {
                    // 连接出错
                    struct sockaddr_in addr;
                    socklen_t addr_len = sizeof(addr);
                    if (getpeername(events[i].data.fd, (struct sockaddr *)&addr, &addr_len) == -1) {
                        perror("getpeername 失败");
                        return;
                    }
                    //给出连接出错的玩家的具体报错信息
                    std::cerr << "Error reading from player " << events[i].data.fd <<"from"<< inet_ntoa(addr.sin_addr) << std::endl;
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                    close(events[i].data.fd);
                }
            }
        }
    }


    return 0;
}