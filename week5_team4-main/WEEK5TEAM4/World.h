#pragma once

#include "Object.h"
#include "Actor.h"

#include "RenderInfo.h"
#include "FBVH.h"

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
	FBVH& GetBVH() { return mBVH; }

private:
	int32 getActorIndex(uint32 actorUUID) const;
	FBVH mBVH;

private:
	enum
	{
		DEFAULT_RESERVE_MEM = 1024U
	};
	
	// Todo: Must reserve
	TArray<AActor*> mActors;



};
