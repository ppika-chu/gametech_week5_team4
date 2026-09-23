#pragma once

#include <cassert>
#include <unordered_set>

#include "Core.h"

template <typename T>
class TSet
{
public:
	TSet() = default;
	~TSet() = default;

	bool Add(const T& data);

	// Returns removed count
	int32 Remove(const T& data);

	uint32 Num() const;
	void Reset();

	bool Contains(const T& data) const;
	bool IsEmpty() const;
	void Reserve(int32 capacity);

	/*
	void Empty(int32 ExpectedNumElements = 0)
	*/

private:
	std::unordered_set<T> mSet;
};

template<typename T>
inline bool TSet<T>::Add(const T& data)
{
	return mSet.insert(data).second;
}

template<typename T>
inline int32 TSet<T>::Remove(const T& data)
{
	return mSet.erase(data);
}

template<typename T>
inline uint32 TSet<T>::Num() const
{
	return static_cast<uint32>(mSet.size());
}

template<typename T>
inline void TSet<T>::Reset()
{
	mSet.clear();
}

template<typename T>
inline bool TSet<T>::Contains(const T& data) const
{
	return mSet.find(data) != mSet.end() ? true : false;
}

template<typename T>
inline bool TSet<T>::IsEmpty() const
{
	return mSet.size() == 0 ? true : false;
}

template<typename T>
inline void TSet<T>::Reserve(int32 capacity)
{
	mSet.reserve(capacity);
}

