#pragma once

#include <cassert>
#include <unordered_map>
#include <utility>
#include <initializer_list>

#include "Core.h"

template <typename T, typename V, typename HashFunc = std::hash<T>, typename EqualFunc = std::equal_to<T>>
class TMap
{
public:
	using MapType = std::unordered_map<T, V, HashFunc, EqualFunc>;
	using Iterator = typename MapType::iterator;

	TMap() = default;
	TMap(std::initializer_list<std::pair<const T, V>> initList) : mMap(initList) {}

	~TMap() = default;

	Iterator begin() {
		return mMap.begin();
	}

	Iterator end() {
		return mMap.end();
	}

	typename MapType::const_iterator begin() const {
		return mMap.cbegin();
	}

	typename MapType::const_iterator end() const {
		return mMap.cend();
	}

	void Add(const T& key, const V& Value);
	int32 Remove(const T& key);
	
	uint32 Num() const;
	void Reset();
	void Empty(int32 ExpectedNumElements = 0);
	V* Find(const T& key);
	const V* Find(const T& key) const;

	bool Contains(const T& key) const;
	bool IsEmpty() const;
	void Reserve(int32 Capacity);

	V& operator[](const T& key);
	const V& operator[](const T& key) const;

private:
	MapType mMap;
};

template  <typename T, typename V, typename HashFunc, typename EqualFunc>
inline void TMap<T, V, HashFunc, EqualFunc>::Add(const T& key, const V& value)
{
	mMap[key] = value;
}

template <typename T, typename V, typename HashFunc, typename EqualFunc>
inline int32 TMap<T, V, HashFunc, EqualFunc>::Remove(const T& key)
{
	return static_cast<int32>(mMap.erase(key));
}

template <typename T, typename V, typename HashFunc, typename EqualFunc>
inline uint32 TMap<T, V, HashFunc, EqualFunc>::Num() const
{
	return static_cast<uint32>(mMap.size());
}

template <typename T, typename V, typename HashFunc, typename EqualFunc>
inline void TMap<T, V, HashFunc, EqualFunc>::Reset()
{
	mMap.clear();
}

template <typename T, typename V, typename HashFunc, typename EqualFunc>
inline void TMap<T, V, HashFunc, EqualFunc>::Empty(int32 capacity)
{
	mMap.clear();
	mMap.reserve(static_cast<size_t>(capacity));
}

template <typename T, typename V, typename HashFunc, typename EqualFunc>
inline V* TMap<T, V, HashFunc, EqualFunc>::Find(const T& key)
{
	auto iter = mMap.find(key);
	if (iter == mMap.end())
	{
		return nullptr;
	}

	return &iter->second;
}

template <typename T, typename V, typename HashFunc, typename EqualFunc>
inline const V* TMap<T, V, HashFunc, EqualFunc>::Find(
	const T& key) const
{
	auto iter = mMap.find(key);

	if (iter == mMap.end())
	{
		return nullptr;
	}

	return &iter->second;
}

template <typename T, typename V, typename HashFunc, typename EqualFunc>
inline bool TMap<T, V, HashFunc, EqualFunc>::Contains(const T& key) const
{
	return mMap.find(key) != mMap.end();
}

template <typename T, typename V, typename HashFunc, typename EqualFunc>
inline bool TMap<T, V, HashFunc, EqualFunc>::IsEmpty() const
{
	return mMap.empty();
}

template <typename T, typename V, typename HashFunc, typename EqualFunc>
inline void TMap<T, V, HashFunc, EqualFunc>::Reserve(int32 capacity)
{
	mMap.reserve(static_cast<size_t>(capacity));
}

template <typename T, typename V, typename HashFunc, typename EqualFunc>
inline V& TMap<T, V, HashFunc, EqualFunc>::operator[](const T& key)
{
	return mMap[key];
}

template <typename T, typename V, typename HashFunc, typename EqualFunc>
inline const V& TMap<T, V, HashFunc, EqualFunc>::operator[](const T& key) const
{
	return mMap.at(key);
}
