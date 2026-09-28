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

// Frustum culling 용
void FBVH::QueryFrustum(const FFrustum& Frustum, const std::function<void(UPrimitiveComponent*)>& Visitor) const
{
    if (RootIndex < 0) return;

    TArray<int32> Stack;
    Stack.Add(RootIndex);

    while (Stack.Num() > 0)
    {
        const int32 CurrentIndex = Stack.Last();
        Stack.RemoveLast();
        
        const FNode& CurrentNode = Nodes[CurrentIndex];
        
        // Frustum 내에 없으면 그 아래 자식 노드 모두 skip
        if (!Frustum.Intersects(CurrentNode.Bounds))    continue;

        // Leaf node 이면 그 안에 있는 Item들만 검사 대상
        if (CurrentNode.Left < 0)
        {
            for (UPrimitiveComponent* Item : CurrentNode.Items)
            {
                Visitor(Item);
            }
        }
        else
        {
            Stack.Add(CurrentNode.Left);
            Stack.Add(CurrentNode.Right);
        }
    };
}

// Picking 용 (Ray와 가장 가까운 Component 반환)
UPrimitiveComponent* FBVH::QueryNearestHit(const FPickingRay& Ray, uint64* OutTestCount, const FFrustum* Frustum) const
{
    if (RootIndex < 0) return nullptr;
    
    UPrimitiveComponent* NearestComponent = nullptr;
    float NearestT = Ray.Length;
    
    TArray<int32> Stack;
    Stack.Add(RootIndex);

    while (Stack.Num() > 0)
    {
        const int32 CurrentIndex = Stack.Last();
        Stack.RemoveLast();
        const FNode& CurrentNode = Nodes[CurrentIndex];

        if (!RayIntersectsAABB(Ray.ToRay(), NearestT, CurrentNode.Bounds)) continue;
        if (CurrentNode.Left < 0)
        {
            for (UPrimitiveComponent* Item : CurrentNode.Items)
            {
                // 화면에서 안 보이는 건 피킹 후보 제외
                if (Frustum && !Frustum->Intersects(Item->GetBoundingBox()))
                    continue;

                // Picking Test count ++
                if (OutTestCount) ++(*OutTestCount);

                float HitT = FLT_MAX;
                if (Item->RayCastComponent(Ray, HitT) && HitT < NearestT)
                {
                    NearestT = HitT;
                    NearestComponent = Item;
                }
            }
        }
        else
        {
            // TODO : Ray 시작점에 더 가까운 자식을 먼저 스택에 넣는 로직 추가
            Stack.Add(CurrentNode.Left);
            Stack.Add(CurrentNode.Right);
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