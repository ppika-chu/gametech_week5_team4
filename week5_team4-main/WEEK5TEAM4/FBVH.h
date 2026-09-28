#pragma once
#include "PrimitiveComponent.h"
#include "TMap.h"
#include <functional>

struct FFrustum;

class FBVH
{
public:
    // 트리 새로 짓는 메소드
    void Build(const TArray<UPrimitiveComponent*>& Items);
    void Clear();

    // Frustum culling 용
    void QueryFrustum(const FFrustum& Frustum, const std::function<void(UPrimitiveComponent*)>& Visitor) const;

    // Picking 용 (Ray와 가장 가까운 Component 반환)
    UPrimitiveComponent* QueryNearestHit(const FPickingRay& Ray, uint64* OutTestCount) const;

    // 객체 삭제했을 때
    void Remove(UPrimitiveComponent* Item);
    
    // 객체 스폰했을 때
    void Insert(UPrimitiveComponent* Item);

    // 기즈모로 이동했을 때 (Remove + Insert) 
    void Move(UPrimitiveComponent* Item);

private:
    struct FEntry
    {
        UPrimitiveComponent* Component = nullptr;
        FAABB Bounds;
    };

    struct FNode
    {
        FAABB Bounds;
        int32 Left = -1;
        int32 Right = -1;
        int32 Parent = -1;
        TArray<UPrimitiveComponent*> Items;
    };

    int32 BuildRecursive(TArray<FEntry>& Entries, int32 Begin, int32 End, int32 ParentIndex);
    
    void RefitUpward(int32 NodeIndex);
    void RecomputeNodeBounds(int32 NodeIndex);
    int32 FindBestLeaf(const FAABB& NewBounds) const;

    int32 PartitionByLongestAxis(TArray<FEntry>& Entries, int32 Begin, int32 End);
    void SplitLeaf(int32 LeafIndex);

    // TODO : 최적의 leaf size인지는 모름. Test 필요
    static constexpr int32 LeafSize = 4;
    TArray<FNode> Nodes;
    TMap<UPrimitiveComponent*, int32> ComponentToLeaf;  // <Component-Leaf Node Idx>
    int32 RootIndex = -1;
};
