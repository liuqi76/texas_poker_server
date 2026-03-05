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

// 返回一个ack帧
std::vector<uint8_t> make_ack_frame(int client_fd);

// 序列化一个帧
std::vector<uint8_t> serialize_frame(uint8_t msg_type, uint32_t seq_id, 
                                     const std::vector<uint8_t>& payload);

// 反序列化一个帧
bool deserialize_frame(const std::vector<uint8_t>& data, Frame& frame);

// 计算校验和
uint32_t calculate_checksum(const std::vector<uint8_t>& data);

// 验证帧头
bool validate_frame_header(const FrameHeader& header);

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

// 网络字节序转换辅助函数
uint32_t htonl_safe(uint32_t hostlong);
uint32_t ntohl_safe(uint32_t netlong);
int32_t htonl_safe(int32_t hostlong);
int32_t ntohl_safe(int32_t netlong);

#endif // PROTOCOL_H
