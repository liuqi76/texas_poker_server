/*
定时器相关函数声明
*/

#ifndef TIMER_H
#define TIMER_H

#include <string>
#include <chrono>

// 定时器线程主循环
void timer_loop();

// 设置行动定时器
void set_action_timer(const std::string& token, std::chrono::steady_clock::time_point deadline);

// 取消行动定时器
void cancel_action_timer(const std::string& token);

// 心跳扫描线程
void heartbeat_scan_loop();

// 停止定时器线程
void stop_timer_thread();

#endif // TIMER_H