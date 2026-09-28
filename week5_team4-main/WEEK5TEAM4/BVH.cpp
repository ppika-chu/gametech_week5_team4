#include "BVH.h"

#include <algorithm>
#include <cmath>

#include "PrimitiveComponent.h"

void FBVH::Clear()
{
	Nodes.Empty();
	BuildItems.Empty();
	LeafIndices.clear();
	RootNode = -1;
}

void FBVH::Build(const TArray<UPrimitiveComponent*>& Components)
{
	Clear();

	BuildItems.Reserve(Components.Num());
	Nodes.Reserve(Components.Num() * 2);
	LeafIndices.reserve(Components.Num());

	for (UPrimitiveComponent* Component : Components)
	{
		if (!Component)
		{
			continue;
		}

		const FAABB Bounds = Component->GetBoundingBox();

		FBuildItem Item;
		Item.Component = Component;
		Item.Bounds = Bounds;
		Item.Center = (Bounds.Min + Bounds.Max) * 0.5f;
		BuildItems.Add(Item);
	}

	if (BuildItems.IsEmpty())
	{
		return;
	}

	RootNode = BuildRecursive(0, BuildItems.Num(), -1);
	BuildItems.Empty();
}

bool FBVH::SetLeafBounds(UPrimitiveComponent* Component, const FAABB& Bounds)
{
	const auto It = LeafIndices.find(Component);
	if (It == LeafIndices.end())
	{
		return false;
	}

	Nodes[It->second].Bounds = Bounds;
	return true;
}

bool FBVH::UpdateLeafAndRefit(UPrimitiveComponent* Component, const FAABB& Bounds)
{
	const auto It = LeafIndices.find(Component);
	if (It == LeafIndices.end())
	{
		return false;
	}

	const int32 LeafIndex = It->second;
	Nodes[LeafIndex].Bounds = Bounds;

	int32 NodeIndex = Nodes[LeafIndex].Parent;
	while (NodeIndex >= 0)
	{
		FBVHNode& Node = Nodes[NodeIndex];
		Node.Bounds = MergeBounds(
			Nodes[Node.LeftChild].Bounds,
			Nodes[Node.RightChild].Bounds);
		NodeIndex = Node.Parent;
	}

	return true;
}

void FBVH::RefitAll()
{
	for (int32 Index = Nodes.Num() - 1; Index >= 0; --Index)
	{
		FBVHNode& Node = Nodes[Index];
		if (Node.IsLeaf())
		{
			continue;
		}

		Node.Bounds = MergeBounds(
			Nodes[Node.LeftChild].Bounds,
			Nodes[Node.RightChild].Bounds);
	}
}

void FBVH::QueryRay(const FPickingRay& Ray, TArray<FBVHRayHit>& OutHits) const
{
	OutHits.Empty();
	if (RootNode < 0 || Ray.Length <= 0.0f)
	{
		return;
	}

	struct FStackItem
	{
		int32 NodeIndex = -1;
		float EntryDistance = 0.0f;
	};

	float RootEntry = 0.0f;
	if (!IntersectsRay(Ray, Nodes[RootNode].Bounds, Ray.Length, RootEntry))
	{
		return;
	}

	TArray<FStackItem> Stack;
	Stack.Reserve(64);
	Stack.Add({ RootNode, RootEntry });

	while (!Stack.IsEmpty())
	{
		const FStackItem Item = Stack.Last();
		Stack.RemoveLast();

		const FBVHNode& Node = Nodes[Item.NodeIndex];
		if (Node.IsLeaf())
		{
			OutHits.Add({ Node.Component, Item.EntryDistance });
			continue;
		}

		float LeftEntry = 0.0f;
		float RightEntry = 0.0f;
		const bool bHitLeft = IntersectsRay(
			Ray, Nodes[Node.LeftChild].Bounds, Ray.Length, LeftEntry);
		const bool bHitRight = IntersectsRay(
			Ray, Nodes[Node.RightChild].Bounds, Ray.Length, RightEntry);

		// Stack은 LIFO이므로 먼 자식을 먼저 넣어 가까운 자식을 먼저 방문한다.
		if (bHitLeft && bHitRight)
		{
			if (LeftEntry <= RightEntry)
			{
				Stack.Add({ Node.RightChild, RightEntry });
				Stack.Add({ Node.LeftChild, LeftEntry });
			}
			else
			{
				Stack.Add({ Node.LeftChild, LeftEntry });
				Stack.Add({ Node.RightChild, RightEntry });
			}
		}
		else if (bHitLeft)
		{
			Stack.Add({ Node.LeftChild, LeftEntry });
		}
		else if (bHitRight)
		{
			Stack.Add({ Node.RightChild, RightEntry });
		}
	}

	std::sort(OutHits.begin(), OutHits.end(),
		[](const FBVHRayHit& A, const FBVHRayHit& B)
		{
			return A.EntryDistance < B.EntryDistance;
		});
}

FAABB FBVH::MergeBounds(const FAABB& A, const FAABB& B)
{
	return FAABB(
		FVector(
			FMath::Min(A.Min.x, B.Min.x),
			FMath::Min(A.Min.y, B.Min.y),
			FMath::Min(A.Min.z, B.Min.z)),
		FVector(
			FMath::Max(A.Max.x, B.Max.x),
			FMath::Max(A.Max.y, B.Max.y),
			FMath::Max(A.Max.z, B.Max.z)));
}

bool FBVH::IntersectsRay(
	const FPickingRay& Ray,
	const FAABB& Bounds,
	float MaxDistance,
	float& OutEntryDistance)
{
	float Enter = 0.0f;
	float Exit = MaxDistance;

	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		const float Direction = Ray.Direction[Axis];
		if (std::fabs(Direction) <= SMALL_NUMBER)
		{
			if (Ray.Near[Axis] < Bounds.Min[Axis] || Ray.Near[Axis] > Bounds.Max[Axis])
			{
				return false;
			}
			continue;
		}

		float AxisEnter = (Bounds.Min[Axis] - Ray.Near[Axis]) / Direction;
		float AxisExit = (Bounds.Max[Axis] - Ray.Near[Axis]) / Direction;
		if (AxisEnter > AxisExit)
		{
			std::swap(AxisEnter, AxisExit);
		}

		Enter = FMath::Max(Enter, AxisEnter);
		Exit = FMath::Min(Exit, AxisExit);
		if (Enter > Exit)
		{
			return false;
		}
	}

	OutEntryDistance = Enter;
	return true;
}

int32 FBVH::BuildRecursive(int32 First, int32 Last, int32 Parent)
{
	const int32 NodeIndex = Nodes.Num();
	Nodes.Emplace();
	Nodes[NodeIndex].Parent = Parent;

	const int32 Count = Last - First;
	if (Count == 1)
	{
		const FBuildItem& Item = BuildItems[First];
		FBVHNode& Node = Nodes[NodeIndex];
		Node.Bounds = Item.Bounds;
		Node.Component = Item.Component;
		LeafIndices[Item.Component] = NodeIndex;
		return NodeIndex;
	}

	FVector CenterMin = BuildItems[First].Center;
	FVector CenterMax = CenterMin;
	for (int32 Index = First + 1; Index < Last; ++Index)
	{
		const FVector& Center = BuildItems[Index].Center;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			CenterMin[Axis] = FMath::Min(CenterMin[Axis], Center[Axis]);
			CenterMax[Axis] = FMath::Max(CenterMax[Axis], Center[Axis]);
		}
	}

	const FVector Size = CenterMax - CenterMin;
	int32 Axis = 0;
	if (Size.y > Size.x)
	{
		Axis = 1;
	}
	if (Size.z > Size[Axis])
	{
		Axis = 2;
	}

	const int32 Middle = First + Count / 2;
	std::nth_element(
		BuildItems.begin() + First,
		BuildItems.begin() + Middle,
		BuildItems.begin() + Last,
		[Axis](const FBuildItem& A, const FBuildItem& B)
		{
			return A.Center[Axis] < B.Center[Axis];
		});

	const int32 LeftChild = BuildRecursive(First, Middle, NodeIndex);
	const int32 RightChild = BuildRecursive(Middle, Last, NodeIndex);

	FBVHNode& Node = Nodes[NodeIndex];
	Node.LeftChild = LeftChild;
	Node.RightChild = RightChild;
	Node.Bounds = MergeBounds(Nodes[LeftChild].Bounds, Nodes[RightChild].Bounds);
	return NodeIndex;
}
