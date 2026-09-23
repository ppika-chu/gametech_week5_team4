
#pragma once

#include <cassert>
#include <vector>
#include <initializer_list>
#include "Core.h"

template<typename T>
class TArray
{
public:
	TArray() = default;
	TArray(std::initializer_list<T> InitList)
		: mDatas(InitList)
	{
	}

	~TArray() = default;

	TArray(const TArray&) = default;
	TArray& operator=(const TArray&) = default;

	TArray(TArray&&) noexcept = default;
	TArray& operator=(TArray&&) noexcept = default;

	T& operator[](uint32 index);
	const T& operator[](uint32 index) const;

	std::vector<T>::iterator begin();
	std::vector<T>::iterator end();

	std::vector<T>::const_iterator begin() const;
	std::vector<T>::const_iterator end() const;

	std::vector<T>::reverse_iterator rbegin();
	std::vector<T>::reverse_iterator rend();

	std::vector<T>::const_reverse_iterator rbegin() const;
	std::vector<T>::const_reverse_iterator rend() const;

	// Todo: Delete, Memory leak if use pointer type
	void Init(const T& data, uint32 count);

	void SetNum(int32 NewNum, bool bAllowShrinking = true);

	uint32 Add(const T& data);

	template <typename... TArg>
	T& Emplace(TArg&&... args)
	{
		return mDatas.emplace_back(std::forward<TArg>(args)...);
	}

	uint32 Insert(const T& data, uint32 index);
	void Reserve(uint32 Number);

	int32 Num() const;
	int32 Max() const;
	
	bool IsEmpty() const;
	void Empty();

	void Reset(int32 newSize);
	void RemoveAt(uint32 index, int32 count);
	void RemoveAtSwap(uint32 index);
	void RemoveLast();

	T& Last()
	{
		assert(mDatas.empty() == false);
		return mDatas.back();
	}

	inline const T* Data() const { return mDatas.data(); }
	inline T* Data() { return mDatas.data(); }

	/*
	int32 Find(const ElementType& Item) const;
	template <typename Predicate>
	int32 FindByPredicate(Predicate Pred) const;
	bool Contains(const ElementType& Item) const;
	T& Top();
	const T& Top() const;

	int32 RemoveSingle(const ElementType& Item);
	int32 RemoveAll(const ElementType& Item); 
	void RemoveAtSwap(int32 Index, int32 Count = 1, bool bAllowShrinking = true);
	*/

private:
	std::vector<T> mDatas;
};

template<typename T>
inline T& TArray<T>::operator[](uint32 index)
{
	assert(index < mDatas.size());

	return mDatas[index];
}

template<typename T>
inline const T& TArray<T>::operator[](uint32 index) const
{
	assert(index < mDatas.size());

	return mDatas[index];
}

template<typename T>
inline std::vector<T>::iterator TArray<T>::begin()
{
	return mDatas.begin();
}

template<typename T>
inline std::vector<T>::iterator TArray<T>::end()
{
	return mDatas.end();
}

template<typename T>
inline std::vector<T>::const_iterator TArray<T>::begin() const
{
	return mDatas.cbegin();
}

template<typename T>
inline std::vector<T>::const_iterator TArray<T>::end() const
{
	return mDatas.cend();
}

template<typename T>
inline std::vector<T>::reverse_iterator TArray<T>::rbegin()
{
	return mDatas.rbegin();
}

template<typename T>
inline std::vector<T>::reverse_iterator TArray<T>::rend()
{
	return mDatas.rend();
}

template<typename T>
inline std::vector<T>::const_reverse_iterator TArray<T>::rbegin() const
{
	return mDatas.crbegin();
}

template<typename T>
inline std::vector<T>::const_reverse_iterator TArray<T>::rend() const
{
	return mDatas.crend();
}


// Todo: Need to fix code
template<typename T>
inline void TArray<T>::Init(const T& data, uint32 count)
{
	// Todo: Check memory leak
	// Memory leak if use pointer type on data
	mDatas.assign(count, data);
}

template <typename T>
inline void TArray<T>::SetNum(int32 NewNum, bool bAllowShrinking)
{
	if (NewNum < 0)
	{
		NewNum = 0;
	}

	if (bAllowShrinking || NewNum > static_cast<int32>(mDatas.size()))
	{
		mDatas.resize(NewNum);
	}
}

template<typename T>
inline uint32 TArray<T>::Add(const T& data)
{
	mDatas.push_back(data);

	return static_cast<uint32>(mDatas.size()) - 1;
}

template<typename T>
inline uint32 TArray<T>::Insert(const T& data, uint32 index)
{
	mDatas.insert(mDatas.begin() + index, data);

	return index;
}

template<typename T>
inline int32 TArray<T>::Num() const
{
	return static_cast<uint32>(mDatas.size());
}

template<typename T>
inline void TArray<T>::Reserve(uint32 capacity)
{
	mDatas.reserve(capacity);
}

template<typename T>
inline int32 TArray<T>::Max() const
{
	return static_cast<uint32>(mDatas.capacity());
}

template<typename T>
inline bool TArray<T>::IsEmpty() const
{
	return mDatas.empty();
}

template <typename T>
inline void TArray<T>::Empty()
{
	mDatas.clear();
}

template<typename T>
inline void TArray<T>::Reset(int32 newSize)
{
	mDatas.clear();

	// Does not reduce memory size
	mDatas.reserve(newSize);
}

template<typename T>
inline void TArray<T>::RemoveAt(uint32 index, int32 count)
{
	assert(mDatas.empty() == false);
	assert(index < mDatas.size());
	assert((index + count) <= mDatas.size());

	auto removeBeginIter = mDatas.begin() + index;
	mDatas.erase(removeBeginIter, removeBeginIter + count);
}

template<typename T>
inline void TArray<T>::RemoveAtSwap(uint32 index)
{
	assert(mDatas.empty() == false);
	assert(index < mDatas.size());

	T moveData = mDatas.back();
	mDatas[index] = moveData;

	RemoveLast();
}

template<typename T>
inline void TArray<T>::RemoveLast()
{
	assert(mDatas.empty() == false);

	mDatas.erase(mDatas.begin() + mDatas.size() - 1);

}







