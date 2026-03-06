#pragma once
/*
自定义协议定义,包含协议相关的处理函数

帧序列化/反序列化函数、各种 payload 的打包/解包函数、Magic 校验、字节序转换
*/

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>
#include <vector>
#include <string>
#include "types.h"

// 协议常量
constexpr uint32_t PROTOCOL_MAGIC = 0x504F4B45; // "POKE"
constexpr uint32_t PROTOCOL_VERSION = 0x00010000; // 1.0.0

// 帧头结构
#pragma pack(push, 1)       // 保存当前对齐设置，然后改为1字节对齐
struct FrameHeader {
    uint32_t magic;          // 魔数，固定为PROTOCOL_MAGIC
    uint32_t version;        // 协议版本
    uint32_t total_length;   // 整个帧的总长度（包括header）
    uint8_t  msg_type;       // 消息类型
    uint32_t seq_id;         // 序列号
    //uint32_t checksum;       // 校验和（可选）
};
#pragma pack(pop)

// 反序列化错误码枚举
enum class DeserializeError {
    SUCCESS = 0,               // 成功
    INSUFFICIENT_DATA,         // 数据长度不足（帧头或完整帧）
    INVALID_MAGIC,             // 魔数不匹配
    VERSION_MISMATCH,          // 版本不匹配
    DATA_CORRUPTED,            // 数据损坏（长度不一致）
    UNKNOWN_ERROR              // 未知错误
};

// 反序列化，输出到 frame
DeserializeError deserialize_frame(const std::vector<uint8_t>& data, Frame& frame);

// 错误码转字符串
const char* deserialize_error_to_string(DeserializeError error);

// 计算校验和
uint32_t calculate_checksum(const std::vector<uint8_t>& data);

// 验证帧头
DeserializeError validate_frame_header(const FrameHeader& header);

// 打包CreateRoomPayload
std::vector<uint8_t> pack_create_room_payload(uint32_t room_id, int32_t small_blind);

// 解包CreateRoomPayload
bool unpack_create_room_payload(const std::vector<uint8_t>& data, 
                                CreateRoomPayload& payload);

// 打包ActionRaisePayload
std::vector<uint8_t> pack_action_raise_payload(int32_t total_bet);

// 解包ActionRaisePayload
bool unpack_action_raise_payload(const std::vector<uint8_t>& data, 
                                 ActionRaisePayload& payload);

// 打包玩家行动payload
std::vector<uint8_t> pack_player_action_payload(uint8_t action_type, int32_t amount);

// 解包玩家行动payload
bool unpack_player_action_payload(const std::vector<uint8_t>& data, 
                                  uint8_t& action_type, int32_t& amount);

// 打包重购请求payload
std::vector<uint8_t> pack_rebuy_payload(int32_t amount);

// 解包重购请求payload
bool unpack_rebuy_payload(const std::vector<uint8_t>& data, int32_t& amount);

// 序列化一个帧（无payload则不传pyload）
std::vector<uint8_t> serialize_frame(uint8_t msg_type, uint32_t seq_id, const std::vector<uint8_t>& payload);
std::vector<uint8_t> serialize_frame(MsgType msg_type, uint32_t seq_id);

// 网络字节序转换辅助函数
uint32_t htonl_safe(uint32_t hostlong);
uint32_t ntohl_safe(uint32_t netlong);
int32_t htonl_safe(int32_t hostlong);
int32_t ntohl_safe(int32_t netlong);

#endif // PROTOCOL_H
