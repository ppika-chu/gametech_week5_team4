#include "ActorComponent.h"
#include "RenderInfo.h"

UActorComponent::UActorComponent()
	: mOwner(nullptr)
{
}

UActorComponent::~UActorComponent()
{
}

void UActorComponent::SetOwner(AActor* owner)
{
	assert(mOwner == nullptr);

	mOwner = owner;
}

AActor* UActorComponent::GetOwner() const
{
	return mOwner;
}

void UActorComponent::Tick(float deltaTime)
{
}

void UActorComponent::Render(FRenderCollector& RenderCollector, const FAABB& WorldBounds)
{
	// Todo: Do nothing, must override, some components may not call Update()
	// assert(false);
}

FAABB UActorComponent::GetBoundingBox() const
{
	return FAABB();
}

void UActorComponent::GetRenderInfos(TArray<FRenderInfo>* outRenderInfos) const
{
	// Todo: Do nothing, must override, some components may not call GetRenderInfos()
	// assert(false);
}

void UActorComponent::RegisterPickTarget(FRenderCollector& RenderCollector, const FAABB& AABB)
{
	// 충돌체가 없는 컴포넌트는 픽킹 대상이 아니다.
}

void UActorComponent::SetEverTick()
{
	bCanEverTick = true;
}

void UActorComponent::SetTickEnabled(bool NewStatus)
{
	bTickEnabled = NewStatus;
}

bool UActorComponent::IsActiveTick() const
{
	return (bCanEverTick && bTickEnabled);
}

void UActorComponent::SetTickListIndex(int32 NewIndex)
{
	TickListIndex = NewIndex;
}

int32 UActorComponent::GetTickListIndex() const
{
	return (TickListIndex);
}