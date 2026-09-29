#pragma once
#include "Core.h"

enum class EOptFlag : uint8
{
    FrustumCulling = 0,
    BVHCulling,
    LOD,
    TransformCache,
    DrawCallSorting,
    BVHPicking,
    PickingFrustum,
    Count
};

// 위 enum과 순서 동일해야 함.
inline bool GOptEnabled[static_cast<uint8>(EOptFlag::Count)] =
{
    true,   // FrustumCulling
    true,   // BVHCulling
    true,   // LOD
    true,   // TransformCache
    true,   // DrawCallSorting
    true,   // BVHPicking
    true,   // PickingFrustum

};

inline bool IsOptEnabled(EOptFlag Flag)
{
    return GOptEnabled[static_cast<uint8>(Flag)];
}

struct FOptFlagInfo
{
    EOptFlag Flag;
    const char* Name;
    const char* Tooltip;
};

inline constexpr FOptFlagInfo GOptFlagInfos[] =
{
    { EOptFlag::FrustumCulling,  "Frustum Culling",   "절두체 밖 오브젝트를 그리지 않는다." },
    { EOptFlag::BVHCulling,      "BVH Culling",       "컬링을 BVH 순회로 한다." },
    { EOptFlag::LOD,             "LOD",               "멀리 있는 오브젝트를 단순화된 메시로 교체" },
    { EOptFlag::TransformCache,  "Transform Cache",   "월드 행렬을 변경될 때만 재계산." },
    { EOptFlag::DrawCallSorting, "Draw Call Sorting", "텍스처/메시로 정렬해 상태 변경을 줄인다." },
    { EOptFlag::BVHPicking,      "BVH Picking",       "피킹을 BVH로 한다." },
    { EOptFlag::PickingFrustum,  "Picking Frustum",   "화면 밖 오브젝트는 피킹 후보에서 제외" },
};