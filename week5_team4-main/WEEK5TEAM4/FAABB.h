#pragma once

#include "Vector.h"
#include "Matrix.h"
#include <xmmintrin.h>

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

		__m128 R0 = _mm_loadu_ps(&Matrix.M[0][0]);
		__m128 R1 = _mm_loadu_ps(&Matrix.M[1][0]);
		__m128 R2 = _mm_loadu_ps(&Matrix.M[2][0]);
		__m128 R3 = _mm_setzero_ps();

		_MM_TRANSPOSE4_PS(R0, R1, R2, R3);

		const __m128 AbsMask = _mm_set1_ps(-0.0f);
		const __m128 ExtentV = _mm_set_ps(0.f, Extent.z, Extent.y, Extent.x);

		__m128 Wx = _mm_mul_ps(_mm_andnot_ps(AbsMask, R0), ExtentV);
		__m128 Wy = _mm_mul_ps(_mm_andnot_ps(AbsMask, R1), ExtentV);
		__m128 Wz = _mm_mul_ps(_mm_andnot_ps(AbsMask, R2), ExtentV);

		float A[4], B[4], C[4];
		_mm_storeu_ps(A, Wx); _mm_storeu_ps(B, Wy); _mm_storeu_ps(C, Wz);

		const FVector WorldExtent(A[0]+A[1]+A[2], B[0]+B[1]+B[2], C[0]+C[1]+C[2]);
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
