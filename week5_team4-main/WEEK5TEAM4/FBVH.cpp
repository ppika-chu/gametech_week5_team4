#include "FBVH.h"
#include "EngineMathLibrary.h"
#include "RayCast.h"
#include "TMap.h"
#include <functional>

void FBVH::Clear()
{
    Nodes.Empty();
    ComponentToLeaf.Reset();
    RootIndex = -1;
}

// 트리 새로 짓는 메소드
void FBVH::Build(const TArray<UPrimitiveComponent*>& Items)
{
    Clear();

    if (Items.Num() == 0)   return;

    // GetBoundingBox 캐싱
    TArray<FEntry> Entries;
    Entries.Reserve(Items.Num());
    for (UPrimitiveComponent* Item : Items)
    {
        FEntry Entry;
        Entry.Component = Item;
        Entry.Bounds = Item->GetBoundingBox();
        Entries.Add(Entry);
    }

    Nodes.Reserve(Entries.Num() * 2);
    RootIndex = BuildRecursive(Entries, 0, Entries.Num(), -1);
}
int32 FBVH::PartitionByLongestAxis(TArray<FEntry>& Entries, int32 Begin, int32 End)
{

    FVector CentroidMin = (Entries[Begin].Bounds.Min + Entries[Begin].Bounds.Max) * 0.5f;
    FVector CentroidMax = CentroidMin;
    for (int32 i = Begin + 1; i < End; ++i)
    {
        const FVector Center = (Entries[i].Bounds.Min + Entries[i].Bounds.Max) * 0.5f;
        CentroidMin.x = FMath::Min(CentroidMin.x, Center.x);
        CentroidMin.y = FMath::Min(CentroidMin.y, Center.y);
        CentroidMin.z = FMath::Min(CentroidMin.z, Center.z);
        CentroidMax.x = FMath::Max(CentroidMax.x, Center.x);
        CentroidMax.y = FMath::Max(CentroidMax.y, Center.y);
        CentroidMax.z = FMath::Max(CentroidMax.z, Center.z);
    }
    
    const FVector CentroidExtent = CentroidMax - CentroidMin;
    
    // 가장 긴 축 고르기
    int32 Axis = 0;
    if (CentroidExtent.y > CentroidExtent[Axis])    Axis = 1;
    if (CentroidExtent.z > CentroidExtent[Axis])    Axis = 2;
    
    const int32 Mid = Begin + (End - Begin) / 2;
    
    // Mid를 기준으로 분할
    std::nth_element(
        Entries.Data() + Begin, Entries.Data() + Mid, Entries.Data() + End,
        [Axis](const FEntry& A, const FEntry& B)
        {
            const float CenterA = ((A.Bounds.Min + A.Bounds.Max) * 0.5f)[Axis];
            const float CenterB = ((B.Bounds.Min + B.Bounds.Max) * 0.5f)[Axis];
            return CenterA < CenterB;
        });
    
    return Mid;
}

// Leaf 하나를 노드로 승격 (insert 할 때 사용)
void FBVH::SplitLeaf(int32 LeafIndex)
{
    TArray<UPrimitiveComponent*> OverflowItems = Nodes[LeafIndex].Items;
    TArray<FEntry> Entries;
    Entries.Reserve(OverflowItems.Num());

    for (UPrimitiveComponent* Comp : OverflowItems)
    {
        FEntry Entry;
        Entry.Bounds = Comp->GetBoundingBox();
        Entry.Component = Comp;
        Entries.Add(Entry);

        ComponentToLeaf.Remove(Comp);
    }

    const int32 Mid = PartitionByLongestAxis(Entries, 0, Entries.Num());
    
    Nodes[LeafIndex].Items.Empty();

    const int32 Left = BuildRecursive(Entries, 0, Mid, LeafIndex);
    const int32 Right = BuildRecursive(Entries, Mid, Entries.Num(), LeafIndex);

    Nodes[LeafIndex].Left = Left;
    Nodes[LeafIndex].Right = Right;

    RefitUpward(LeafIndex);
}


int32 FBVH::BuildRecursive(TArray<FEntry>& Entries, int32 Begin, int32 End, int32 ParentIndex)
{
    FAABB Bounds = Entries[Begin].Bounds;
    for (int32 i = Begin + 1; i < End; ++i)
    {
        Bounds.ExpandToInclude(Entries[i].Bounds.Min);
        Bounds.ExpandToInclude(Entries[i].Bounds.Max);
    }
    
    const int32 Count = End - Begin;

    // Leaf 노드인 경우 (Base Case)
    if (Count <= LeafSize)
    {
        FNode Node;
        Node.Bounds = Bounds;
        Node.Parent = ParentIndex;
        for (int32 i = Begin; i< End; ++i)
        {
            Node.Items.Add(Entries[i].Component);
        }

        const int32 NodeIndex = Nodes.Num();
        Nodes.Add(Node);

        for (int32 i = Begin; i < End; ++i)
        {
            ComponentToLeaf.Add(Entries[i].Component, NodeIndex);
        }

        return NodeIndex;
    }

    // 가장 긴 축 가져오기
    const int32 Mid = PartitionByLongestAxis(Entries, Begin, End);

    // Node 채우고 추가하기
    const int32 NodeIndex = Nodes.Num();
    Nodes.Add(FNode{}); 
    Nodes[NodeIndex].Parent = ParentIndex;

    const int32 LeftIndex = BuildRecursive(Entries, Begin, Mid, NodeIndex);
    const int32 RightIndex = BuildRecursive(Entries, Mid, End, NodeIndex);

    Nodes[NodeIndex].Bounds = Bounds;
    Nodes[NodeIndex].Left = LeftIndex;
    Nodes[NodeIndex].Right = RightIndex;
    
    return NodeIndex;
}

namespace {
    struct FCullStackEntry
    {
        int32  NodeIndex;
        uint32 PlaneMask;
    };
}

// Frustum culling 용
void FBVH::QueryFrustum(const FFrustum& Frustum, const std::function<void(UPrimitiveComponent*)>& Visitor) const
{
    if (RootIndex < 0) return;

    TArray<FCullStackEntry> Stack;
    Stack.Reserve(64);
    Stack.Add({ RootIndex, 0b111111 });   // 루트는 6개 평면 전부 검사

    while (Stack.Num() > 0)
    {
        const FCullStackEntry Entry = Stack.Last();
        Stack.RemoveLast();

        const FNode& CurrentNode = Nodes[Entry.NodeIndex];

        // 부모가 이미 Inside면(마스크 0) 판정을 건너뛴다
        uint32 Mask = Entry.PlaneMask;
        if (Mask != 0 && Frustum.ClassifyAABB(CurrentNode.Bounds, Mask) == FFrustum::EFrustumTestResult::Outside)
        {
            continue;   // 서브트리 전체 버림
        }

        if (CurrentNode.Left < 0)   // 리프
        {
            for (UPrimitiveComponent* Item : CurrentNode.Items)
            {
                // 노드가 Inside면 아이템도 안쪽이 확실하다. 걸쳐 있을 때만 남은 평면으로 판정
                if (Mask != 0)
                {
                    uint32 ItemMask = Mask;
                    if (Frustum.ClassifyAABB(Item->GetBoundingBox(), ItemMask) == FFrustum::EFrustumTestResult::Outside)
                    {
                        continue;
                    }
                }
                Visitor(Item);
            }
        }
        else
        {
            // 자식은 줄어든 마스크를 물려받는다 (Inside면 0이 전달됨)
            Stack.Add({ CurrentNode.Left,  Mask });
            Stack.Add({ CurrentNode.Right, Mask });
        }
    }
}
// Picking 용 (Ray와 가장 가까운 Component 반환)
UPrimitiveComponent* FBVH::QueryNearestHit(const FPickingRay& Ray, uint64* OutTestCount) const
{
    if (RootIndex < 0) return nullptr;
    
    UPrimitiveComponent* NearestComponent = nullptr;
    float NearestT = Ray.Length;
    const FRaySIMD RaySIMD(Ray.ToRay());   // 피킹 한 번에 나눗셈 3번으로 끝

    // 자식을 넣을 때 이미 구한 진입 거리(Enter)를 같이 넣어서,
    // 꺼낼 때 같은 박스를 다시 검사하지 않는다.
    struct FPickStackEntry
    {
        int32 NodeIndex;
        float Enter;
    };

    // 루트만 여기서 한 번 검사한다. 나머지는 부모가 넣을 때 검사한다.
    float RootEnter;
    if (!RayIntersectsAABB(RaySIMD, NearestT, Nodes[RootIndex].Bounds, RootEnter)) return nullptr;

    TArray<FPickStackEntry> Stack;
    Stack.Add({ RootIndex, RootEnter });

    while (Stack.Num() > 0)
    {
        const FPickStackEntry Entry = Stack.Last();
        Stack.RemoveLast();

        // 넣은 뒤에 더 가까운 물체를 찾았으면(NearestT 감소) 이 노드는 그보다 뒤에 있어서 볼 필요가 없다.
        // 넣을 때 박스 자체는 통과했으므로, 달라질 수 있는 조건은 이것뿐이다.
        if (Entry.Enter > NearestT) continue;

        const FNode& CurrentNode = Nodes[Entry.NodeIndex];

        if (CurrentNode.Left < 0)
        {
            for (UPrimitiveComponent* Item : CurrentNode.Items)
            {
                // 레이가 이 아이템의 박스를 안 지나가거나,
                // 지나가더라도 지금까지 찾은 가장 가까운 물체(NearestT)보다 뒤에 있으면 건너뛴다
                float ItemEnter;
                if (!RayIntersectsAABB(RaySIMD, NearestT, Item->GetBoundingBox(), ItemEnter))
                    continue;

                // 프러스텀 검사는 하지 않는다. 피킹 레이(Near~Far)는 전부 프러스텀 안에 있으므로
                // 위에서 Ray-AABB를 통과했으면 프러스텀과도 반드시 겹친다.

                // Picking Test count ++
                if (OutTestCount) ++(*OutTestCount);

                float HitT = FLT_MAX;
                if (Item->RayCastComponent(Ray, NearestT, HitT) && HitT < NearestT)
                {
                    NearestT = HitT;
                    NearestComponent = Item;
                }
            }
        }
        else
        {
            // 두 자식 박스에 레이가 들어가는 거리를 구한다 (NearestT보다 멀면 miss)
            float LeftEnter, RightEnter;
            const bool bHitLeft = RayIntersectsAABB(RaySIMD, NearestT, Nodes[CurrentNode.Left].Bounds, LeftEnter);
            const bool bHitRight = RayIntersectsAABB(RaySIMD, NearestT, Nodes[CurrentNode.Right].Bounds, RightEnter);

            // 가까운 쪽을 나중에 넣어서 먼저 꺼내지게 한다
            if (bHitLeft && bHitRight)
            {
                if (LeftEnter < RightEnter) { Stack.Add({ CurrentNode.Right, RightEnter }); Stack.Add({ CurrentNode.Left, LeftEnter }); }
                else                        { Stack.Add({ CurrentNode.Left, LeftEnter });   Stack.Add({ CurrentNode.Right, RightEnter }); }
            }
            else if (bHitLeft)  { Stack.Add({ CurrentNode.Left, LeftEnter }); }
            else if (bHitRight) { Stack.Add({ CurrentNode.Right, RightEnter }); }
        }
    }
    return NearestComponent;
}


// AABB 다시 계산
void FBVH::RecomputeNodeBounds(int32 NodeIndex)
{
    FNode& Node = Nodes[NodeIndex];

    // Leaf Node 일 경우
    if (Node.Left < 0)
    {
        // Item 없을 경우 pass
        if (Node.Items.Num() == 0) return;

        // Item 있으면 첫번째 Item 기준으로 확장
        FAABB Bounds = Node.Items[0]->GetBoundingBox();
        
        for (int i = 1; i < Node.Items.Num(); i++)
        {
            Bounds.ExpandToInclude(Node.Items[i]->GetBoundingBox().Min);
            Bounds.ExpandToInclude(Node.Items[i]->GetBoundingBox().Max);
        }
        Node.Bounds = Bounds;
    }
    else
    {
        FAABB Bounds = Nodes[Node.Left].Bounds;
        Bounds.ExpandToInclude(Nodes[Node.Right].Bounds.Min);
        Bounds.ExpandToInclude(Nodes[Node.Right].Bounds.Max);
        Node.Bounds = Bounds;
    }
}

// 루트까지 거슬러 올라가며 Bounds 재계산
void FBVH::RefitUpward(int32 NodeIndex)
{
    int32 Current = NodeIndex;

    while (Current >= 0)
    {
        RecomputeNodeBounds(Current);
        Current = Nodes[Current].Parent;
    }
}

// 최적의 Leaf 노드 찾기
int32 FBVH::FindBestLeaf(const FAABB& NewBounds) const
{
    int32 Current = RootIndex;
    
    // Leaf 노드에 닿을 때까지
    while (Nodes[Current].Left >= 0)
    {
        const FAABB& LeftBounds = Nodes[Nodes[Current].Left].Bounds;
        const FAABB& RightBounds = Nodes[Nodes[Current].Right].Bounds;
        
        auto ExpandedVolume = [&NewBounds](const FAABB& Box)
        {
            FAABB Expanded = Box;
            Expanded.ExpandToInclude(NewBounds.Min);
            Expanded.ExpandToInclude(NewBounds.Max);

            const FVector Size = Expanded.Max - Expanded.Min;
            return Size.x * Size.y * Size.z;
        };

        Current = (ExpandedVolume(LeftBounds) <= ExpandedVolume(RightBounds))?
                Nodes[Current].Left : Nodes[Current].Right;
    }
    return Current;
}

// 객체 삭제했을 때
void FBVH::Remove(UPrimitiveComponent* Item)
{
    int32* LeafIdxPtr = ComponentToLeaf.Find(Item);
    if (!LeafIdxPtr) return;    // Leaf에 없음.
    
    int32 LeafIdx = *LeafIdxPtr;
    FNode& Leaf = Nodes[LeafIdx];

    for (int i = 0; i < Leaf.Items.Num(); ++i)
    {
        if (Leaf.Items[i] == Item)
        {
            Leaf.Items.RemoveAtSwap(i);
            break;
        }
    }
    ComponentToLeaf.Remove(Item);
    RefitUpward(LeafIdx);
}

// 객체 스폰했을 때
void FBVH::Insert(UPrimitiveComponent* Item)
{
    FAABB NewBounds = Item->GetBoundingBox();

    if (RootIndex < 0)
    {
        FNode Leaf;
        Leaf.Bounds = NewBounds;
        Leaf.Items.Add(Item);
        
        RootIndex = Nodes.Num();
        Nodes.Add(Leaf);
        ComponentToLeaf.Add(Item, RootIndex);
        return;
    }

    const int32 LeafIdx = FindBestLeaf(NewBounds);
    Nodes[LeafIdx].Items.Add(Item);
    ComponentToLeaf.Add(Item, LeafIdx);

    if (Nodes[LeafIdx].Items.Num() > LeafSize)
    {
        SplitLeaf(LeafIdx);
    }
    else
    {
        RefitUpward(LeafIdx);
    }
}

// 기즈모로 이동했을 때 (Remove + Insert) 
void FBVH::Move(UPrimitiveComponent* Item)
{
    Remove(Item);
    Insert(Item);
}