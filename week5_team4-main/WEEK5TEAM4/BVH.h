#pragma once

#include <cfloat>
#include <unordered_map>

#include "FAABB.h"
#include "RayCast.h"
#include "TArray.h"

class UPrimitiveComponent;

struct FBVHRayHit
{
	UPrimitiveComponent* Component = nullptr;

	// Near에서 Far까지의 선분 매개변수.
	// 0이면 Near, 1이면 Far다.
	float HitT = FLT_MAX;

	// Near에서 충돌 지점까지의 월드 공간 거리다.
	float HitDistanceWorld = FLT_MAX;
};

struct FBVHRayQueryStats
{
	uint32 TestedNodes = 0;
	uint32 TestedEntries = 0;
	uint32 NarrowPhaseTests = 0;
	uint32 PrunedNodes = 0;
};

class FBVH
{
public:
	void Clear();

	// Picking BVH에 등록할 정적 Primitive만 전달한다.
	// 유효한 Mesh와 AABB를 가진 컴포넌트만 전달하는 것을 권장한다.
	void Build(const TArray<UPrimitiveComponent*>& Components);

	// 변경된 컴포넌트의 Bounds와 부모 Bounds를 갱신한다.
	// 트리의 분할 구조 자체는 변경하지 않는다.
	bool UpdateLeafAndRefit(UPrimitiveComponent* Component,	const FAABB& NewBounds);

	// 모든 컴포넌트의 현재 Bounds를 다시 읽어 갱신한다.
	// 매 프레임 호출하는 용도가 아니다.
	bool RefitAll();

	// BVH 탐색과 실제 메시 RayCast를 수행하고 가장 가까운 Hit만 반환한다.
	bool RayCastClosest(const FPickingRay& Ray,	FBVHRayHit& OutHit,	FBVHRayQueryStats* OutStats = nullptr) const;

	bool IsEmpty() const
	{
		return RootNode < 0;
	}

	int32 GetNodeCount() const
	{
		return Nodes.Num();
	}

	int32 GetEntryCount() const
	{
		return Entries.Num();
	}

private:
	// Picking에서는 작은 Leaf가 가지치기에 유리하다.
	static constexpr int32 MaxLeafEntries = 4;

	struct FBVHEntry
	{
		UPrimitiveComponent* Component = nullptr;
		FAABB Bounds;
		FVector Center;
	};

	struct FBVHNode
	{
		FAABB Bounds;

		int32 Parent = -1;
		int32 LeftChild = -1;
		int32 RightChild = -1;

		int32 FirstEntry = 0;
		int32 EntryCount = 0;

		bool IsLeaf() const
		{
			return LeftChild < 0 && RightChild < 0;
		}
	};

	struct FLeafLocation
	{
		int32 EntryIndex = -1;
		int32 LeafNodeIndex = -1;
	};

	static bool IsValidBounds(const FAABB& Bounds);

	static FAABB MergeBounds(const FAABB& A, const FAABB& B);

	static bool IntersectsRay(
		const FPickingRay& Ray,
		const FAABB& Bounds,
		float MaxDistanceWorld,
		float& OutEntryDistanceWorld);

	int32 BuildRecursive(int32 First, int32 Last, int32 Parent);

	FAABB CalculateEntryRangeBounds(int32 First, int32 Count) const;

	void RecomputeLeafBounds(int32 LeafNodeIndex);
	void RefitParents(int32 NodeIndex);

private:
	TArray<FBVHNode> Nodes;
	TArray<FBVHEntry> Entries;

	std::unordered_map<UPrimitiveComponent*, FLeafLocation> LeafLocations;

	int32 RootNode = -1;
};