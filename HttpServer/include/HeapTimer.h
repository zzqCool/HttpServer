#pragma once

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h> 
#include <functional> 
#include <assert.h> 
#include <chrono>

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode
{
    // �ļ�������
	int id;
	TimeStamp expires;
	TimeoutCallBack cb_func;
	bool operator<(const TimerNode& t)
	{
		return expires < t.expires;
	}
};

// С����
class HeapTimer
{
public:
    HeapTimer() { heap_.reserve(64); }
    ~HeapTimer() { clear(); }

    void adjust(int id, int new_expires);
    void add(int id, int timeout, const TimeoutCallBack& cb);
    void pop();
    void do_work(int id);
    void clear();
    void tick();
    ssize_t get_next_tick();

private:
    void del_(size_t i);
    void sift_up_(size_t i);                        // �ϸ�
    bool sift_down_(size_t index, size_t n);        // �³�
    void swap_node_(size_t i, size_t j);            

    std::vector<TimerNode> heap_;
    // �׽���������λ��
    std::unordered_map<int, size_t> ref_;
};

