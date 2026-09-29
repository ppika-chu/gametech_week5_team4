#include "BVH.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

#include "PrimitiveComponent.h"

void FBVH::Clear()
{
	Nodes.Empty();
	Entries.Empty();
	LeafLocations.clear();

	RootNode = -1;
}

void FBVH::Build(const TArray<UPrimitiveComponent*>& Components)
{
	Clear();

	Entries.Reserve(Components.Num());
	LeafLocations.reserve(Components.Num());

	std::unordered_set<UPrimitiveComponent*> SeenComponents;
	SeenComponents.reserve(Components.Num());

	for (UPrimitiveComponent* Component : Components)
	{
		if (!Component)
		{
			continue;
		}

		// 같은 컴포넌트가 중복 등록되면
		// LeafLocations의 위치 정보가 모호해진다.
		if (!SeenComponents.insert(Component).second)
		{
			continue;
		}

		const FAABB Bounds = Component->GetBoundingBox();
		if (!IsValidBounds(Bounds))
		{
			continue;
		}

		FBVHEntry Entry;
		Entry.Component = Component;
		Entry.Bounds = Bounds;
		Entry.Center = (Bounds.Min + Bounds.Max) * 0.5f;

		Entries.Add(Entry);
	}

	if (Entries.IsEmpty())
	{
		return;
	}

	Nodes.Reserve(Entries.Num() * 2);

	RootNode = BuildRecursive(0, Entries.Num(), -1);
}

bool FBVH::UpdateLeafAndRefit(UPrimitiveComponent* Component, const FAABB& NewBounds)
{
	if (!Component || !IsValidBounds(NewBounds))
	{
		return false;
	}

	const auto Iterator = LeafLocations.find(Component);
	if (Iterator == LeafLocations.end())
	{
		return false;
	}

	const FLeafLocation& Location = Iterator->second;

	if (Location.EntryIndex < 0 ||
		Location.EntryIndex >= Entries.Num() ||
		Location.LeafNodeIndex < 0 ||
		Location.LeafNodeIndex >= Nodes.Num())
	{
		return false;
	}

	FBVHEntry& Entry = Entries[Location.EntryIndex];
	Entry.Bounds = NewBounds;
	Entry.Center = (NewBounds.Min + NewBounds.Max) * 0.5f;

	RecomputeLeafBounds(Location.LeafNodeIndex);

	RefitParents(Nodes[Location.LeafNodeIndex].Parent);

	return true;
}

bool FBVH::RefitAll()
{
	if (RootNode < 0)
	{
		return true;
	}

	bool bAllBoundsValid = true;

	for (FBVHEntry& Entry : Entries)
	{
		if (!Entry.Component)
		{
			bAllBoundsValid = false;
			continue;
		}

		const FAABB NewBounds =	Entry.Component->GetBoundingBox();

		if (!IsValidBounds(NewBounds))
		{
			bAllBoundsValid = false;
			continue;
		}

		Entry.Bounds = NewBounds;
		Entry.Center = (NewBounds.Min + NewBounds.Max) * 0.5f;
	}

	// BuildRecursive는 부모 노드를 자식보다 먼저 추가한다.
	// 역순으로 순회하면 항상 자식 Bounds가 먼저 갱신된다.
	for (int32 NodeIndex = Nodes.Num() - 1;	NodeIndex >= 0;	--NodeIndex)
	{
		FBVHNode& Node = Nodes[NodeIndex];

		if (Node.IsLeaf())
		{
			RecomputeLeafBounds(NodeIndex);
			continue;
		}

		Node.Bounds = MergeBounds(Nodes[Node.LeftChild].Bounds,	Nodes[Node.RightChild].Bounds);
	}

	return bAllBoundsValid;
}

bool FBVH::RayCastClosest(
	const FPickingRay& Ray,
	FBVHRayHit& OutHit,
	FBVHRayQueryStats* OutStats) const
{
	OutHit = {};

	if (OutStats)
	{
		*OutStats = {};
	}

	if (RootNode < 0 || Ray.Length <= SMALL_NUMBER)
	{
		return false;
	}

	struct FStackItem
	{
		int32 NodeIndex = -1;
		float EntryDistanceWorld = 0.0f;
	};

	float RootEntryDistanceWorld = 0.0f;

	if (!IntersectsRay(
		Ray,
		Nodes[RootNode].Bounds,
		Ray.Length,
		RootEntryDistanceWorld))
	{
		return false;
	}

	TArray<FStackItem> Stack;
	Stack.Reserve(64);
	Stack.Add({ RootNode, RootEntryDistanceWorld });

	float ClosestHitT = FLT_MAX;
	float ClosestDistanceWorld = Ray.Length;

	while (!Stack.IsEmpty())
	{
		const FStackItem StackItem = Stack.Last();
		Stack.RemoveLast();

		if (StackItem.EntryDistanceWorld >
			ClosestDistanceWorld)
		{
			if (OutStats)
			{
				++OutStats->PrunedNodes;
			}
			continue;
		}

		if (OutStats)
		{
			++OutStats->TestedNodes;
		}

		const FBVHNode& Node = Nodes[StackItem.NodeIndex];

		if (Node.IsLeaf())
		{
			const int32 LastEntry =	Node.FirstEntry + Node.EntryCount;

			for (int32 EntryIndex = Node.FirstEntry; EntryIndex < LastEntry; ++EntryIndex)
			{
				if (OutStats)
				{
					++OutStats->TestedEntries;
				}

				const FBVHEntry& Entry = Entries[EntryIndex];

				if (!Entry.Component)
				{
					continue;
				}

				float EntryDistanceWorld = 0.0f;

				if (!IntersectsRay(
					Ray,
					Entry.Bounds,
					ClosestDistanceWorld,
					EntryDistanceWorld))
				{
					continue;
				}

				// 이 AABB에 들어가는 지점이 이미 찾은 실제
				// 메시 Hit보다 멀면 Narrow Phase가 필요 없다.
				if (EntryDistanceWorld > ClosestDistanceWorld)
				{
					continue;
				}

				if (OutStats)
				{
					++OutStats->NarrowPhaseTests;
				}

				float HitT = FLT_MAX;

				if (!Entry.Component->RayCastComponent(Ray, HitT))
				{
					continue;
				}

				// 현재 RayCastComponent의 OutHitT는
				// Near→Far 선분의 매개변수로 사용한다.
				if (HitT < 0.0f || HitT > 1.0f)
				{
					continue;
				}

				if (HitT >= ClosestHitT)
				{
					continue;
				}

				ClosestHitT = HitT;
				ClosestDistanceWorld =	HitT * Ray.Length;

				OutHit.Component = Entry.Component;
				OutHit.HitT = HitT;
				OutHit.HitDistanceWorld = ClosestDistanceWorld;
			}

			continue;
		}

		float LeftEntryDistanceWorld = 0.0f;
		float RightEntryDistanceWorld = 0.0f;

		const bool bHitLeft = IntersectsRay(
			Ray,
			Nodes[Node.LeftChild].Bounds,
			ClosestDistanceWorld,
			LeftEntryDistanceWorld);

		const bool bHitRight = IntersectsRay(
			Ray,
			Nodes[Node.RightChild].Bounds,
			ClosestDistanceWorld,
			RightEntryDistanceWorld);

		// Stack은 LIFO다.
		// 먼 노드를 먼저 Push해야 가까운 노드가 먼저 처리된다.
		if (bHitLeft && bHitRight)
		{
			if (LeftEntryDistanceWorld <= RightEntryDistanceWorld)
			{
				Stack.Add({	Node.RightChild, RightEntryDistanceWorld });

				Stack.Add({	Node.LeftChild,	LeftEntryDistanceWorld });
			}
			else
			{
				Stack.Add({	Node.LeftChild,	LeftEntryDistanceWorld});

				Stack.Add({	Node.RightChild, RightEntryDistanceWorld });
			}
		}
		else if (bHitLeft)
		{
			Stack.Add({	Node.LeftChild,	LeftEntryDistanceWorld });
		}
		else if (bHitRight)
		{
			Stack.Add({	Node.RightChild, RightEntryDistanceWorld });
		}
	}

	return OutHit.Component != nullptr;
}

bool FBVH::IsValidBounds(const FAABB& Bounds)
{
	const bool bFinite =
		std::isfinite(Bounds.Min.x) &&
		std::isfinite(Bounds.Min.y) &&
		std::isfinite(Bounds.Min.z) &&
		std::isfinite(Bounds.Max.x) &&
		std::isfinite(Bounds.Max.y) &&
		std::isfinite(Bounds.Max.z);

	if (!bFinite)
	{
		return false;
	}

	return Bounds.Min.x <= Bounds.Max.x &&
		Bounds.Min.y <= Bounds.Max.y &&
		Bounds.Min.z <= Bounds.Max.z;
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
	float MaxDistanceWorld,
	float& OutEntryDistanceWorld)
{
	float Enter = 0.0f;
	float Exit = MaxDistanceWorld;

	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		const float Direction = Ray.Direction[Axis];

		if (std::fabs(Direction) <= SMALL_NUMBER)
		{
			if (Ray.Near[Axis] < Bounds.Min[Axis] ||
				Ray.Near[Axis] > Bounds.Max[Axis])
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

	OutEntryDistanceWorld = Enter;
	return true;
}

int32 FBVH::BuildRecursive(int32 First, int32 Last, int32 Parent)
{
	const int32 NodeIndex = Nodes.Num();

	Nodes.Emplace();

	Nodes[NodeIndex].Parent = Parent;
	Nodes[NodeIndex].FirstEntry = First;
	Nodes[NodeIndex].EntryCount = Last - First;

	const int32 EntryCount = Last - First;

	if (EntryCount <= MaxLeafEntries)
	{
		FBVHNode& LeafNode = Nodes[NodeIndex];

		LeafNode.Bounds = CalculateEntryRangeBounds(First, EntryCount);

		for (int32 EntryIndex = First; EntryIndex < Last; ++EntryIndex)
		{
			LeafLocations[
				Entries[EntryIndex].Component] =
				{
					EntryIndex,
					NodeIndex
				};
		}

		return NodeIndex;
	}

	FVector CenterMin = Entries[First].Center;
	FVector CenterMax = CenterMin;

	for (int32 EntryIndex = First + 1; EntryIndex < Last; ++EntryIndex)
	{
		const FVector& Center =	Entries[EntryIndex].Center;

		CenterMin.x = FMath::Min(CenterMin.x, Center.x);
		CenterMin.y = FMath::Min(CenterMin.y, Center.y);
		CenterMin.z = FMath::Min(CenterMin.z, Center.z);

		CenterMax.x = FMath::Max(CenterMax.x, Center.x);
		CenterMax.y = FMath::Max(CenterMax.y, Center.y);
		CenterMax.z = FMath::Max(CenterMax.z, Center.z);
	}

	const FVector CenterExtent = CenterMax - CenterMin;

	int32 SplitAxis = 0;

	if (CenterExtent.y > CenterExtent.x)
	{
		SplitAxis = 1;
	}

	if (CenterExtent.z > CenterExtent[SplitAxis])
	{
		SplitAxis = 2;
	}

	const int32 Middle = First + EntryCount / 2;

	std::nth_element(
		Entries.begin() + First,
		Entries.begin() + Middle,
		Entries.begin() + Last,
		[SplitAxis](
			const FBVHEntry& A,
			const FBVHEntry& B)
		{
			return A.Center[SplitAxis] < B.Center[SplitAxis];
		});

	const int32 LeftChild =	BuildRecursive(First, Middle, NodeIndex);

	const int32 RightChild = BuildRecursive(Middle,	Last, NodeIndex);

	FBVHNode& Node = Nodes[NodeIndex];

	Node.LeftChild = LeftChild;
	Node.RightChild = RightChild;

	Node.Bounds = MergeBounds(Nodes[LeftChild].Bounds, Nodes[RightChild].Bounds);

	return NodeIndex;
}

FAABB FBVH::CalculateEntryRangeBounds(int32 First, int32 Count) const
{
	FAABB Result = Entries[First].Bounds;

	const int32 Last = First + Count;

	for (int32 EntryIndex = First + 1; EntryIndex < Last; ++EntryIndex)
	{
		Result = MergeBounds(Result, Entries[EntryIndex].Bounds);
	}

	return Result;
}

void FBVH::RecomputeLeafBounds(int32 LeafNodeIndex)
{
	FBVHNode& LeafNode = Nodes[LeafNodeIndex];

	if (!LeafNode.IsLeaf() ||
		LeafNode.EntryCount <= 0)
	{
		return;
	}

	LeafNode.Bounds = CalculateEntryRangeBounds(LeafNode.FirstEntry, LeafNode.EntryCount);
}

void FBVH::RefitParents(int32 NodeIndex)
{
	while (NodeIndex >= 0)
	{
		FBVHNode& Node = Nodes[NodeIndex];

		Node.Bounds = MergeBounds(Nodes[Node.LeftChild].Bounds, Nodes[Node.RightChild].Bounds);

		NodeIndex = Node.Parent;
	}
}