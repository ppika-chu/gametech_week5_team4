#pragma once

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
	//void Render();

	const FBVH& GetStaticBVH() const { return StaticBVH; }

	bool RayCastStaticBVH(const FPickingRay& Ray, FBVHRayHit& OutHit, FBVHRayQueryStats* OutStats = nullptr);

	void RebuildStaticBVH();
	void MarkStaticBVHDirty();
	void UpdateActorInStaticBVH(AActor* Actor);

private:
	int32 getActorIndex(uint32 actorUUID) const;

private:
	enum
	{
		DEFAULT_RESERVE_MEM = 1024U
	};
	
	// Todo: Must reserve
	TArray<AActor*> mActors;

	FBVH StaticBVH;
	bool bStaticBVHDirty = true;
	TArray<UPrimitiveComponent*> StaticPrimitives;
};
