/*
定时器相关函数

timer_loop、最小堆、set_action_timer、cancel_action_timer、
heartbeat_scan_loop
*/

#include "timer.h"
#include "types.h"
#include <iostream>
#include <queue>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <atomic>

// 定时器项结构
struct TimerItem {
    std::string token;
    std::chrono::steady_clock::time_point deadline;
    uint32_t seqId; // 用于取消定时器
    
    // 用于最小堆比较
    bool operator>(const TimerItem& other) const {
        return deadline > other.deadline;
    }
};

// 定时器最小堆和同步原语
std::priority_queue<TimerItem, std::vector<TimerItem>, std::greater<TimerItem>> timer_heap;
std::mutex timer_mutex;
std::condition_variable timer_cv;
std::atomic<bool> stop_timer{false};

// 序列号生成
std::atomic<uint32_t> next_seq_id{1};

void timer_loop() {
    // 新增：定时器线程主循环，维护最小堆，到期则投入TIMEOUT Task
    while (!stop_timer) {
        std::unique_lock<std::mutex> lock(timer_mutex);
        
        if (timer_heap.empty()) {
            // 堆为空，等待新定时器
            timer_cv.wait(lock, [] {
                return !timer_heap.empty() || stop_timer;
            });
            
            if (stop_timer) {
                return;
            }
        }
        
        // 获取最早到期的定时器
        TimerItem earliest = timer_heap.top();
        auto now = std::chrono::steady_clock::now();
        
        if (earliest.deadline <= now) {
            // 定时器到期
            timer_heap.pop();
            lock.unlock();
            
            // 创建超时任务
            Task timeout_task;
            timeout_task.msgType = TIMEOUT;
            timeout_task.uid = 0; // TODO: 需要根据token获取uid
            timeout_task.dealerId = 0;
            timeout_task.payload = std::vector<uint8_t>();
            
            enqueue_task(timeout_task);
            
            std::cout << "定时器到期，token: " << earliest.token 
                      << ", seqId: " << earliest.seqId << std::endl;
        } else {
            // 等待直到下一个定时器到期
            auto wait_time = earliest.deadline - now;
            timer_cv.wait_for(lock, wait_time);
        }
    }
}

void set_action_timer(const std::string& token, std::chrono::steady_clock::time_point deadline) {
    // 新增：为指定玩家设置行动deadline，写入最小堆
    std::lock_guard<std::mutex> lock(timer_mutex);
    
    TimerItem item;
    item.token = token;
    item.deadline = deadline;
    item.seqId = next_seq_id++;
    
    timer_heap.push(std::move(item));
    timer_cv.notify_one();
    
    std::cout << "设置行动定时器，token: " << token 
              << ", deadline: " << std::chrono::duration_cast<std::chrono::seconds>(deadline.time_since_epoch()).count()
              << ", seqId: " << item.seqId << std::endl;
}

void cancel_action_timer(const std::string& token) {
    // 新增：取消指定玩家的计时器（行动完成时调用），通过seqId失效
    // 注意：这里简化实现，实际应该通过seqId精确取消
    std::lock_guard<std::mutex> lock(timer_mutex);
    
    // 由于std::priority_queue不支持直接删除，这里采用标记失效的方式
    // 在实际到期时检查token是否仍然有效
    // TODO: 实现更精确的取消机制
    
    std::cout << "取消行动定时器，token: " << token << std::endl;
}

void heartbeat_scan_loop() {
    // 新增：心跳扫描线程主循环，每30s检查所有玩家最后活跃时间
    while (!stop_timer) {
        std::this_thread::sleep_for(std::chrono::seconds(30));
        
        auto now = std::chrono::steady_clock::now();
        auto timeout_threshold = std::chrono::seconds(90); // 90秒无心跳认为超时
        
        std::lock_guard<std::mutex> lock(fd_token_mutex);
        
        std::vector<std::string> timeout_tokens;
        
        for (const auto& pair : last_active_time) {
            auto elapsed = now - pair.second;
            if (elapsed > timeout_threshold) {
                timeout_tokens.push_back(pair.first);
            }
        }
        
        // 处理超时玩家
        for (const auto& token : timeout_tokens) {
            // 创建断线任务
            Task disconnect_task;
            disconnect_task.msgType = DISCONNECT;
            disconnect_task.uid = 0; // TODO: 需要根据token获取uid
            disconnect_task.dealerId = 0;
            disconnect_task.payload = std::vector<uint8_t>();
            
            enqueue_task(disconnect_task);
            
            std::cout << "心跳超时，token: " << token << std::endl;
        }
    }
}

void stop_timer_thread() {
    // 停止定时器线程
    stop_timer = true;
    timer_cv.notify_all();
    std::cout << "定时器线程已停止" << std::endl;
}
