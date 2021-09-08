#pragma once

#include <semaphore.h>
#include <pthread.h>
#include <exception>

/*
 *  RAII全称是 “Resource Acquisition is Initialization”，直译过来是“资源获取即初始化”
 *  在构造函数中申请分配资源，在析构函数中释放资源。因为C++的语言机制保证了，当一个对象创建的时候，自动调用构造函数
 *	当对象超出作用域的时候会自动调用析构函数。所以，在 RAII 的指导下，应该使用类来管理资源，将资源和对象的生命周期绑定
 *	RAII 的核心思想是将资源或者状态与对象的生命周期绑定，通过C++的语言机制，实现资源和状态的安全管理, 智能指针是 RAII 最好的例子
*/

// 信号量
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

// 互斥锁
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
	// 获取锁地址
	pthread_mutex_t* get()
	{
		return &mutex;
	}
private:
	pthread_mutex_t mutex;
};


// 条件变量
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