#pragma once

#include "Vector.h"
#include "Matrix.h"

struct FAABB
{
	FVector Min;
	FVector Max;

	FAABB() = default;
	FAABB(const FVector& InMin, const FVector& InMax)
		: Min(InMin)
		, Max(InMax)
	{
	}

	inline void ExpandToInclude(const FVector& Point)
	{
		Min.x = FMath::Min(Min.x, Point.x);
		Min.y = FMath::Min(Min.y, Point.y);
		Min.z = FMath::Min(Min.z, Point.z);
		Max.x = FMath::Max(Max.x, Point.x);
		Max.y = FMath::Max(Max.y, Point.y);
		Max.z = FMath::Max(Max.z, Point.z);
	}

	inline void GetCorners(FVector Out[8]) const
	{
		Out[0] = FVector(Min.x, Min.y, Min.z);
		Out[1] = FVector(Max.x, Min.y, Min.z);
		Out[2] = FVector(Min.x, Max.y, Min.z);
		Out[3] = FVector(Max.x, Max.y, Min.z);
		Out[4] = FVector(Min.x, Min.y, Max.z);
		Out[5] = FVector(Max.x, Min.y, Max.z);
		Out[6] = FVector(Min.x, Max.y, Max.z);
		Out[7] = FVector(Max.x, Max.y, Max.z);
	}

	inline FAABB ToWorld(const FMatrix& Matrix) const
	{
		const FVector Center = (Min + Max) * 0.5f;
		const FVector Extent = (Max - Min) * 0.5f;

		const FVector WorldCenter = Matrix.TransformPosition(Center);

		FVector WorldExtent;
		for (int32 i = 0; i < 3; ++i)
		{
			WorldExtent[i] =
				FGenericPlatformMath::Abs(Matrix.M[0][i]) * Extent.x +
				FGenericPlatformMath::Abs(Matrix.M[1][i]) * Extent.y +
				FGenericPlatformMath::Abs(Matrix.M[2][i]) * Extent.z;
		}

		return FAABB(WorldCenter - WorldExtent, WorldCenter + WorldExtent);
	}

	template <typename Func>
	inline void ForEachCornerLines(Func&& f) const
	{
		FVector corners[8];
		GetCorners(corners);

		TPair<int32, int32> edges[] = {
			{ 0, 1 },{ 1, 3 },{ 3, 2 },{ 2, 0 },
			{ 4, 5 },{ 5, 7 },{ 7, 6 },{ 6, 4 },
			{ 0, 4 },{ 1, 5 },{ 2, 6 },{ 3, 7 }
		};

		for (const auto& edge : edges)
		{
			f(corners[edge.first], corners[edge.second]);
		}
	}
};
