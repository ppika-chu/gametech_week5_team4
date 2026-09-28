#pragma once

#include "Vector.h"
#include "Matrix.h"
#include "FAABB.h"
#include "MathUtility.h"

struct FPlane
{
	FVector	Normal;
	FVector AbsoluteNormal;
	float	Offset;
	
	FPlane() : Normal(FVector(0, 0, 0)), AbsoluteNormal(FVector(0,0,0)), Offset(0) {}

	explicit FPlane(const FVector4& Value)
	{

		float Length = Value.ToVec3().Length();
		if (Length < KINDA_SMALL_NUMBER)
		{
			Normal = FVector(0, 0, 0);
			Offset = 0;
			return;
		}
		float InverseLength = 1 / Length;
		Normal = FVector(Value.x * InverseLength, Value.y * InverseLength, Value.z * InverseLength);
		AbsoluteNormal = FVector(FMath::Abs(Normal.x), FMath::Abs(Normal.y), FMath::Abs(Normal.z));
		Offset = Value.w * InverseLength;
	}

	float SignedDistance(const FVector& Point) const
	{
		return FVector::dot(Normal, Point) + Offset;
	}
};


struct FFrustum
{
	enum class EPlane { Near, Far, Right, Left, Top, Bottom, Count };

	FPlane Faces[static_cast<int32>(EPlane::Count)];

	FFrustum() = default;

	FPlane& operator[] (EPlane P)
	{
		return (Faces[static_cast<int>(P)]);
	}

	bool CheckFrustumCulling(const FAABB& AABB) const
	{
		FVector Center;
		FVector Extent;

		Center = (AABB.Max + AABB.Min) * 0.5f;
		Extent = (AABB.Max - AABB.Min) * 0.5f;

		for (int32 i = 0; i < static_cast<int32>(EPlane::Count); i++)
		{
			float Distance = Faces[i].SignedDistance(Center);
			float ProjectedExtent = FVector::dot(Extent, Faces[i].AbsoluteNormal);

			if (Distance < -ProjectedExtent)
				return (true);
		}
		return (false);
	}

	enum class EFrustumTestResult { Outside, Inside, Intersect };

	EFrustumTestResult ClassifyAABB(const FAABB& AABB, uint32& PlaneMask) const
	{
		FVector Center;
		FVector Extent;

		Center = (AABB.Max + AABB.Min) * 0.5f;
		Extent = (AABB.Max - AABB.Min) * 0.5f;

		for (int32 i = 0; i < static_cast<int32>(EPlane::Count); i++)
		{
			if ((PlaneMask & (1u << i)) == 0)
			{
				continue;
			}

			float Distance = Faces[i].SignedDistance(Center);
			float ProjectedExtent = FVector::dot(Extent, Faces[i].AbsoluteNormal);

			if (Distance < -ProjectedExtent)
				return (EFrustumTestResult::Outside);
			else if (Distance > ProjectedExtent)
				PlaneMask &= ~(1u << i);
		}
		if (PlaneMask == 0)
			return (EFrustumTestResult::Inside);

		return (EFrustumTestResult::Intersect);
	}
};

inline FFrustum createFrustumFromCamera(const FMatrix& Matrix)
{
	FFrustum     Frustum;

	const FVector4 ClipX = FVector4(Matrix.M[0][0], Matrix.M[1][0], Matrix.M[2][0], Matrix.M[3][0]); // x
	const FVector4 ClipY = FVector4(Matrix.M[0][1], Matrix.M[1][1], Matrix.M[2][1], Matrix.M[3][1]); // y
	const FVector4 ClipZ = FVector4(Matrix.M[0][2], Matrix.M[1][2], Matrix.M[2][2], Matrix.M[3][2]); // z
	const FVector4 ClipW = FVector4(Matrix.M[0][3], Matrix.M[1][3], Matrix.M[2][3], Matrix.M[3][3]); // w

	Frustum[FFrustum::EPlane::Left] = FPlane(ClipW + ClipX); // - w' ≤ x' (왼쪽 끝보다 오른쪽)
	Frustum[FFrustum::EPlane::Right] = FPlane(ClipW - ClipX); // x' ≤ w' (오른쪽 끝보다 왼쪽)
	Frustum[FFrustum::EPlane::Bottom] = FPlane(ClipW + ClipY); // - w' ≤ y' (아래 끝보다 위)
	Frustum[FFrustum::EPlane::Top] = FPlane(ClipW - ClipY); // y' ≤ w' (위 끝보다 아래)
	Frustum[FFrustum::EPlane::Near] = FPlane(ClipZ); // 0 ≤ z' (Near보다 멀리)
	Frustum[FFrustum::EPlane::Far] = FPlane(ClipW - ClipZ); // z' ≤ w' (Far보다 가까이)

	return Frustum;
}



