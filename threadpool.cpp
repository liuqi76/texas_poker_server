/*
线程池维护和使用等函数

threadpool_init、enqueue_task、worker_loop、
任务队列的 mutex 和 condition_variable
*/

#include "threadpool.h"
#include "types.h"
#include "handler.h"
#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

// 全局任务队列和同步原语
std::queue<Task> task_queue;
std::mutex task_queue_mutex;
std::condition_variable task_queue_cv;
std::atomic<bool> stop_threadpool{false};

void threadpool_init(int worker_count) {
    // 新增：启动固定数量的worker线程，阻塞等待任务队列
    for (int i = 0; i < worker_count; ++i) {
        std::thread worker(worker_loop);
        worker.detach();
        std::cout << "工作线程 " << i + 1 << " 已启动" << std::endl;
    }
    std::cout << "线程池初始化完成，共 " << worker_count << " 个工作线程" << std::endl;
}

void enqueue_task(Task task) {
    // 新增：将Task投入全局任务队列，唤醒一个worker
    {
        std::lock_guard<std::mutex> lock(task_queue_mutex);
        task_queue.push(std::move(task));
    }
    task_queue_cv.notify_one();
}

void worker_loop() {
    // 新增：worker线程主循环，取Task并路由到对应处理函数
    while (!stop_threadpool) {
        Task task;
        
        {
            std::unique_lock<std::mutex> lock(task_queue_mutex);
            
            // 等待任务或停止信号
            task_queue_cv.wait(lock, [] {
                return !task_queue.empty() || stop_threadpool;
            });
            
            if (stop_threadpool && task_queue.empty()) {
                return;
            }
            
            // 获取任务
            task = std::move(task_queue.front());
            task_queue.pop();
        }
        
        // 处理任务
        try {
            dispatch_task(task);
        } catch (const std::exception& e) {
            std::cerr << "处理任务时发生异常: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "处理任务时发生未知异常" << std::endl;
        }
    }
}

void stop_threadpool() {
    // 停止线程池
    stop_threadpool = true;
    task_queue_cv.notify_all();
    std::cout << "线程池已停止" << std::endl;
}

