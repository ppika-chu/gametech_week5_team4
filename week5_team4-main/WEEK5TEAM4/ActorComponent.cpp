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

void UActorComponent::Render(FRenderCollector& RenderCollector)
{
	// Todo: Do nothing, must override, some components may not call Update()
	// assert(false);
}

void UActorComponent::GetRenderInfos(TArray<FRenderInfo>* outRenderInfos) const
{
	// Todo: Do nothing, must override, some components may not call GetRenderInfos()
	// assert(false);
}

void UActorComponent::RegisterPickTarget(FRenderCollector& RenderCollector)
{
	// 충돌체가 없는 컴포넌트는 픽킹 대상이 아니다.
}
