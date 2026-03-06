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

/*打算改成所有无payload帧的正反序列化统一写
std::vector<uint8_t> make_ack_frame(int client_fd){//已完成
// 空payload，只是通知客户端可以发握手包了
std::vector<uint8_t> payload;

uint32_t total_len = sizeof(FrameHeader) + payload.size();

FrameHeader header;
header.magic       = htonl(PROTOCOL_MAGIC);
header.version     = PROTOCOL_VERSION;
header.total_length   = htonl(total_len);
header.msg_type    = ACCEPT;//表示连接建立
header.seq_id      = htonl(0);

std::vector<uint8_t> frame(sizeof(FrameHeader));
memcpy(frame.data(), &header, sizeof(FrameHeader));

return frame;
}
*/

//序列化一个无payload帧
std::vector<uint8_t> make_empty_frame(MsgType msgType, uint32_t seqId = 0) {
    uint32_t total_len = sizeof(FrameHeader);
    
    FrameHeader header;
    header.magic = htonl(PROTOCOL_MAGIC);
    header.version = PROTOCOL_VERSION;
    header.total_length = htonl(total_len);
    header.msg_type = msgType;
    header.seq_id = htonl(seqId);
    
    std::vector<uint8_t> frame(sizeof(FrameHeader));
    memcpy(frame.data(), &header, sizeof(FrameHeader));
    
    return frame;
}

//反序列化一个无payload帧（已确认）

// 网络字节序转换辅助函数
uint32_t ntohl_safe(uint32_t netlong) {
    return ntohl(netlong);
}

int32_t ntohl_safe(int32_t netlong) {
    return ntohl(netlong);
}

// 验证帧头
bool validate_frame_header(const FrameHeader& header) {
    uint32_t magic = ntohl_safe(header.magic);
    uint32_t version = ntohl_safe(header.version);
    
    if (magic != PROTOCOL_MAGIC) {
        return false; // 魔数不匹配
    }
    
    if (version != PROTOCOL_VERSION) {
        return false; // 版本不匹配
    }
    
    return true;
}

// 统一反序列化函数 - 将原始字节流反序列化为Frame结构体
bool deserialize_frame(const std::vector<uint8_t>& data, Frame& frame) {
    // 检查数据长度是否足够包含帧头
    if (data.size() < sizeof(FrameHeader)) {
        return false;
    }
    
    // 提取帧头
    FrameHeader header;
    memcpy(&header, data.data(), sizeof(FrameHeader));
    
    // 验证帧头
    if (!validate_frame_header(header)) {
        return false;
    }
    
    // 获取帧的总长度（网络字节序转主机字节序）
    uint32_t total_length = ntohl_safe(header.total_length);
    
    // 验证数据长度是否与帧头中声明的总长度一致
    if (data.size() < total_length) {
        return false; // 数据不完整
    }
    
    // 提取消息类型和序列号
    frame.msgType = static_cast<MsgType>(header.msg_type);
    frame.seqId = ntohl_safe(header.seq_id);
    
    // 计算payload长度
    uint32_t payload_length = total_length - sizeof(FrameHeader);
    
    // 提取payload（如果有的话）
    if (payload_length > 0) {
        frame.payload.resize(payload_length);
        memcpy(frame.payload.data(), data.data() + sizeof(FrameHeader), payload_length);
    } else {
        frame.payload.clear(); // 无payload
    }
    
    return true;
}

// 序列化一个帧（支持有payload和无payload）
std::vector<uint8_t> serialize_frame(uint8_t msg_type, uint32_t seq_id, 
                                     const std::vector<uint8_t>& payload) {
    uint32_t total_len = sizeof(FrameHeader) + payload.size();
    
    FrameHeader header;
    header.magic = htonl(PROTOCOL_MAGIC);
    header.version = PROTOCOL_VERSION;
    header.total_length = htonl(total_len);
    header.msg_type = msg_type;
    header.seq_id = htonl(seq_id);
    
    std::vector<uint8_t> frame(total_len);
    memcpy(frame.data(), &header, sizeof(FrameHeader));
    
    // 如果有payload，复制payload数据
    if (!payload.empty()) {
        memcpy(frame.data() + sizeof(FrameHeader), payload.data(), payload.size());
    }
    
    return frame;
}

// 计算校验和（可选功能）
uint32_t calculate_checksum(const std::vector<uint8_t>& data) {
    uint32_t checksum = 0;
    for (size_t i = 0; i < data.size(); i++) {
        checksum += data[i];
    }
    return checksum;
}
