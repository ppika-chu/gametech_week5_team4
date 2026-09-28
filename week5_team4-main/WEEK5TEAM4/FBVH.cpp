#include "FBVH.h"
#include "UStaticMeshComponent.h"
#include "EngineMathLibrary.h"
#include "RayCast.h"

void FBVH::MarkDirty()
{ 
	bDirty = true; 
}

bool FBVH::IsDirty() const 
{ 
	return bDirty; 
}

void FBVH::Clear()
{
	Nodes.Empty();
	Primitives.Empty();
	bDirty = true;
}

bool FBVH::IsEmpty() const 
{ 
	return Nodes.IsEmpty(); 
}

int32 FBVH::GetRootIndex() const 
{ 
	return 0; 
}

const TArray<FBVHNode>& FBVH::GetNodes() const 
{ 
	return Nodes; 
}

const TArray<FBVHPrimitive>& FBVH::GetPrimitives() const 
{
	return Primitives;
}

void	FBVH::Build(const TArray<AActor*>& Actors)
{
	bDirty = false;
	Nodes.Empty();
	Primitives.Empty();

	CollectPrimitives(Actors);
	if (Primitives.IsEmpty())
		return;

	Nodes.Reserve(Primitives.Num() * 2);
	BuildNode(0, Primitives.Num(), 0);
	
}

void	FBVH::Refit()
{
	for (FBVHPrimitive& Primitive : Primitives)
	{
		Primitive.StaticMeshComponent->UpdateWorldCache();
		Primitive.WorldAABB = Primitive.StaticMeshComponent->GetCachedWorldBounds();


		Primitive.Centroid = (Primitive.WorldAABB.Min + Primitive.WorldAABB.Max) * 0.5f;
	}

	for (int32 NodeIndex = Nodes.Num() - 1; NodeIndex >= 0; --NodeIndex)
	{
		FBVHNode& CurrentNode = Nodes[NodeIndex];
		if (CurrentNode.IsLeaf())
		{
			CurrentNode.Bounds = Primitives[CurrentNode.FirstPrimitive].WorldAABB;

			for (int32 Index = 1; Index < CurrentNode.PrimitiveCount; Index++)
			{
				CurrentNode.Bounds.ExpandToInclude(Primitives[CurrentNode.FirstPrimitive + Index].WorldAABB);
			}
		}
		else
		{
			CurrentNode.Bounds = Nodes[CurrentNode.LeftChild].Bounds;
			CurrentNode.Bounds.ExpandToInclude(Nodes[CurrentNode.RightChild].Bounds);
		}
	}
}

// Begin은 포함, End는 포함하지 않는 구간
int32	FBVH::BuildNode(int32 Begin, int32 End, int32 Depth)
{
	FBVHNode BVHNode;

	int32 NodeIndex = Nodes.Num();
	Nodes.Add(BVHNode);

	Nodes[NodeIndex].Bounds = Primitives[Begin].WorldAABB;
	for (int32 i = Begin + 1; i < End; i++)
	{
		Nodes[NodeIndex].Bounds.ExpandToInclude(Primitives[i].WorldAABB);
	}

	Nodes[NodeIndex].SubtreePrimitiveCount = End - Begin;

	int32 Count = End - Begin;
	Nodes[NodeIndex].FirstPrimitive = Begin;
	if (Count <= MaxPrimitivesPerLeaf || Depth >= MaxDepth)
	{
		Nodes[NodeIndex].PrimitiveCount = Count;
		return NodeIndex;
	}

	FVector Centroid = Primitives[Begin].Centroid;
	FAABB CentroidBounds(Centroid, Centroid);
	for (int32 i = Begin + 1; i < End; i++)
	{
		CentroidBounds.ExpandToInclude(Primitives[i].Centroid);
	}

	FVector Size = CentroidBounds.Max - CentroidBounds.Min;

	int32 Axis = 0;
	if (Size.y > Size[Axis])
		Axis = 1;
	if (Size.z > Size[Axis])
		Axis = 2;

	int32 Mid = (Begin + End) / 2;

	std::nth_element(
		Primitives.begin() + Begin,     // 구간 시작
		Primitives.begin() + Mid,       // 가운데 자리
		Primitives.begin() + End,       // 구간 끝
		[Axis](const FBVHPrimitive& A, const FBVHPrimitive& B)
		{   return A.Centroid[Axis] < B.Centroid[Axis];   }); // 비교용 람다

	int32 Left = BuildNode(Begin, Mid, Depth + 1);
	int32 Right = BuildNode(Mid, End, Depth + 1);

	Nodes[NodeIndex].LeftChild = Left;
	Nodes[NodeIndex].RightChild = Right;

	return NodeIndex;
}


void	FBVH::CollectPrimitives(const TArray<AActor*>& Actors)
{
	for (AActor* Actor : Actors)
	{
		for (UActorComponent* ActorComponent : Actor->GetComponents())
		{
			UStaticMeshComponent* StaticMeshComponent = ActorComponent->Cast<UStaticMeshComponent>();
			if (StaticMeshComponent == nullptr || StaticMeshComponent->GetMesh() == nullptr)
				continue;
			FBVHPrimitive BVHPrimitive;
			BVHPrimitive.StaticMeshComponent = StaticMeshComponent;
			StaticMeshComponent->UpdateWorldCache();
			BVHPrimitive.WorldAABB = StaticMeshComponent->GetCachedWorldBounds();

			BVHPrimitive.Centroid = (BVHPrimitive.WorldAABB.Min + BVHPrimitive.WorldAABB.Max) * 0.5f;
			Primitives.Add(BVHPrimitive);
		}
	}
}

struct FCullStackEntry
{
	int32  NodeIndex;
	uint32 PlaneMask;
};

void	FBVH::QueryFrustum(const FFrustum& Frustum, TArray<UStaticMeshComponent*>& OutVisible, uint32& OutCulledCount) const
{
	OutVisible.Empty();
	OutCulledCount = 0;

	if (Nodes.IsEmpty())
		return;

	FCullStackEntry CullStack[MaxDepth * 2];
	uint32 Index = 0;

	CullStack[Index].NodeIndex = 0;
	CullStack[Index].PlaneMask = 0b111111;
	Index++;

	while (Index > 0)
	{
		--Index;
		const FCullStackEntry& CurrentCullStack = CullStack[Index];
		const FBVHNode& CurrentNode = Nodes[CurrentCullStack.NodeIndex];
		

		uint32 NewMask = CurrentCullStack.PlaneMask;
		FFrustum::EFrustumTestResult Result = Frustum.ClassifyAABB(CurrentNode.Bounds, NewMask);

		if (Result == FFrustum::EFrustumTestResult::Outside)
		{
			OutCulledCount += CurrentNode.SubtreePrimitiveCount;
		}
		else if (Result == FFrustum::EFrustumTestResult::Inside)
		{
			for (int32 PrimitiveIndex = CurrentNode.FirstPrimitive;
				PrimitiveIndex < CurrentNode.FirstPrimitive + CurrentNode.SubtreePrimitiveCount; ++PrimitiveIndex)
			{
				OutVisible.Add(Primitives[PrimitiveIndex].StaticMeshComponent);
			}
		}
		else if (Result == FFrustum::EFrustumTestResult::Intersect)
		{
			if (CurrentNode.IsLeaf())
			{
				for (int32 PrimitiveIndex = CurrentNode.FirstPrimitive;
					PrimitiveIndex < CurrentNode.FirstPrimitive + CurrentNode.PrimitiveCount; ++PrimitiveIndex)
				{
					uint32 PrimitiveMask = NewMask;
					if (Frustum.ClassifyAABB(Primitives[PrimitiveIndex].WorldAABB, PrimitiveMask) == FFrustum::EFrustumTestResult::Outside)
						OutCulledCount += 1;
					else
						OutVisible.Add(Primitives[PrimitiveIndex].StaticMeshComponent);
				}
			}
			else
			{
				CullStack[Index].NodeIndex = CurrentNode.LeftChild;
				CullStack[Index].PlaneMask = NewMask;
				Index++;

				CullStack[Index].NodeIndex = CurrentNode.RightChild;
				CullStack[Index].PlaneMask = NewMask;
				Index++;
			}
		}
	}
}

struct FRayStackEntry
{
	int32	NodeIndex;
	float	Enter;
};

UStaticMeshComponent* FBVH::QueryRay(const FPickingRay& PickingRay, float& OutHitT) const
{
	float MinHitT = FLT_MAX;
	float TempHitT = 0;
	float Enter = 0;
	
	if (Nodes.IsEmpty())
		return nullptr;

	FRayStackEntry RayStack[MaxDepth * 2];
	uint32 Index = 0;
	UStaticMeshComponent* ResultStaticMesh = nullptr;

	RayStack[Index].NodeIndex = 0;
	if (!RayIntersectsAABB(PickingRay.ToRay(), PickingRay.Length, Nodes[RayStack[Index].NodeIndex].Bounds, Enter))
	{
		return nullptr;
	}
	RayStack[Index].Enter = Enter / PickingRay.Length;
	Index++;

	while (Index > 0)
	{
		--Index;
		const FRayStackEntry CurrentRayStack = RayStack[Index];
		const FBVHNode& CurrentNode = Nodes[CurrentRayStack.NodeIndex];


		if (CurrentRayStack.Enter >= MinHitT)
			continue;

		if (CurrentNode.IsLeaf())
		{
			for (int32 PrimitiveIndex = CurrentNode.FirstPrimitive;
				PrimitiveIndex < CurrentNode.FirstPrimitive + CurrentNode.PrimitiveCount; ++PrimitiveIndex)
			{


				const FBVHPrimitive& Primitive = Primitives[PrimitiveIndex];

				float PrimitiveEnter = 0.0f;
				if (!RayIntersectsAABB(PickingRay.ToRay(), PickingRay.Length, Primitive.WorldAABB, PrimitiveEnter))
				{
					continue;
				}
				if (PrimitiveEnter / PickingRay.Length >= MinHitT)
				{
					continue;
				}

				if (Primitive.StaticMeshComponent->RayCastComponent(PickingRay, TempHitT) && TempHitT < MinHitT)
				{
					MinHitT = TempHitT;
					ResultStaticMesh = Primitive.StaticMeshComponent;
				}
			}
		}
		else
		{
			FRayStackEntry LeftRayStackEntry;
			FRayStackEntry RightRayStackEntry;

			bool bHitLeft = RayIntersectsAABB(PickingRay.ToRay(), PickingRay.Length, Nodes[CurrentNode.LeftChild].Bounds, Enter);
			if (bHitLeft)
			{
				LeftRayStackEntry.NodeIndex = CurrentNode.LeftChild;
				LeftRayStackEntry.Enter = Enter / PickingRay.Length;
			}
			bool bHitRight = RayIntersectsAABB(PickingRay.ToRay(), PickingRay.Length, Nodes[CurrentNode.RightChild].Bounds, Enter);
			if (bHitRight)
			{
				RightRayStackEntry.NodeIndex = CurrentNode.RightChild;
				RightRayStackEntry.Enter = Enter / PickingRay.Length;
			}

			if (!bHitLeft && bHitRight)
			{
				RayStack[Index++] = RightRayStackEntry;
			}
			else if (bHitLeft && !bHitRight)
			{
				RayStack[Index++] = LeftRayStackEntry;
			}
			else if (bHitLeft && bHitRight)
			{
				if (LeftRayStackEntry.Enter > RightRayStackEntry.Enter)
				{
					RayStack[Index++] = LeftRayStackEntry;
					RayStack[Index++] = RightRayStackEntry;
				}
				else
				{
					RayStack[Index++] = RightRayStackEntry;
					RayStack[Index++] = LeftRayStackEntry;
				}
			}
		}
	}
	OutHitT = MinHitT;
	return (ResultStaticMesh);
}