#pragma once

#include "Object.h"
struct FAABB;

struct FRenderInfo;
class FRenderCollector;

enum EActorComponentFlags
{
	EditorOnly = 1 << 0, // 에디터에서만 존재하는 컴포넌트. 게임에서는 제거된다.
	DoNotSerialize = 1 << 1, // 직렬화하지 않는다. (에디터에서만 존재하는 컴포넌트는 기본적으로 직렬화하지 않는다.)
};

class UActorComponent : public UObject
{
	REFLECT_CLASS(UActorComponent, UObject)
public:
	UActorComponent();
	virtual ~UActorComponent();

	void SetOwner(AActor* owner);
	AActor* GetOwner() const;

	// Todo: Make as pure class
	virtual void Tick(float deltaTime);
	virtual void Render(FRenderCollector& RenderCollector, const FAABB& WorldBounds);
	virtual FAABB GetBoundingBox() const;
	virtual void GetRenderInfos(TArray<FRenderInfo>* outRenderInfos) const;

	void SetEverTick();
	void SetTickEnabled(bool NewStatus);
	bool IsActiveTick() const;
	void SetTickListIndex(int32 NewIndex);
	int32 GetTickListIndex() const;

	// 이 컴포넌트가 마우스 픽킹 대상이면 컬렉터에 자신을 등록한다.
	// 기본은 등록하지 않는다. 충돌체가 있는 컴포넌트만 재정의한다.
	virtual void RegisterPickTarget(FRenderCollector& RenderCollector, const FAABB& AABB);

	inline void SetEditorOnly(bool bEditorOnly) 
	{ 
		if (bEditorOnly)
		{
			mComponentFlags |= EActorComponentFlags::EditorOnly; 
		}
		else
		{
			mComponentFlags &= ~EActorComponentFlags::EditorOnly;
		}
	}
	
	inline bool IsEditorOnly() const { return (mComponentFlags & EActorComponentFlags::EditorOnly) != 0; }

	inline void SetDoNotSerialize(bool bDoNotSerialize) 
	{ 
		if (bDoNotSerialize)
		{
			mComponentFlags |= EActorComponentFlags::DoNotSerialize; 
		}
		else
		{
			mComponentFlags &= ~EActorComponentFlags::DoNotSerialize;
		}
	}

	inline bool ShouldSerialize() const { return (mComponentFlags & EActorComponentFlags::DoNotSerialize) == 0; }

protected:
	AActor* mOwner;

private:
	uint32 mComponentFlags = 0;
	bool bCanEverTick = false;
	bool bTickEnabled = true;
	// 틱 목록 안의 위치. -1이면 등록 안 됨
	int32 TickListIndex = -1;
};

