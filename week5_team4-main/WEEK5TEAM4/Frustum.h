#pragma once
#pragma once

#include "Matrix.h"
#include "FAABB.h"
#include "MathUtility.h"

struct FFrustumPlane
{
	// 평면 방정식: Ax + By + Cz + D = 0
    FVector Normal;
    float D = 0.0f;

	// 점이 평면의 어느 쪽에 있는지 확인
	// 평면의 normal이 절두체 안쪽을 향하도록 정의되어 있음
	// 평면의 normal과 점을 내적한 값에 D를 더한 값이 0보다 크면 절두체 안쪽, 0이면 평면 위, 0보다 작으면 절두체 바깥쪽
    float SignedDistance(const FVector& Point) const
    {
        return FVector::dot(Normal, Point) + D;
    }
};

struct FFrustum
{
    enum
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

    static FFrustumPlane MakePlane(float A, float B, float C, float D)
    {
        FFrustumPlane Result;

        const float Length = FMath::Sqrt(A * A + B * B + C * C);

        if (Length <= 1.0e-8f)
        {
            return Result;
        }

        const float InvLength = 1.0f / Length;

        Result.Normal = FVector(
            A * InvLength,
            B * InvLength,
            C * InvLength
        );

        Result.D = D * InvLength;

        return Result;
    }

    void Build(const FMatrix& ViewProjection)
    {
        // 현재 엔진은 행벡터:
        //
        // ClipPosition = WorldPosition * ViewProjection
        //
        // 따라서 프러스텀 평면은 행이 아니라
        // ViewProjection의 열을 조합해서 추출한다.

        // Left: x + w >= 0
        Planes[Left] = MakePlane(
            ViewProjection.M[0][3] + ViewProjection.M[0][0],
            ViewProjection.M[1][3] + ViewProjection.M[1][0],
            ViewProjection.M[2][3] + ViewProjection.M[2][0],
            ViewProjection.M[3][3] + ViewProjection.M[3][0]
        );

        // Right: w - x >= 0
        Planes[Right] = MakePlane(
            ViewProjection.M[0][3] - ViewProjection.M[0][0],
            ViewProjection.M[1][3] - ViewProjection.M[1][0],
            ViewProjection.M[2][3] - ViewProjection.M[2][0],
            ViewProjection.M[3][3] - ViewProjection.M[3][0]
        );

        // Bottom: y + w >= 0
        Planes[Bottom] = MakePlane(
            ViewProjection.M[0][3] + ViewProjection.M[0][1],
            ViewProjection.M[1][3] + ViewProjection.M[1][1],
            ViewProjection.M[2][3] + ViewProjection.M[2][1],
            ViewProjection.M[3][3] + ViewProjection.M[3][1]
        );

        // Top: w - y >= 0
        Planes[Top] = MakePlane(
            ViewProjection.M[0][3] - ViewProjection.M[0][1],
            ViewProjection.M[1][3] - ViewProjection.M[1][1],
            ViewProjection.M[2][3] - ViewProjection.M[2][1],
            ViewProjection.M[3][3] - ViewProjection.M[3][1]
        );

        // DirectX의 NDC 깊이 범위는 0~1
        // Near: z >= 0
        Planes[Near] = MakePlane(
            ViewProjection.M[0][2],
            ViewProjection.M[1][2],
            ViewProjection.M[2][2],
            ViewProjection.M[3][2]
        );

        // Far: w - z >= 0
        Planes[Far] = MakePlane(
            ViewProjection.M[0][3] - ViewProjection.M[0][2],
            ViewProjection.M[1][3] - ViewProjection.M[1][2],
            ViewProjection.M[2][3] - ViewProjection.M[2][2],
            ViewProjection.M[3][3] - ViewProjection.M[3][2]
        );
    }

    bool Intersects(const FAABB& Bounds) const
    {
        const FVector Center = (Bounds.Min + Bounds.Max) * 0.5f;

        const FVector Extent = (Bounds.Max - Bounds.Min) * 0.5f;

        for (int32 Index = 0; Index < Count; ++Index)
        {
            const FFrustumPlane& Plane = Planes[Index];

            // AABB를 평면 Normal에 투영한 반지름
            const float Radius = 
                FMath::Abs(Plane.Normal.x) * Extent.x
                + FMath::Abs(Plane.Normal.y) * Extent.y
                + FMath::Abs(Plane.Normal.z) * Extent.z;

            const float Distance = Plane.SignedDistance(Center);

            // AABB 전체가 평면 바깥에 있음
            if (Distance + Radius < 0.0f)
            {
                return false;
            }
        }

        return true;
    }
};