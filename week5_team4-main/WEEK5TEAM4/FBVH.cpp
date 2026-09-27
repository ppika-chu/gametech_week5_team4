#include "FBVH.h"
#include "EngineMathLibrary.h"
#include "FEditorViewportClient.h"

void FBVH::Clear()
{
    Nodes.Empty();
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
    RootIndex = BuildRecursive(Entries, 0, Entries.Num());
}

int32 FBVH::BuildRecursive(TArray<FEntry>& Entries, int32 Begin, int32 End)
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
        for (int32 i = Begin; i< End; ++i)
        {
            Node.Items.Add(Entries[i].Component);
        }

        const int32 NodeIndex = Nodes.Num();
        Nodes.Add(Node);
        return NodeIndex;
    }

    FVector CentroidMin = (Entries[Begin].Bounds.Min + Entries[Begin].Bounds.Max) * 0.5f;
    FVector CentroidMax = CentroidMin;
    for (int32 i = Begin + 1; i < End; ++i)
    {
        const FVector Center = (Entries[i].Bounds.Min + Entries[i].Bounds.Max) * 0.5f;
        CentroidMin.x = FMath::Min(CentroidMin.x, Center.x);
        CentroidMin.y = FMath::Min(CentroidMin.y, Center.y);
        CentroidMin.z = FMath::Min(CentroidMin.z, Center.z);
        CentroidMax.x = FMath::Max(CentroidMin.x, Center.x);
        CentroidMax.y = FMath::Max(CentroidMin.y, Center.y);
        CentroidMax.z = FMath::Max(CentroidMin.z, Center.z);
    }

    const FVector CentroidExtent = CentroidMax - CentroidMin;

    // 가장 긴 축 고르기
    int32 Axis = 0;
    if (CentroidExtent.y > CentroidExtent[Axis])    Axis = 1;
    if (CentroidExtent.z > CentroidExtent[Axis])    Axis = 2;

    const int32 Mid = Begin + Count / 2;

    // Mid를 기준으로 분할
    std::nth_element(
        Entries.Data() + Begin, Entries.Data() + Mid, Entries.Data() + End,
        [Axis](const FEntry& A, const FEntry& B)
        {
            const float CenterA = ((A.Bounds.Min + A.Bounds.Max) * 0.5f)[Axis];
            const float CenterB = ((B.Bounds.Min + B.Bounds.Max) * 0.5f)[Axis];
            return CenterA < CenterB;
        });
    
    // Node 채우고 추가하기
    const int32 NodeIndex = Nodes.Num();
    Nodes.Add(FNode{}); 

    const int32 LeftIndex = BuildRecursive(Entries, Begin, Mid);
    const int32 RightIndex = BuildRecursive(Entries, Mid, End);

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
UPrimitiveComponent* FBVH::QueryNearestHit(const FPickingRay& Ray, uint64* OutTestCount) const
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