#pragma once
#include "FAABB.h"
#include "TArray.h"
#include "FMeshDescription.h"

struct FMeshBVHNode
{
    FAABB Bounds;
    int32 Left = -1;
    int32 Right = -1;
    int32 FirstTri = -1;
    int32 TriCount = -1;
};

// 뮐러 트럼보어용 V0, E1, E2
struct FPreComputedTri
{
    FVector V0, E1, E2;
    uint32 OriginalTriIndex;    // 원본 인덱스 추적용
};

class FMeshBVH
{
public:
    void Build(const TArray<FVertex>& Vertices, const TArray<uint32>& Indices);
    void Clear();
    bool IsValid() const { return RootIndex >= 0; }
    bool RayCast(const FVector& LocalOrigin, const FVector& LocalDir, float& InOutMaxT) const;
private:
    TArray<FMeshBVHNode> Nodes;
    TArray<FPreComputedTri> Triangles;
    int32 RootIndex = -1;

    struct FTriEntry
    {
        FAABB Bounds;
        FPreComputedTri Tri;
    };

    int32 BuildRecursive(TArray<FTriEntry>& Entries, int32 Begin, int32 End);
    int32 PartitionByLongestAxis(TArray<FTriEntry>& Entries, int32 Begin, int32 End);

    static constexpr int32 LeafSize = 4;
    static constexpr int32 MaxStackDepth = 64;
};