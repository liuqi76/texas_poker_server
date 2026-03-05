/*
将从network.cpp接收到的原始帧反序列化，
并填充为types.Frame的工作，
以及将S2C指令序列化成可以直接发送的帧的工作。
返回的类型应该是types.Frame或者std::vector<uint8_t>

需要的内容：
自定义协议定义,包含协议相关的处理函数

帧序列化/反序列化函数、各种 payload 的打包/解包函数、Magic 校验、字节序转换
*/

#include "protocol.h"
#include "types.h"

#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

std::vector<uint8_t> make_ack_frame(int client_fd){//已完成
// 空payload，只是通知客户端可以发握手包了
    std::vector<uint8_t> payload;

    uint32_t total_len = sizeof(FrameHeader) + payload.size();

    FrameHeader header;
    header.magic       = htonl(PROTOCOL_MAGIC);
    header.version     = 0x01;
    header.total_length   = htonl(total_len);
    header.msg_type    = ACCEPT;//表示连接建立
    header.seq_id      = htonl(0);

    std::vector<uint8_t> frame(sizeof(FrameHeader));
    memcpy(frame.data(), &header, sizeof(FrameHeader));

    return frame;
}