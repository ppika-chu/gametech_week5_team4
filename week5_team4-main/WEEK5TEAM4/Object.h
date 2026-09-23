#pragma once

#include <functional>

#include "Core.h"
#include "TArray.h"
#include "TSparseArray.h"
#include "ObjectFactory.h"

namespace json { class JSON; }

class UObject;

struct FClassInfo
{
	FString Name;
	const FClassInfo* SuperClass;
	std::function<UObject* ()> Constructor;

	FClassInfo(FString name, const FClassInfo* superClass, std::function<UObject* ()> constructor)
		: Name(std::move(name)), SuperClass(superClass), Constructor(constructor) {
	}

	UObject* CreateInstance() const;

	bool IsChildOf(const FClassInfo* other) const;

private:
};

class UObject
{
public:
	// Todo: Fix
	int32 UUID;
	uint32 InternalIndex;
	uint32 ObjectMapIndex;

	virtual ~UObject();

	void Initialize();

	// StaticClass() in Unreal Engine
	static const FClassInfo* GetStaticClass();

	// GetClass() in Unreal Engine
	virtual const FClassInfo* GetClass() const { return GetStaticClass(); }

	// TODO?: Replace json type with a more generic type, such as a variant or a map
	virtual void SerializeClass(json::JSON& outJson) const;
	virtual void DeserializeClass(const json::JSON& inJson);

	template<typename TObject>
		requires std::derived_from<TObject, UObject>
	bool IsA() const;

	bool IsA(const FClassInfo* classInfo) const;

	template<typename TObject>
		requires std::derived_from<TObject, UObject>
	TObject* Cast();

	static UObject* GetObjectByUUID(int32 uuid);
	static UObject* GetObjectByInternalIndex(uint32 internalIndex);

	template<typename TObject>
		requires std::derived_from<TObject, UObject>
	static TObject* GetObjectByUUID(int32 uuid);

	template<typename TObject>
		requires std::derived_from<TObject, UObject>
	static TObject* GetObjectByInternalIndex(uint32 internalIndex);

	static TSparseArray<UObject*>& GetGObjectArray() { return GUObjectArray; }
	inline static uint64 GetGObjectRevision() { return GUObjectRevision; }

public:
	static TSparseArray<UObject*> GUObjectArray;
	static TMap<const FClassInfo*, TArray<uint32>> GUObjectMap;

protected:
	UObject();
	inline static uint64 GUObjectRevision = 0;

private:
	friend struct FObjectFactory;
	
	template <typename T>
	friend class TObjectIterator;
};

// 이터레이터를 사용하면 for문 안에서의 Object Insert/Remove는 허용하지 않는다. (나중에 지연 삭제를 구현할 수 있다면 허용 가능)
template <typename T>
class TObjectIterator
{
public:
	TObjectIterator(bool bIncludeChildren = false)
	{
		if (bIncludeChildren)
		{
			for (auto& Pair : UObject::GUObjectMap)
			{
				const FClassInfo* classInfo = Pair.first;
				if (classInfo->IsChildOf(T::GetStaticClass()))
				{
					mClassInfos.Add(&Pair.second);
				}
			}
		}
		else
		{
			mClassInfos.Add(&UObject::GUObjectMap[T::GetStaticClass()]);
		}

		if (!mClassInfos.IsEmpty())
		{
			mCurrentClassInfoIndex = 0;
			const TArray<uint32>& ObjectIndices = *mClassInfos[mCurrentClassInfoIndex];
			if (!ObjectIndices.IsEmpty())
			{
				mCurrentObjectIndex = 0;
			}
		}
	}

	explicit operator bool() const
	{
		return mCurrentClassInfoIndex != -1 && mCurrentObjectIndex != -1;
	}

	T* operator*() const
	{
		const TArray<uint32>& ObjectIndices = *mClassInfos[mCurrentClassInfoIndex];
		uint32 ObjectIndex = ObjectIndices[mCurrentObjectIndex];
		UObject* Object = UObject::GUObjectArray[ObjectIndex];
		return Object->Cast<T>();
	}

	TObjectIterator& operator++()
	{
		if (mCurrentClassInfoIndex == -1 || mCurrentObjectIndex == -1)
		{
			return *this;
		}

		const TArray<uint32>& ObjectIndices = *mClassInfos[mCurrentClassInfoIndex];

		mCurrentObjectIndex++;
		if (mCurrentObjectIndex >= ObjectIndices.Num())
		{
			mCurrentClassInfoIndex++;
			if (mCurrentClassInfoIndex >= mClassInfos.Num())
			{
				// 모든 클래스와 객체를 순회했으므로 반복자를 끝으로 설정
				mCurrentClassInfoIndex = -1;
				mCurrentObjectIndex = -1;
				return *this;
			}

			mCurrentObjectIndex = 0;
		}

		return *this;
	}

private:
	TArray<TArray<uint32>*> mClassInfos;
	int32 mCurrentClassInfoIndex = -1;
	int32 mCurrentObjectIndex = -1;
};

#include  "Object.inl"
