#pragma once

#include <semaphore.h>
#include <pthread.h>
#include <exception>

/*
 *  RAIIȫ���� ��Resource Acquisition is Initialization����ֱ������ǡ���Դ��ȡ����ʼ����
 *  �ڹ��캯�������������Դ���������������ͷ���Դ����ΪC++�����Ի��Ʊ�֤�ˣ���һ�����󴴽���ʱ���Զ����ù��캯��
 *	�����󳬳��������ʱ����Զ������������������ԣ��� RAII ��ָ���£�Ӧ��ʹ������������Դ������Դ�Ͷ�����������ڰ�
 *	RAII �ĺ���˼���ǽ���Դ����״̬�������������ڰ󶨣�ͨ��C++�����Ի��ƣ�ʵ����Դ��״̬�İ�ȫ����, ����ָ���� RAII ��õ�����
*/

// �ź���
class Sem 
{
public:
	Sem() 
	{
		if (sem_init(&sem, 0, 0) != 0)
		{
			throw std::exception();
		}
	}
	~Sem()
	{
		sem_destroy(&sem);
	}
	bool wait()
	{
		return sem_wait(&sem) == 0;
	}
	bool post()
	{
		return sem_post(&sem) == 0;
	}
private:
	sem_t sem;
};

// ������
class Lock
{
public:
	Lock()
	{
		if (pthread_mutex_init(&mutex, nullptr) != 0)
		{
			throw std::exception();
		}
	}
	~Lock()
	{
		pthread_mutex_destroy(&mutex);
	}
	bool lock()
	{
		return pthread_mutex_lock(&mutex) == 0;
	}
	bool unlock()
	{
		return pthread_mutex_unlock(&mutex) == 0;
	}
	// ��ȡ����ַ
	pthread_mutex_t* get()
	{
		return &mutex;
	}
private:
	pthread_mutex_t mutex;
};


// ��������
class Condition
{
public:
	Condition()
	{
		if (pthread_cond_init(&cond, nullptr) != 0)
		{
			throw std::exception();
		}
	}
	~Condition()
	{
		pthread_cond_destroy(&cond);
	}
	bool wait(pthread_mutex_t& mutex)
	{
		int ret = 0;
		ret = pthread_cond_wait(&cond, &mutex);
		return ret == 0;
	}
	bool signal()
	{
		return pthread_cond_signal(&cond) == 0;
	}
	bool broadcast()
	{
		return pthread_cond_broadcast(&cond) == 0;
	}
private:
	pthread_cond_t cond;
};