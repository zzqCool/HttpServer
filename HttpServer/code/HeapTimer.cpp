#include "HeapTimer.h"

void HeapTimer::sift_up_(size_t i)
{
	assert(i >= 0 && i < heap_.size());
	size_t j = (i - 1) / 2;			// 父节点
	while (j >= 0)
	{
		if (heap_[j] < heap_[i])	break;
		swap_node_(i, j);
		i = j;
		j = (i - 1) / 2;
	}
}

void HeapTimer::swap_node_(size_t i, size_t j)
{
	assert(i >= 0 && i < heap_.size());
	assert(j >= 0 && j < heap_.size());
	std::swap(heap_[i], heap_[j]);
	// i, j : vector 中 TimeNode 的索引
	// heap_[i].id : TimeNode 中的文件描述符
	ref_[heap_[i].id] = i;
	ref_[heap_[j].id] = j;
}

bool HeapTimer::sift_down_(size_t index, size_t n)
{
	assert(index >= 0 && index < heap_.size());
	assert(n >= 0 && n <= heap_.size());
	size_t parent = index;
	size_t child = parent * 2 + 1;		// 子节点
	while (child < n) {
		// 左右子节点相互比较
		if (child + 1 < n && heap_[child + 1] < heap_[child]) {
			child++;
		}
		if (heap_[parent] < heap_[child]) break;
		swap_node_(parent, child);
		parent = child;
		child = parent * 2 + 1;
	}
	return parent > index;			// 是否进行了交换
}

void HeapTimer::add(int id, int timeout, const TimeoutCallBack& cb)
{
	assert(id >= 0);
	size_t i;
	if (ref_.count(id) == 0)	// 新节点，堆尾插入，调整堆
	{
		i = heap_.size();
		ref_[id] = i;
		heap_.push_back({ id, Clock::now() + MS(timeout), cb});
		sift_up_(i);
	}
	else						// 已有节点，调整堆
	{
		i = ref_[id];			// 获取该套接字在 vector 中的索引
		heap_[i].expires = Clock::now() + MS(timeout);
		heap_[i].cb_func = cb;
		// 先尝试向上交换，再向下进行交换
		if (!sift_down_(i, heap_.size()))		// 向上或向上调整
		{
			sift_up_(i);
		}
	}
}

// 删除指定节点，并触发回调函数
void HeapTimer::do_work(int id)
{
	if (heap_.empty() || ref_.count(id) == 0)	return;		// 不存在
	// 获得数组中的索引
	size_t i = ref_[id];
	TimerNode node = heap_[i];
	node.cb_func();
	del_(i);
}

/* 删除指定位置的结点 */
void HeapTimer::del_(size_t index)
{
	assert(!heap_.empty() && index >= 0 && index < heap_.size());
	// 将被删除节点换至对尾，调整堆
	size_t i = index;
	size_t n = heap_.size() - 1;
	if (i < n)
	{
		swap_node_(i, n);			// 将队尾节点与堆顶节点进行交换
		if (!sift_down_(i, n-1))	// 调整交换后的堆尾节点
		{
			sift_up_(i);
		}
	}
	// 删除被交换至堆尾的节点
	ref_.erase(heap_.back().id);
	heap_.pop_back();
}

void HeapTimer::adjust(int id, int timeout)
{
	assert(!heap_.empty() && ref_.count(id) > 0);
	heap_[ref_[id]].expires = Clock::now() + MS(timeout);;
	sift_down_(ref_[id], heap_.size());
}

// 清除超时节点，遇到第一个未超时节点返回
void HeapTimer::tick()
{
	if (heap_.empty())	return;
	while (!heap_.empty())
	{
		TimerNode node = heap_.front();		// 超时时间最小节点
		if (std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0)
		{	// 未超时
			break;
		}
		node.cb_func();
		pop();
	}
}

void HeapTimer::pop()
{
	assert(!heap_.empty());
	del_(0);
}

void HeapTimer::clear() {
	ref_.clear();
	heap_.clear();
}

// 获得离最近一次超时的间隔
ssize_t HeapTimer::get_next_tick() {
	tick();
	ssize_t res = -1;
	if (!heap_.empty()) {
		res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
		if (res < 0)	res = 0;
	}
	return res;
}