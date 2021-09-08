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
    // 文件描述符
	int id;
	TimeStamp expires;
	TimeoutCallBack cb_func;
	bool operator<(const TimerNode& t)
	{
		return expires < t.expires;
	}
};

// 小顶堆
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
    void sift_up_(size_t i);                        // 上浮
    bool sift_down_(size_t index, size_t n);        // 下沉
    void swap_node_(size_t i, size_t j);            

    std::vector<TimerNode> heap_;
    // 套接字与索引位置
    std::unordered_map<int, size_t> ref_;
};

