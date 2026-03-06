# Texas Poker Server

> 基于 Linux 的高并发德州扑克游戏服务器，**目前开发中**。

## 技术特性

- **网络层**：epoll ET 边缘触发模式，非阻塞 IO，支持高并发长连接
- **并发模型**：独立网络线程 + 线程池，IO 与业务逻辑解耦
- **自定义二进制协议**：魔数校验、版本控制、seq_id 防重放、跨平台帧结构（`#pragma pack(1)` + 网络字节序）
- **鉴权**：连接级 Token 鉴权，`random_device` 真随机生成，支持断线重连
- **牌型判断**：基于预计算查找表（`table_generator`），O(1) 复杂度完成五张牌型评估
- **定时器**：独立定时器线程，处理行动超时、心跳检测、断线清理

## 项目结构

```
├── main.cpp            # 入口，服务器启动与初始化、epoll主循环
├── network.h/cpp       # 网络层：连接管理、收发序列化帧、请求加入任务队列
├── protocol.h/cpp      # 协议层：帧结构定义、序列化/反序列化、帧->任务转换
├── handler.h/cpp       # 业务层：具体游戏业务、
├── room.h/cpp          # 房间定义与管理实现
├── player.h/cpp        # 玩家状态定义与实现
├── dealer.h/cpp        # 发牌与游戏流程控制
├── threadpool.h/cpp    # 线程池实现
├── timer.h/cpp         # 定时器与心跳扫描定义与实现
├── types.h             # 公共类型定义、公共map、锁定义
├── table_generator.cpp # 牌型查找表生成器
└── table_test          # 牌型查找表测试
```

## 协议帧结构

```
C2S（客户端 → 服务端）
+--------+---------+------------+----------+---------+---------+
| magic  | version | total_len  | msg_type | seq_id  | payload |
| 4B     | 1B      | 4B         | 1B       | 4B      | NB      |
+--------+---------+------------+----------+---------+---------+

S2C（服务端 → 客户端）
+--------+---------+------------+----------+---------+--------+---------+
| magic  | version | total_len  | msg_type | seq_id  | status | payload |
| 4B     | 1B      | 4B         | 1B       | 4B      | 1B     | NB      |
+--------+---------+------------+----------+---------+--------+---------+
```

所有多字节字段使用网络字节序。

## 构建

依赖：Linux、g++、C++17

```bash
g++ -std=c++17 -O2 -pthread \
    main.cpp network.cpp protocol.cpp handler.cpp \
    room.cpp player.cpp dealer.cpp threadpool.cpp timer.cpp \
    -o texas_poker_server
```

运行前需先生成牌型查找表（首次运行生成即可，已有该表则不需要）：

```bash
g++ -O2 table_generator.cpp -o table_generator
./table_generator
```

启动服务器：

```bash
./texas_poker_server
```

## 开发状态

- [√] 网络层框架（epoll + 非阻塞 IO）
- [√] 自定义二进制协议设计与序列化
- [-] 线程池
- [-] Token 鉴权与连接管理
- [√] 牌型查找表
- [x] `read_and_enqueue` 完整拆包实现
- [x] 握手与重连流程
- [x] 游戏业务逻辑完善
- [x] 客户端
