#pragma once

#include <unordered_map>

#include "FAABB.h"
#include "RayCast.h"
#include "TArray.h"

class UPrimitiveComponent;

struct FBVHNode
{
	FAABB Bounds;
	int32 Parent = -1;
	int32 LeftChild = -1;
	int32 RightChild = -1;
	UPrimitiveComponent* Component = nullptr;

	bool IsLeaf() const { return Component != nullptr; }
};

struct FBVHRayHit
{
	UPrimitiveComponent* Component = nullptr;
	float EntryDistance = 0.0f;
};

class FBVH
{
public:
	void Clear();
	void Build(const TArray<UPrimitiveComponent*>& Components);

	bool SetLeafBounds(UPrimitiveComponent* Component, const FAABB& Bounds);
	bool UpdateLeafAndRefit(UPrimitiveComponent* Component, const FAABB& Bounds);
	void RefitAll();

	void QueryRay(const FPickingRay& Ray, TArray<FBVHRayHit>& OutHits) const;

	size_t GetLeafCount() const
	{
		return LeafIndices.size();
	}

private:
	struct FBuildItem
	{
		UPrimitiveComponent* Component = nullptr;
		FAABB Bounds;
		FVector Center;
	};

	static FAABB MergeBounds(const FAABB& A, const FAABB& B);
	static bool IntersectsRay(
		const FPickingRay& Ray,
		const FAABB& Bounds,
		float MaxDistance,
		float& OutEntryDistance);

	int32 BuildRecursive(int32 First, int32 Last, int32 Parent);

private:
	TArray<FBVHNode> Nodes;
	TArray<FBuildItem> BuildItems;
	std::unordered_map<UPrimitiveComponent*, int32> LeafIndices;
	int32 RootNode = -1;
};
