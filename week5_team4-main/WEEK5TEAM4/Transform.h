#pragma once
#include "Vector.h"
#include "Rotator.h"
#include "Matrix.h"
#include <cassert>

// 스케일 하한. 0에 가까워지면 MakeMatrix()의 행렬식(세 축 스케일의 곱)이 무너져
// FMatrix::Inverse()가 Identity를 돌려주고, 그 액터는 레이캐스트로 클릭할 수 없게 된다.
// SMALL_NUMBER는 부동소수점 오차를 재는 값이라 물리적 크기의 하한으로는 너무 작다.
constexpr float MIN_SCALE = 0.001f;

struct FTransform
{
	FTransform(){ }
	FTransform(FVector _Location, FRotator _Rotation, FVector _Scale) : Location(_Location), Rotation(_Rotation), Scale(_Scale)
	{
	}
	FVector Location = FVector(0);
	FRotator Rotation = FRotator(0, 0, 0);
	FVector Scale = FVector(1);

	/*FMatrix MakeMatrix() const
	{
		return  FMatrix::Scale(Scale) * FMatrix::Rotate(Rotation) * FMatrix::Translation(Location);
	}*/

	FMatrix MakeMatrix() const
	{
		FMatrix result = FMatrix::Rotate(Rotation);

		// (x, x, x, x) ... 
		const __m128 ScaleX = _mm_set1_ps(Scale.x);
		const __m128 ScaleY = _mm_set1_ps(Scale.y);
		const __m128 ScaleZ = _mm_set1_ps(Scale.z);

		const __m128 Row0 = _mm_load_ps(result.M[0]);
		const __m128 Row1 = _mm_load_ps(result.M[1]);
		const __m128 Row2 = _mm_load_ps(result.M[2]);

		_mm_store_ps(result.M[0], _mm_mul_ps(Row0, ScaleX));
		_mm_store_ps(result.M[1], _mm_mul_ps(Row1, ScaleY));
		_mm_store_ps(result.M[2], _mm_mul_ps(Row2, ScaleZ));

		const __m128 TranslationRow = _mm_set_ps(1.0f, Location.z, Location.y, Location.x);
		_mm_store_ps(result.M[3], TranslationRow);

		return result;

	}

	FMatrix InverseMatrix() const
	{
		assert(Scale.x == 0.f || Scale.y == 0.f || Scale.z == 0.f);

		return {
			FMatrix::Translation(FVector(-Location.x, -Location.y, -Location.z))
			* FMatrix::Rotate(Rotation).Transpose()
			* FMatrix::Scale(FVector(1.0f / Scale.x, 1.0f / Scale.y, 1.0f / Scale.z))
		};
	}
};
