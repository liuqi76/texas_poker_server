/*
线程池相关函数声明
*/

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "types.h"

// 初始化线程池
void threadpool_init(int worker_count);

// 加入任务
void enqueue_task(Task task);

// 工作线程循环
void worker_loop();

// 停止线程池
void stop_threadpool();

#endif // THREADPOOL_H