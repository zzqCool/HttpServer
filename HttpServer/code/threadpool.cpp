#include "threadpool.h"

ThreadPool::ThreadPool(int thread_number, int max_tasks) :
	task_threads(nullptr), min_num(4), max_num(thread_number), busy_num(0), 
	live_num(min_num), exit_num(0), tasks_capacity(max_tasks), shutdown(false)
{
	if (thread_number <= 0 || max_tasks <= 0)
	{
		throw std::exception();
	}

	// �������������߳�
	manager_thread = new pthread_t;
	pthread_create(manager_thread, NULL, manage, this);

	// �����̳߳����޴��������߳��鲢��ʼ��Ϊ��
	task_threads = new pthread_t[max_num];
	memset(task_threads, 0, sizeof(pthread_t) * max_num);

	if (!task_threads)
	{
		throw std::exception();
	}

	// ������С�߳���ѭ�����������߳�
	for (size_t i = 0; i < min_num; i++)
	{
		if (pthread_create(&task_threads[i], NULL, worker, this) != 0)
		{
			delete[] task_threads;
			throw std::exception();
		}
		// ���߳̽��з�����������ٶԵ����߳̽��л���
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
	// �������չ������߳�
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
		// ÿ�� 3s ���һ��
		sleep(3);

		// ��ȡ�̳߳��д���̺߳�æ�߳�����
		pool_lock.lock();
		size_t current_num = live_num;
		size_t work_num = busy_num;
		size_t queue_size = task_queue.size();
		pool_lock.unlock();
		printf("current_num is %d, work_num is %d, queue_size is %d\n", current_num, work_num, queue_size);
		// ����߳�
		// ������� > ����̸߳��� && ����߳��� < ����߳���
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

		// �����߳�
		// æ�߳���*2 < ����߳��� && ����߳��� > ��С�߳���
		if (work_num * 2 < current_num && current_num > min_num)
		{
			pool_lock.lock();
			exit_num = NUMBER;
			pool_lock.unlock();
			// �����ŵĵ��߳���ɱ
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
	// �������
	task_queue.push(task);
	pool_lock.unlock();
	queue_sem.post();
	return true;
}

void* ThreadPool::worker(void* arg)
{
	// arg ���ܴ��ݵ� this ����
	ThreadPool* pool = (ThreadPool*)arg;
	pool->run();
	return nullptr;
}

// ����������������
// ����ź���λ�ڻ�����֮ǰ����ֹ����߳��໥������������
void ThreadPool::run()
{
	while (!shutdown)
	{
		// ������̶߳�����������ʱ�������߳̽��ڴ������������������Ϊ��
		queue_sem.wait();
		// ��������Ƿ�Ϊ�գ�Ϊ������������
		pool_lock.lock();
		// ��ס�����̳߳�
		if (task_queue.empty())
		{
			// �ж��Ƿ���Ҫ�����߳�
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
		// �����������ȡ������
		std::function<void()> task = task_queue.front();
		task_queue.pop();
		busy_num++;
		pool_lock.unlock();

		task();				// ����ִ��

		busy_num_lock.lock();
		busy_num--;
		busy_num_lock.unlock();
	}
}

// �߳��˳�
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