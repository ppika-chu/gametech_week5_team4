#pragma once
#include "Core.h"

// LOD 전환 거리. LOD1만 슬라이더로 조정하고, LOD2는 그 값에 비례해서 살짝 더 멀리 잡는다.
inline float GLOD1DistanceRatio = 50.0f;
inline constexpr float GLOD2DistanceMultiplier = 1.5f;   
inline float GetLOD2DistanceRatio() { return GLOD1DistanceRatio * GLOD2DistanceMultiplier; }

enum class ECullingMode : uint8
{
    Off = 0,    // 컬링 안 함. 전부 그린다
    Linear,     // 전체 액터 선형 순회 + 오브젝트별 프러스텀 검사
    BVH,        // BVH 순회로 서브트리 단위 스킵
    Count
};

enum class EPickingMode : uint8
{
    BruteForce = 0, // 후보 전체를 레이캐스트
    BVH,            // BVH 순회
    BVHFrustum,     // BVH 순회 + 화면 밖 후보 제외
    Count
};

inline ECullingMode GCullingMode = ECullingMode::BVH;
inline EPickingMode GPickingMode = EPickingMode::BVHFrustum;

// enum 순서와 일치해야 함 (ImGui Combo 라벨)
inline constexpr const char* GCullingModeNames[] = { "Off", "Linear", "BVH" };
inline constexpr const char* GPickingModeNames[] = { "Brute Force", "BVH", "BVH + Frustum" };

// --- 서로 독립적으로 켜고 끄는 플래그 ---

enum class EOptFlag : uint8
{
    LOD = 0,
    TransformCache,
    DrawCallSorting,
    Count
};

// 위 enum과 순서 동일해야 함.
inline bool GOptEnabled[static_cast<uint8>(EOptFlag::Count)] =
{
    true,   // LOD
    true,   // TransformCache
    true,   // DrawCallSorting
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
    { EOptFlag::LOD,             "LOD",               "멀리 있는 오브젝트를 단순화된 메시로 교체" },
    { EOptFlag::TransformCache,  "Transform Cache",   "월드 행렬을 변경될 때만 재계산" },
    { EOptFlag::DrawCallSorting, "Draw Call Sorting", "Early-Z 정렬 + 텍스처/메시 묶어 상태 캐싱" },
};
