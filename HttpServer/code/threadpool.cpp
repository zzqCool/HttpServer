#include "threadpool.h"

ThreadPool::ThreadPool(int thread_number, int max_tasks) :
	task_threads(nullptr), min_num(4), max_num(thread_number), busy_num(0), 
	live_num(min_num), exit_num(0), tasks_capacity(max_tasks), shutdown(false)
{
	if (thread_number <= 0 || max_tasks <= 0)
	{
		throw std::exception();
	}

	// 单独创建管理线程
	manager_thread = new pthread_t;
	pthread_create(manager_thread, NULL, manage, this);

	// 根据线程池上限创建任务线程组并初始化为零
	task_threads = new pthread_t[max_num];
	memset(task_threads, 0, sizeof(pthread_t) * max_num);

	if (!task_threads)
	{
		throw std::exception();
	}

	// 根据最小线程数循环创建工作线程
	for (size_t i = 0; i < min_num; i++)
	{
		if (pthread_create(&task_threads[i], NULL, worker, this) != 0)
		{
			delete[] task_threads;
			throw std::exception();
		}
		// 对线程进行分离操作，不再对单独线程进行回收
		if (pthread_detach(task_threads[i]))
		{
			delete[] task_threads;
			throw std::exception();
		}
	}
}

ThreadPool::~ThreadPool()
{
	close_pool();
}

void ThreadPool::close_pool()
{
	shutdown = true;
	// 阻塞回收管理者线程
	pthread_join(*manager_thread, nullptr);
	for (size_t i = 0; i < live_num; i++)
	{
		queue_sem.post();
	}
	if (manager_thread)	delete manager_thread;
	if (task_threads)	delete[] task_threads;
}

void* ThreadPool::manage(void* arg)
{
	ThreadPool* pool = (ThreadPool*)arg;
	pool->manage_thread();
	return nullptr;
}

void ThreadPool::manage_thread()
{
	while (!shutdown)
	{
		// 每隔 3s 检测一次
		sleep(3);

		// 获取线程池中存活线程和忙线程数量
		pool_lock.lock();
		size_t current_num = live_num;
		size_t work_num = busy_num;
		size_t queue_size = task_queue.size();
		pool_lock.unlock();
		printf("current_num is %d, work_num is %d, queue_size is %d\n", current_num, work_num, queue_size);
		// 添加线程
		// 任务个数 > 存活线程个数 && 存活线程数 < 最大线程数
		if (queue_size > current_num && current_num < max_num)
		{
			pool_lock.lock();
			int count = 0;
			for (size_t i = 0; i < max_num && count < NUMBER && current_num < max_num; i++)
			{
				if (task_threads[i] == 0)
				{
					pthread_create(&task_threads[i], NULL, worker, this);
					count++;
					live_num++;
					printf("create tasks_thread[%d]\n", i);
				}
			}
			pool_lock.unlock();
		}

		// 销毁线程
		// 忙线程数*2 < 存活线程数 && 存活线程数 > 最小线程数
		if (work_num * 2 < current_num && current_num > min_num)
		{
			pool_lock.lock();
			exit_num = NUMBER;
			pool_lock.unlock();
			// 让闲着的的线程自杀
			for (int i = 0; i < NUMBER; i++)
			{
				queue_sem.post();
			}
		}
	}
}

bool ThreadPool::append(std::function<void()> task)
{
	pool_lock.lock();
	if (task_queue.size() >= tasks_capacity && !shutdown)
	{
		pool_lock.unlock();
		return false;
	}
	if (shutdown)
	{
		pool_lock.unlock();
		return false;
	}
	// 添加任务
	task_queue.push(task);
	pool_lock.unlock();
	queue_sem.post();
	return true;
}

void* ThreadPool::worker(void* arg)
{
	// arg 接受传递的 this 参数
	ThreadPool* pool = (ThreadPool*)arg;
	pool->run();
	return nullptr;
}

// 生产者消费者问题
// 因此信号量位于互斥锁之前，防止多个线程相互竞争导致死锁
void ThreadPool::run()
{
	while (!shutdown)
	{
		// 当存活线程多于任务数量时，多余线程将在此阻塞，并且任务队列为空
		queue_sem.wait();
		// 任务队列是否为空，为空则阻塞队列
		pool_lock.lock();
		// 锁住整个线程池
		if (task_queue.empty())
		{
			// 判断是否需要销毁线程
			if (exit_num > 0)
			{
				exit_num--;
				if (live_num > min_num)
				{
					live_num--;
					pool_lock.unlock();
					thread_exit();
				}
			}
		}
		// 从任务队列中取出任务
		std::function<void()> task = task_queue.front();
		task_queue.pop();
		busy_num++;
		pool_lock.unlock();

		task();				// 任务执行

		busy_num_lock.lock();
		busy_num--;
		busy_num_lock.unlock();
	}
}

// 线程退出
void ThreadPool::thread_exit()
{
	pthread_t tid = pthread_self();
	for (size_t i = 0; i < max_num; ++i)
	{
		if (task_threads[i] == tid)
		{
			task_threads[i] = 0;
			printf("tasks_thread[%d] is exiting\n", i);
			break;
		}
	}
	pthread_exit(NULL);
}