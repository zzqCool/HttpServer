#include "HeapTimer.h"

void HeapTimer::sift_up_(size_t i)
{
	assert(i >= 0 && i < heap_.size());
	size_t j = (i - 1) / 2;			// ���ڵ�
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
	// i, j : vector �� TimeNode ������
	// heap_[i].id : TimeNode �е��ļ�������
	ref_[heap_[i].id] = i;
	ref_[heap_[j].id] = j;
}

bool HeapTimer::sift_down_(size_t index, size_t n)
{
	assert(index >= 0 && index < heap_.size());
	assert(n >= 0 && n <= heap_.size());
	size_t parent = index;
	size_t child = parent * 2 + 1;		// �ӽڵ�
	while (child < n) {
		// �����ӽڵ��໥�Ƚ�
		if (child + 1 < n && heap_[child + 1] < heap_[child]) {
			child++;
		}
		if (heap_[parent] < heap_[child]) break;
		swap_node_(parent, child);
		parent = child;
		child = parent * 2 + 1;
	}
	return parent > index;			// �Ƿ�����˽���
}

void HeapTimer::add(int id, int timeout, const TimeoutCallBack& cb)
{
	assert(id >= 0);
	size_t i;
	if (ref_.count(id) == 0)	// �½ڵ㣬��β���룬������
	{
		i = heap_.size();
		ref_[id] = i;
		heap_.push_back({ id, Clock::now() + MS(timeout), cb});
		sift_up_(i);
	}
	else						// ���нڵ㣬������
	{
		i = ref_[id];			// ��ȡ���׽����� vector �е�����
		heap_[i].expires = Clock::now() + MS(timeout);
		heap_[i].cb_func = cb;
		// �ȳ������Ͻ����������½��н���
		if (!sift_down_(i, heap_.size()))		// ���ϻ����ϵ���
		{
			sift_up_(i);
		}
	}
}

// ɾ��ָ���ڵ㣬�������ص�����
void HeapTimer::do_work(int id)
{
	if (heap_.empty() || ref_.count(id) == 0)	return;		// ������
	// ��������е�����
	size_t i = ref_[id];
	TimerNode node = heap_[i];
	node.cb_func();
	del_(i);
}

/* ɾ��ָ��λ�õĽ�� */
void HeapTimer::del_(size_t index)
{
	assert(!heap_.empty() && index >= 0 && index < heap_.size());
	// ����ɾ���ڵ㻻����β��������
	size_t i = index;
	size_t n = heap_.size() - 1;
	if (i < n)
	{
		swap_node_(i, n);			// ����β�ڵ���Ѷ��ڵ���н���
		if (!sift_down_(i, n-1))	// ����������Ķ�β�ڵ�
		{
			sift_up_(i);
		}
	}
	// ɾ������������β�Ľڵ�
	ref_.erase(heap_.back().id);
	heap_.pop_back();
}

void HeapTimer::adjust(int id, int timeout)
{
	assert(!heap_.empty() && ref_.count(id) > 0);
	heap_[ref_[id]].expires = Clock::now() + MS(timeout);;
	sift_down_(ref_[id], heap_.size());
}

// �����ʱ�ڵ㣬������һ��δ��ʱ�ڵ㷵��
void HeapTimer::tick()
{
	if (heap_.empty())	return;
	while (!heap_.empty())
	{
		TimerNode node = heap_.front();		// ��ʱʱ����С�ڵ�
		if (std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0)
		{	// δ��ʱ
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

// ��������һ�γ�ʱ�ļ��
ssize_t HeapTimer::get_next_tick() {
	tick();
	ssize_t res = -1;
	if (!heap_.empty()) {
		res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
		if (res < 0)	res = 0;
	}
	return res;
}