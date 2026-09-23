#pragma once

#pragma once

#include <cassert>
#include <queue>
#include <utility>
#include "Core.h"

template <typename T>
class TQueue
{
public:
	TQueue() = default;
	~TQueue() = default;

	bool Enqueue(const T& item);
	bool Dequeue(T* outItem);

	bool Peek(T* outItem) const;
	T* Peek();

	bool IsEmpty() const;
	uint32 Num() const;

	void Empty();
	void Reset();

private:
	std::queue<T> mQueue;
};

template<typename T>
inline bool TQueue<T>::Enqueue(const T& item)
{
	mQueue.push(item);

	return true;
}

template<typename T>
inline bool TQueue<T>::Dequeue(T* outItem)
{
	assert(outItem != nullptr);

	if (mQueue.empty())
	{
		return false;
	}

	*outItem = std::move(mQueue.front());
	mQueue.pop();

	return true;
}

template<typename T>
inline bool TQueue<T>::Peek(T* outItem) const
{
	assert(outItem != nullptr);

	if (mQueue.empty())
	{
		return false;
	}

	*outItem = mQueue.front();

	return true;
}

template<typename T>
inline T* TQueue<T>::Peek()
{
	if (mQueue.empty())
	{
		return nullptr;
	}

	return &mQueue.front();
}

template<typename T>
inline bool TQueue<T>::IsEmpty() const
{
	return mQueue.empty();
}

template<typename T>
inline uint32 TQueue<T>::Num() const
{
	return static_cast<uint32>(mQueue.size());
}

template<typename T>
inline void TQueue<T>::Empty()
{
	std::queue<T> emptyQueue;
	std::swap(mQueue, emptyQueue);
}

template<typename T>
inline void TQueue<T>::Reset()
{
	Empty();
}
