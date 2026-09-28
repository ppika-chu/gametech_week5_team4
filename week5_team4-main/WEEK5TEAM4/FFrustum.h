#pragma once

#include "Vector.h"
#include "Matrix.h"
#include "FAABB.h"

struct FPlane
{
	FVector	Normal;
	float	Offset;
	
	FPlane() : Normal(FVector(0, 0, 0)), Offset(0) {}

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

	bool CheckFrustumCulling(FAABB AABB)
	{
		FVector Center;
		FVector Extent;

		Center = (AABB.Max + AABB.Min) * 0.5f;
		Extent = (AABB.Max - AABB.Min) * 0.5f;

		for (int32 i = 0; i < static_cast<int32>(EPlane::Count); i++)
		{
			float Distance = Faces[i].SignedDistance(Center);
			float R = (fabs(Faces[i].Normal.x) * Extent.x) + (fabs(Faces[i].Normal.y) * Extent.y) + (fabs(Faces[i].Normal.z) * Extent.z);

			if (Distance < -R)
				return (true);
		}
		return (false);
	}
};


//  ① - w' ≤ x'     (왼쪽 끝보다 오른쪽)
//	② x' ≤ w'      (오른쪽 끝보다 왼쪽)
//	③ - w' ≤ y'     (아래 끝보다 위)
//	④ y' ≤ w'      (위 끝보다 아래)
//	⑤ 0 ≤ z'       (Near보다 멀리)
//	⑥ z' ≤ w'      (Far보다 가까이)


inline FFrustum createFrustumFromCamera(const FMatrix& Matrix)
{
	FFrustum     frustum;

	const FVector4 C0 = FVector4(Matrix.M[0][0], Matrix.M[1][0], Matrix.M[2][0], Matrix.M[3][0]); // x
	const FVector4 C1 = FVector4(Matrix.M[0][1], Matrix.M[1][1], Matrix.M[2][1], Matrix.M[3][1]); // y
	const FVector4 C2 = FVector4(Matrix.M[0][2], Matrix.M[1][2], Matrix.M[2][2], Matrix.M[3][2]); // z
	const FVector4 C3 = FVector4(Matrix.M[0][3], Matrix.M[1][3], Matrix.M[2][3], Matrix.M[3][3]); // w

	frustum[FFrustum::EPlane::Left] = FPlane(C3 + C0);
	frustum[FFrustum::EPlane::Right] = FPlane(C3 - C0);
	frustum[FFrustum::EPlane::Bottom] = FPlane(C3 + C1);
	frustum[FFrustum::EPlane::Top] = FPlane(C3 - C1);
	frustum[FFrustum::EPlane::Near] = FPlane(C2);
	frustum[FFrustum::EPlane::Far] = FPlane(C3 - C2);

	return frustum;
}



