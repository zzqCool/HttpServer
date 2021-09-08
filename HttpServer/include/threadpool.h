#pragma once

#include <unistd.h>
#include <pthread.h>
#include <iostream>
#include <string>
#include <string.h>
#include <functional>
#include <queue>
#include <semaphore.h>
#include "locker.h"

/*
 *	线程池组成部分
 *		任务队列：存储处理任务，交给工作线程
 *		工作线程：任务队列的消费者
 *		管理线程：周期性检测线程，根据需要对线程进行增加和删除
 */

class ThreadPool {
public:
	ThreadPool(int thread_number = 16, int max_tasks = 1000);
	~ThreadPool();
	bool append(std::function<void()> task);

private:
	// 线程函数必须是静态函数，由于类函数会自动传递 this 指针参数，因此必须将该函数定义为静态函数
	static void* worker(void* arg);
	static void* manage(void* arg);
	void run();					// 工作线程
	void manage_thread();		// 管理线程
	void thread_exit();			// 线程退出
	void close_pool();			// 线程池退出

private:
	pthread_t* manager_thread;			// 线程池中的管理线程
	pthread_t* task_threads;			// 线程池中的的工作线程
	size_t min_num;						// 最小线程数
	size_t max_num;						// 最大线程数
	size_t busy_num;					// 忙线程数
	size_t live_num;					// 存活线程数
	size_t exit_num;					// 需要销毁线程数

	// 任务队列
	std::queue<std::function<void()>> task_queue;			
	size_t tasks_capacity;				// 任务队列最大任务数量

	Lock pool_lock;						// 整个线程池互斥锁
	Lock busy_num_lock;					// busy_num 互斥锁
	Sem queue_sem;						// 任务队列信号量

	bool shutdown;						// 是否销毁线程池

	static const int NUMBER = 2;
};


