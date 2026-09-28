#pragma once

#include "FAABB.h"
#include "Vector.h"
#include "TArray.h"
#include "Actor.h"
#include "FFrustum.h"
#include <algorithm>


class UStaticMeshComponent;
struct FPickingRay;

struct FBVHPrimitive {
	UStaticMeshComponent* StaticMeshComponent = nullptr;
	FAABB WorldAABB;
	FVector Centroid;
};

struct FBVHNode
{
	FAABB Bounds; // 이 노드 아래 전체를 감싸는 AABB

	int32 LeftChild = -1; // Nodes 배열의 인덱스 (-1 = 없음)
	int32 RightChild = -1;

	int32 FirstPrimitive = 0; // Primitives 배열에서의 시작 위치
	int32 PrimitiveCount = 0; // 0이면 내부 노드

	int32 SubtreePrimitiveCount = 0; // 서브트리 전체 프리미티브 수

	bool IsLeaf() const
	{ 
		return (PrimitiveCount > 0); 
	}
};

class FBVH
{
public:
	static constexpr int32 MaxPrimitivesPerLeaf = 4;
	static constexpr int32 MaxDepth = 64;

	FBVH() = default;

	// 액터 추가/삭제 시 호출 → 다음 Update에서 Rebuild
	void	MarkDirty();
	bool	IsDirty() const;

	void	Clear();

	bool	IsEmpty() const;
	int32	GetRootIndex() const;

	const	TArray<FBVHNode>& GetNodes() const;
	const	TArray<FBVHPrimitive>& GetPrimitives() const;

	void	Build(const TArray<AActor*>& Actors);
	void	Refit();
	void	QueryFrustum(const FFrustum& Frustum, TArray<UStaticMeshComponent*>& OutVisible, uint32& OutCulledCount) const;
	UStaticMeshComponent*	QueryRay(const FPickingRay& PickingRay, float& OutHitT) const;



private:
	TArray<FBVHNode>		Nodes;
	TArray<FBVHPrimitive>	Primitives;
	bool					bDirty = true;

	void	CollectPrimitives(const TArray<AActor*>& Actors);
	int32	BuildNode(int32 Begin, int32 End, int32 Depth);

};