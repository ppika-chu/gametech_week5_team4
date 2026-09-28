#pragma once

#include <unordered_set>

#include "Object.h"
#include "Actor.h"
#include "BVH.h"

#include "RenderInfo.h"
//struct FRenderInfo;

class UWorld final : public UObject
{
	REFLECT_CLASS(UWorld, UObject)
public:
	UWorld() = default;
	virtual ~UWorld();

	virtual void SerializeClass(json::JSON& outJson) const override;
	virtual void DeserializeClass(const json::JSON& inJson) override;

	void AddActor(AActor* actor);
	bool RemoveActor(uint32 componentUUID);


	TArray<AActor*>& GetActors() { return mActors; }

	void Tick(float deltaTime);
	void Render(float deltaTime, FRenderCollector& outCollector);

	UPrimitiveComponent* RayCastBVH(const FPickingRay& Ray, uint64& OutTestCount);
	void RequestBVHRebuild();
	void RequestBVHBoundsUpdate(UPrimitiveComponent* Component);


private:
	int32 getActorIndex(uint32 actorUUID) const;
	void RebuildBVH();
	void FlushBVHUpdates();

private:
	enum
	{
		DEFAULT_RESERVE_MEM = 1024U
	};
	
	// Todo: Must reserve
	TArray<AActor*> mActors;
	FBVH BVH;
	std::unordered_set<UPrimitiveComponent*> BVHBoundsUpdatePending;
	bool bBVHRebuildPending = true;



};
