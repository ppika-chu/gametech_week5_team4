#include "FMeshBVH.h"
#include <algorithm>
#include "EngineMathLibrary.h"

void FMeshBVH::Clear()
{
    Nodes.Empty();
    Triangles.Empty();
    RootIndex = -1;
}

void FMeshBVH::Build(const TArray<FVertex>& Vertices, const TArray<uint32>& Indices)
{
    Clear();

    const int32 TriCount = Indices.Num() / 3;
    if (TriCount == 0) return;

    TArray<FTriEntry> Entries;

    Entries.Reserve(TriCount);
    
    for (int32 TriIndex = 0; TriIndex < TriCount; ++TriIndex)
    {
        const FVector& V0 = Vertices[Indices[TriIndex*3+0]].GetPosition();
        const FVector& V1 = Vertices[Indices[TriIndex*3+1]].GetPosition();
        const FVector& V2 = Vertices[Indices[TriIndex*3+2]].GetPosition();
        
        FTriEntry Entry;
        Entry.Bounds = FAABB(V0, V0);
        Entry.Bounds.ExpandToInclude(V1);
        Entry.Bounds.ExpandToInclude(V2);

        Entry.Tri.V0 = V0;
        Entry.Tri.E1 = V1-V0;
        Entry.Tri.E2 = V2-V0;
        Entry.Tri.OriginalTriIndex = static_cast<uint32>(TriIndex);

        Entries.Add(Entry);
    }

    Nodes.Reserve(TriCount *2);
    Triangles.Reserve(TriCount);
    RootIndex = BuildRecursive(Entries, 0, Entries.Num());
}

bool FMeshBVH::RayCast(const FVector& LocalOrigin, const FVector& LocalDir, float& InOutMaxT) const
{
    if (RootIndex < 0) return false;
    
    const FRaySIMD LocalRay(FRay(LocalOrigin, LocalDir));
    bool bHit = false;

    // 자식을 넣을 때 구한 진입 거리(Enter)를 같이 넣어서, 꺼낼 때 같은 박스를 다시 검사하지 않는다.
    struct FStackEntry
    {
        int32 NodeIndex;
        float Enter;
    };

    // 루트만 여기서 한 번 검사한다. 나머지는 부모가 넣을 때 검사한다.
    float RootEnter;
    if (!RayIntersectsAABB(LocalRay, InOutMaxT, Nodes[RootIndex].Bounds, RootEnter)) return false;

    // 균형 트리라 깊이가 log2(삼각형 수 / 4) 정도이고, 맞은 자식만 넣으므로 64칸이면 충분하다.
    FStackEntry Stack[MaxStackDepth];
    int32 StackSize = 0;
    Stack[StackSize++] = { RootIndex, RootEnter };

    while (StackSize > 0)
    {
        const FStackEntry Entry = Stack[--StackSize];

        // 넣은 뒤에 더 가까운 삼각형을 찾았으면(InOutMaxT 감소) 이 노드는 그보다 뒤라서 볼 필요가 없다.
        // 넣을 때 박스 자체는 통과했으므로, 달라질 수 있는 조건은 이것뿐이다.
        if (Entry.Enter > InOutMaxT) continue;

        const FMeshBVHNode& Node = Nodes[Entry.NodeIndex];

        if (Node.Left < 0)
        {
            for (int32 i = 0; i < Node.TriCount; ++i)
            {
                const FPreComputedTri& Tri = Triangles[Node.FirstTri + i];
                const FVector P = FVector::cross(LocalDir, Tri.E2);
                const float Det = FVector::dot(Tri.E1, P);
                if (fabsf(Det) < 1e-8f) continue;   // 평행하므로 hit 검사 pass

                const FVector T = LocalOrigin - Tri.V0;
                const float InvDet = 1.0f / Det;
                const float U = FVector::dot(T, P) * InvDet; 

                if (U < 0.0f || U > 1.0f) continue;

                const FVector Q = FVector::cross(T, Tri.E1);
                const float V = FVector::dot(LocalDir, Q) * InvDet;
                if (V < 0.0f || U+V > 1.0f) continue;

                const float HitT = FVector::dot(Tri.E2, Q) * InvDet;
                if (HitT > 1e-8f && HitT < InOutMaxT)
                {
                    InOutMaxT = HitT;
                    bHit = true;
                }
            }
        }
        else
        {
            // 자식 박스에 레이가 들어가는 거리를 구한다 (InOutMaxT보다 멀면 miss)
            float LeftEnter, RightEnter;
            const bool bHitLeft  = RayIntersectsAABB(LocalRay, InOutMaxT, Nodes[Node.Left].Bounds,  LeftEnter);
            const bool bHitRight = RayIntersectsAABB(LocalRay, InOutMaxT, Nodes[Node.Right].Bounds, RightEnter);

            // 맞은 자식만 넣는다. 레이가 먼저 들어가는 쪽을 나중에 넣어서 먼저 꺼내지게 한다.
            if (bHitLeft && bHitRight)
            {
                if (LeftEnter < RightEnter) { Stack[StackSize++] = { Node.Right, RightEnter }; Stack[StackSize++] = { Node.Left, LeftEnter }; }
                else                        { Stack[StackSize++] = { Node.Left, LeftEnter };   Stack[StackSize++] = { Node.Right, RightEnter }; }
            }
            else if (bHitLeft)  { Stack[StackSize++] = { Node.Left, LeftEnter }; }
            else if (bHitRight) { Stack[StackSize++] = { Node.Right, RightEnter }; }
        }
    }
    return bHit;
}

int32 FMeshBVH::BuildRecursive(TArray<FTriEntry>& Entries, int32 Begin, int32 End)
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
        FMeshBVHNode Node;
        Node.Bounds = Bounds;
        Node.FirstTri = Triangles.Num();
        Node.TriCount = Count;

        for (int32 i = Begin; i< End; ++i)
        {
            Triangles.Add(Entries[i].Tri);
        }

        const int32 NodeIndex = Nodes.Num();
        Nodes.Add(Node);

        return NodeIndex;
    }

    // 가장 긴 축 가져오기
    const int32 Mid = PartitionByLongestAxis(Entries, Begin, End);

    // Node 채우고 추가하기
    const int32 NodeIndex = Nodes.Num();
    Nodes.Add(FMeshBVHNode{}); 

    const int32 LeftIndex = BuildRecursive(Entries, Begin, Mid);
    const int32 RightIndex = BuildRecursive(Entries, Mid, End);

    Nodes[NodeIndex].Bounds = Bounds;
    Nodes[NodeIndex].Left = LeftIndex;
    Nodes[NodeIndex].Right = RightIndex;
    
    return NodeIndex;
}

// FBVH와 완전히 같은 로직
int32 FMeshBVH::PartitionByLongestAxis(TArray<FTriEntry>& Entries, int32 Begin, int32 End)
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
        [Axis](const FTriEntry& A, const FTriEntry& B)
        {
            const float CenterA = ((A.Bounds.Min + A.Bounds.Max) * 0.5f)[Axis];
            const float CenterB = ((B.Bounds.Min + B.Bounds.Max) * 0.5f)[Axis];
            return CenterA < CenterB;
        });
    
    return Mid;
}

