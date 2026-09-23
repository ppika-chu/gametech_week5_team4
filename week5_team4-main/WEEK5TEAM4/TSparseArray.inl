
template<typename T>
T& TSparseArray<T>::operator[](uint32 index)
{
	assert(index < mDatas.size());
	return mDatas[index].second.Element;
}

template<typename T>
const T& TSparseArray<T>::operator[](uint32 index) const
{
	assert(index < mDatas.size());
	return mDatas[index].second.Element;
}

template<typename T>
typename TSparseArray<T>::Iterator TSparseArray<T>::begin()
{
	return TSparseArray<T>::Iterator(this, 0);
}

template<typename T>
typename TSparseArray<T>::Iterator TSparseArray<T>::end()
{
	return TSparseArray<T>::Iterator(this, mDatas.size());
}

template<typename T>
typename TSparseArray<T>::ConstIterator TSparseArray<T>::begin() const
{
	return TSparseArray<T>::ConstIterator(this, 0);
}

template<typename T>
typename TSparseArray<T>::ConstIterator TSparseArray<T>::end() const
{
	return TSparseArray<T>::ConstIterator(this, mDatas.size());
}

template<typename T>
uint32 TSparseArray<T>::Add(const T& data)
{
	mNumElements++;
	if (mFreeIndex != -1)
	{
		uint32 index = mFreeIndex;
		mFreeIndex = mDatas[index].second.NextFreeIndex;
		mDatas[index] = { true, Slot{data} };
		return index;
	}
	else
	{
		mDatas.push_back({ true, Slot{data} });
		return static_cast<uint32>(mDatas.size() - 1);
	}
}

template<typename T>
uint32 TSparseArray<T>::Emplace(const T& data)
{
	mNumElements++;
	if (mFreeIndex != -1)
	{
		uint32 index = mFreeIndex;
		mFreeIndex = mDatas[index].second.NextFreeIndex;
		mDatas[index] = { true, Slot{data} };
		return index;
	}
	else
	{
		mDatas.emplace_back({ true, Slot{data} });
		return static_cast<uint32>(mDatas.size() - 1);
	}
}

template<typename T>
int32 TSparseArray<T>::Num() const
{
	return mNumElements;
}

template<typename T>
void TSparseArray<T>::Reserve(uint32 capacity)
{
	mDatas.reserve(capacity);
}

template<typename T>
int32 TSparseArray<T>::Size() const
{
	return static_cast<int32>(mDatas.size());
}

template<typename T>
int32 TSparseArray<T>::Max() const
{
	return static_cast<int32>(mDatas.capacity());
}

template<typename T>
bool TSparseArray<T>::IsEmpty() const
{
	return mNumElements == 0;
}

template<typename T>
bool TSparseArray<T>::IsValidIndex(uint32 index) const
{
	return index < mDatas.size() && mDatas[index].first;
}

template<typename T>
void TSparseArray<T>::Reset(int32 newSize)
{
	mDatas.clear();
	mDatas.reserve(newSize);
	mFreeIndex = -1;
	mNumElements = 0;
}

template<typename T>
void TSparseArray<T>::RemoveAt(uint32 index)
{
	assert(index < mDatas.size());
	if (mDatas[index].first) // If the slot is occupied
	{
		mDatas[index].first = false; // Mark as free
		mDatas[index].second.NextFreeIndex = mFreeIndex; // Link to the previous free index
		mFreeIndex = index; // Update the free index to this slot
		mNumElements--;
	}
}

template<typename T>
TArray<T> TSparseArray<T>::ToTArray() const
{
	TArray<T> result;
	result.Reserve(mNumElements);
	for (const auto& [occupied, slot] : mDatas)
	{
		if (occupied)
		{
			result.Add(slot.Element);
		}
	}
	return result;
}

template<typename T>
void TSparseArray<T>::Iterator::SkipEmpty()
{
	while (Index < Owner->mDatas.size() && !Owner->mDatas[Index].first)
	{
		++Index;
	}
}

template<typename T>
TSparseArray<T>::Iterator::Iterator(TSparseArray* owner, std::size_t index)
	: Owner(owner), Index(index)
{
	SkipEmpty();
}

template<typename T>
T& TSparseArray<T>::Iterator::operator*() const
{
	assert(Index < Owner->mDatas.size());
	return Owner->mDatas[Index].second.Element;
}

template<typename T>
T* TSparseArray<T>::Iterator::operator->() const
{
	assert(Index < Owner->mDatas.size());
	return &Owner->mDatas[Index].second.Element;
}

template<typename T>
TSparseArray<T>::Iterator& TSparseArray<T>::Iterator::operator++()
{
	++Index;
	SkipEmpty();
	return *this;
}

template<typename T>
bool TSparseArray<T>::Iterator::operator==(const Iterator& other) const
{
	return Owner == other.Owner && Index == other.Index;
}

template<typename T>
bool TSparseArray<T>::Iterator::operator!=(const Iterator& other) const
{
	return !(*this == other);
}

template<typename T>
void TSparseArray<T>::ConstIterator::SkipEmpty()
{
	while (Index < Owner->mDatas.size() && !Owner->mDatas[Index].first)
	{
		++Index;
	}
}

template<typename T>
TSparseArray<T>::ConstIterator::ConstIterator(TSparseArray* owner, std::size_t index)
	: Owner(owner), Index(index)
{
	SkipEmpty();
}

template<typename T>
const T& TSparseArray<T>::ConstIterator::operator*() const
{
	assert(Index < Owner->mDatas.size());
	return Owner->mDatas[Index].second.Element;
}

template<typename T>
const T* TSparseArray<T>::ConstIterator::operator->() const
{
	assert(Index < Owner->mDatas.size());
	return &Owner->mDatas[Index].second.Element;
}

template<typename T>
typename TSparseArray<T>::ConstIterator& TSparseArray<T>::ConstIterator::operator++()
{
	++Index;
	SkipEmpty();
	return *this;
}

template<typename T>
bool TSparseArray<T>::ConstIterator::operator==(const ConstIterator& other) const
{
	return Owner == other.Owner && Index == other.Index;
}

template<typename T>
bool TSparseArray<T>::ConstIterator::operator!=(const ConstIterator& other) const
{
	return !(*this == other);
}
