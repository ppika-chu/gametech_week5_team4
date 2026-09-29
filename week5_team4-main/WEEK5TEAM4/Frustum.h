#pragma once

#include <cmath>
#include "FAABB.h"
#include "Matrix.h"
#include "Vector.h"
#include <immintrin.h>


enum class EFrustumResult
{
    Outside,
    Intersect,
    Inside
};

struct FFrustumPlane
{
    FVector Normal;
    float Distance = 0.0f;

    // dot(Normal, Point) + Distance >= 0이면 프러스텀 안쪽
    float SignedDistance(const FVector& Point) const
    {
        return FVector::dot(Normal, Point) + Distance;
    }

    void Normalize()
    {
        const float Length = Normal.Length();

        if (Length <= KINDA_SMALL_NUMBER)
        {
            return;
        }

        Normal /= Length;
        Distance /= Length;
    }
};

struct FFrustum
{
    enum EPlaneIndex
    {
        Left,
        Right,
        Bottom,
        Top,
        Near,
        Far,
        Count
    };

    FFrustumPlane Planes[Count];

    static FFrustum FromViewProjection(const FMatrix& ViewProjection)
    {
        FFrustum Result;

        // 행벡터 규약: Position * ViewProjection
        // 따라서 행이 아니라 열을 조합해야 한다.

        Result.Planes[Left] = {
            FVector(
                ViewProjection.M[0][3] + ViewProjection.M[0][0],
                ViewProjection.M[1][3] + ViewProjection.M[1][0],
                ViewProjection.M[2][3] + ViewProjection.M[2][0]
            ),
            ViewProjection.M[3][3] + ViewProjection.M[3][0]
        };

        Result.Planes[Right] = {
            FVector(
                ViewProjection.M[0][3] - ViewProjection.M[0][0],
                ViewProjection.M[1][3] - ViewProjection.M[1][0],
                ViewProjection.M[2][3] - ViewProjection.M[2][0]
            ),
            ViewProjection.M[3][3] - ViewProjection.M[3][0]
        };

        Result.Planes[Bottom] = {
            FVector(
                ViewProjection.M[0][3] + ViewProjection.M[0][1],
                ViewProjection.M[1][3] + ViewProjection.M[1][1],
                ViewProjection.M[2][3] + ViewProjection.M[2][1]
            ),
            ViewProjection.M[3][3] + ViewProjection.M[3][1]
        };

        Result.Planes[Top] = {
            FVector(
                ViewProjection.M[0][3] - ViewProjection.M[0][1],
                ViewProjection.M[1][3] - ViewProjection.M[1][1],
                ViewProjection.M[2][3] - ViewProjection.M[2][1]
            ),
            ViewProjection.M[3][3] - ViewProjection.M[3][1]
        };

        // DirectX clip space의 깊이 범위는 0 <= z <= w.
        Result.Planes[Near] = {
            FVector(
                ViewProjection.M[0][2],
                ViewProjection.M[1][2],
                ViewProjection.M[2][2]
            ),
            ViewProjection.M[3][2]
        };

        Result.Planes[Far] = {
            FVector(
                ViewProjection.M[0][3] - ViewProjection.M[0][2],
                ViewProjection.M[1][3] - ViewProjection.M[1][2],
                ViewProjection.M[2][3] - ViewProjection.M[2][2]
            ),
            ViewProjection.M[3][3] - ViewProjection.M[3][2]
        };

        for (int32 Index = 0; Index < Count; ++Index)
        {
            Result.Planes[Index].Normalize();
        }

        return Result;
    }

    EFrustumResult TestAABB(const FAABB& Bounds, float Epsilon = 0.001f) const
    {
        const FVector Center = (Bounds.Min + Bounds.Max) * 0.5f;
        const FVector Extent = (Bounds.Max - Bounds.Min) * 0.5f;

        bool bIntersects = false;

        for (int32 Index = 0; Index < Count; ++Index)
        {
            const FFrustumPlane& Plane = Planes[Index];

            const float Radius =
                std::abs(Plane.Normal.x) * Extent.x +
                std::abs(Plane.Normal.y) * Extent.y +
                std::abs(Plane.Normal.z) * Extent.z;

            const float CenterDistance = Plane.SignedDistance(Center);

            // AABB 전체가 이 평면의 바깥쪽에 있다.
            if (CenterDistance + Radius < -Epsilon)
            {
                return EFrustumResult::Outside;
            }

            // 평면을 가로지르는 AABB다.
            if (CenterDistance - Radius < Epsilon)
            {
                bIntersects = true;
            }
        }

        return bIntersects ? EFrustumResult::Intersect : EFrustumResult::Inside;
    }
};