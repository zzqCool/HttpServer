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
 *	�̳߳���ɲ���
 *		������У��洢�������񣬽��������߳�
 *		�����̣߳�������е�������
 *		�����̣߳������Լ���̣߳�������Ҫ���߳̽������Ӻ�ɾ��
 */

class ThreadPool {
public:
	ThreadPool(int thread_number = 16, int max_tasks = 1000);
	~ThreadPool();
	bool append(std::function<void()> task);

private:
	// �̺߳��������Ǿ�̬�����������ຯ�����Զ����� this ָ���������˱��뽫�ú�������Ϊ��̬����
	static void* worker(void* arg);
	static void* manage(void* arg);
	void run();					// �����߳�
	void manage_thread();		// �����߳�
	void thread_exit();			// �߳��˳�
	void close_pool();			// �̳߳��˳�

private:
	pthread_t* manager_thread;			// �̳߳��еĹ����߳�
	pthread_t* task_threads;			// �̳߳��еĵĹ����߳�
	size_t min_num;						// ��С�߳���
	size_t max_num;						// ����߳���
	size_t busy_num;					// æ�߳���
	size_t live_num;					// ����߳���
	size_t exit_num;					// ��Ҫ�����߳���

	// �������
	std::queue<std::function<void()>> task_queue;			
	size_t tasks_capacity;				// ������������������

	Lock pool_lock;						// �����̳߳ػ�����
	Lock busy_num_lock;					// busy_num ������
	Sem queue_sem;						// ��������ź���

	bool shutdown;						// �Ƿ������̳߳�

	static const int NUMBER = 2;
};


