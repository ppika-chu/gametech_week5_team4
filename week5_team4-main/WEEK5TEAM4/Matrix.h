#pragma once
#include "Vector.h"
#include "MathUtility.h"
#include "Rotator.h"
#include "Vector.h"
#include "enum.h"
#include <utility>
#include <xmmintrin.h>

struct FMatrix { 
	float M[4][4];

	static const FMatrix Identity;
	static const FMatrix Zero;

	// 언리얼 좌표계(X 전방 / Y 우측 / Z 상방)를
	// DirectX NDC(X 우측 / Y 위 / Z 화면 안쪽)로 바꾸는 축 교환 행렬.
	static const FMatrix UEToDX;

	FMatrix() : M{} {}
	
	FMatrix(float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23,
		float m30, float m31, float m32, float m33)
	{
		M[0][0] = m00; M[0][1] = m01; M[0][2] = m02; M[0][3] = m03;
		M[1][0] = m10; M[1][1] = m11; M[1][2] = m12; M[1][3] = m13;
		M[2][0] = m20; M[2][1] = m21; M[2][2] = m22; M[2][3] = m23;
		M[3][0] = m30; M[3][1] = m31; M[3][2] = m32; M[3][3] = m33;
	}

	FMatrix(const FVector4& M0, const FVector4& M1, const FVector4& M2, const FVector4& M3)
	{
		M[0][0] = M0.x; M[0][1] = M0.y; M[0][2] = M0.z; M[0][3] = M0.w;
		M[1][0] = M1.x; M[1][1] = M1.y; M[1][2] = M1.z; M[1][3] = M1.w;
		M[2][0] = M2.x; M[2][1] = M2.y; M[2][2] = M2.z; M[2][3] = M2.w;
		M[3][0] = M3.x; M[3][1] = M3.y; M[3][2] = M3.z; M[3][3] = M3.w;
	}

	static FMatrix makeIdentity() // 단위행렬 만드는 함수
	{
		FMatrix R = {};
		R.M[0][0] = R.M[1][1] = R.M[2][2] = R.M[3][3] = 1.0f;
		return R;
	}

	FMatrix operator* (const FMatrix& Other) const
	{
		FMatrix result;

		__m128 BRow0 = _mm_loadu_ps(&Other.M[0][0]);
		__m128 BRow1 = _mm_loadu_ps(&Other.M[1][0]);
		__m128 BRow2 = _mm_loadu_ps(&Other.M[2][0]);
		__m128 BRow3 = _mm_loadu_ps(&Other.M[3][0]);

		for (int row = 0; row < 4; ++row)
		{
			__m128 R = _mm_mul_ps(_mm_set1_ps(M[row][0]), BRow0);
			R = _mm_add_ps(R, _mm_mul_ps(_mm_set1_ps(M[row][1]), BRow1));
			R = _mm_add_ps(R, _mm_mul_ps(_mm_set1_ps(M[row][2]), BRow2));
			R = _mm_add_ps(R, _mm_mul_ps(_mm_set1_ps(M[row][3]), BRow3));
			_mm_storeu_ps(&result.M[row][0], R);
		}
		
		return result;

		#if 0
		for (int row = 0; row < 4;++row) {
			for (int col = 0;col < 4;++col) {
				for (int k = 0;k < 4;++k) {
					result.M[row][col] += M[row][k] * Other.M[k][col];
				}
			}
		}
		#endif
	}

	FMatrix operator*(float Scalar) const
	{
		FMatrix result;
		const __m128 S = _mm_set1_ps(Scalar);
		for (int row = 0; row < 4; ++row)
		{
			_mm_storeu_ps(result.M[row], _mm_mul_ps(_mm_loadu_ps(M[row]), S));
		}
		return result;
	}


	FMatrix operator+ (const FMatrix& Other) const
	{
		FMatrix result;
		for (int row = 0; row < 4; ++row)
		{
			_mm_storeu_ps(result.M[row], _mm_add_ps(_mm_loadu_ps(M[row]), _mm_loadu_ps(Other.M[row])));
		}
		return result;
	}



	FMatrix operator- (const FMatrix& Other) const
	{
		FMatrix result;
		for (int row = 0; row < 4; ++row)
		{
			_mm_storeu_ps(result.M[row], _mm_sub_ps(_mm_loadu_ps(M[row]), _mm_loadu_ps(Other.M[row])));
		}
		return result;
	}


	FMatrix operator+(float f) const
	{
		FMatrix result;
		const __m128 S = _mm_set1_ps(f);
		for (int row = 0; row < 4; ++row)
		{
			_mm_storeu_ps(result.M[row], _mm_add_ps(_mm_loadu_ps(M[row]), S));
		}
		return result;
	}

	FMatrix operator-(float f) const
	{
		FMatrix result;
		const __m128 S = _mm_set1_ps(f);
		for (int row = 0; row < 4; ++row)
		{
			_mm_storeu_ps(result.M[row], _mm_sub_ps(_mm_loadu_ps(M[row]), S));
		}
		return result;
	}

	FMatrix& operator*=(const FMatrix& Other)
	{
		*this = *this * Other;
		return *this;
	}

	bool operator==(const FMatrix& m) const
	{
		for (int row = 0; row < 4; ++row) {
			for (int col = 0; col < 4; ++col) {
				if (M[row][col] != m.M[row][col]) {
					return false;
				}
			}
		}
		return true;
	}

	bool operator!=(const FMatrix& m) const
	{
		return !(*this == m);
	}

	// 부동소수 오차를 감안한 비교.
	// 곱셈이나 역행렬로 만들어낸 행렬끼리는 == 대신 이쪽을 써야 한다
	bool Equals(const FMatrix& m, float Tolerance = KINDA_SMALL_NUMBER) const
	{
		for (int row = 0; row < 4; ++row) {
			for (int col = 0; col < 4; ++col) {
				if (FMath::Abs(M[row][col] - m.M[row][col]) > Tolerance) {
					return false;
				}
			}
		}
		return true;
	}

	FMatrix Transpose() const
	{ 
		__m128 R0 = _mm_loadu_ps(&M[0][0]);
		__m128 R1 = _mm_loadu_ps(&M[1][0]);
		__m128 R2 = _mm_loadu_ps(&M[2][0]);
		__m128 R3 = _mm_loadu_ps(&M[3][0]);
		_MM_TRANSPOSE4_PS(R0, R1, R2, R3);
		
		FMatrix result;
		_mm_storeu_ps(&result.M[0][0], R0);
		_mm_storeu_ps(&result.M[1][0], R1);
		_mm_storeu_ps(&result.M[2][0], R2);
		_mm_storeu_ps(&result.M[3][0], R3);

		return result;

		#if 0
		FMatrix result = {};
		for (int row = 0; row < 4; ++row) {
			for (int col = 0; col < 4; ++col) {
				result.M[row][col] = M[col][row];
			}
		}
		#endif
	}

	static FMatrix Scale(float n)
	  {
		FMatrix result = Identity;
		result.M[0][0] = n;
		result.M[1][1] = n;
		result.M[2][2] = n;

		return result;
	}

	static FMatrix Scale(const FVector v)
	{
		FMatrix result = Identity;
		result.M[0][0] = v.x;
		result.M[1][1] = v.y;
		result.M[2][2] = v.z;

		return result;
	}

	static FMatrix RotateX(float degree) // Roll : X축 회전
	{
		FMatrix result = Identity;
		float s, c;
		FMath::sincos<float>(s, c, degree * PI / 180);

		result.M[1][1] = c;
		result.M[1][2] = -s;
		result.M[2][1] = s;
		result.M[2][2] = c;

		return result;
	}

	static FMatrix RotateY(float degree) // Pitch : Y축 회전
	{
		FMatrix result = Identity;
		float s, c;
		FMath::sincos<float>(s, c, degree * PI / 180);

		result.M[0][0] = c;
		result.M[0][2] = s;
		result.M[2][0] = -s;
		result.M[2][2] = c;

		return result;
	}

	static FMatrix RotateZ(float degree) // Yaw : Z축 회전
	{
		FMatrix result = Identity;
		float s, c;
		FMath::sincos<float>(s, c, degree * PI / 180);

		result.M[0][0] = c;
		result.M[0][1] = s;
		result.M[1][0] = -s;
		result.M[1][1] = c;

		return result;
	}

	static FMatrix Rotate(const FRotator r)
	{
		//Pitch, Yaw, Roll의 각각 cossin 구하기
		FMatrix Matrix = FMatrix::Identity;
		float cosP, cosY, cosR;
		float sinP, sinY, sinR;

		//도 -> 라디안 변환 후 sincos 호출
		FMath::sincos<float>(sinP, cosP, r.Pitch * PI / 180);
		FMath::sincos<float>(sinY, cosY, r.Yaw * PI / 180);
		FMath::sincos<float>(sinR, cosR, r.Roll * PI / 180);

		Matrix.M[0][0] = cosP * cosY;
		Matrix.M[0][1] = cosP * sinY;
		Matrix.M[0][2] = sinP;
		Matrix.M[1][0] = sinR * sinP * cosY - cosR * sinY;
		Matrix.M[1][1] = sinR * sinP * sinY + cosR * cosY;
		Matrix.M[1][2] = -sinR * cosP;
		Matrix.M[2][0] = -(cosR * sinP * cosY + sinR * sinY);
		Matrix.M[2][1] = sinR * cosY - cosR * sinP * sinY;
		Matrix.M[2][2] = cosR * cosP;

		return Matrix;
	}

	static FMatrix Translation(const FVector v)
	{
		FMatrix result = Identity;
		result.M[3][0] = v.x;
		result.M[3][1] = v.y;
		result.M[3][2] = v.z;

		return result;
	}

	static FMatrix Ortho(float Left, float Right, float Bottom, float Top, float NearZ, float FarZ)
	{
		return FMatrix{
			FVector4(2.0f / (Right - Left), 0.0f, 0.0f, 0.0f),
			FVector4(0.0f, 2.0f / (Top - Bottom), 0.0f, 0.0f),
			FVector4(0.0f, 0.0f, 1.0f / (FarZ - NearZ), 0.0f),
			FVector4(-(Right + Left) / (Right - Left), -(Top + Bottom) / (Top - Bottom), -NearZ / (FarZ - NearZ), 1.0f)
		};
	}

	[[nodiscard]] FVector GetUnitAxis(EAxis Axis) const
	{
		const int i = static_cast<int>(Axis);
		return FVector(M[i][0], M[i][1], M[i][2]);
	}

	// 위치 변환 (w = 1, 이동 포함).  행벡터 규약 v x M
	[[nodiscard]] FVector TransformPosition(const FVector& V) const
	{
		const __m128 Row0 = _mm_loadu_ps(M[0]);
		const __m128 Row1 = _mm_loadu_ps(M[1]);
		const __m128 Row2 = _mm_loadu_ps(M[2]);
		const __m128 Row3 = _mm_loadu_ps(M[3]);

		const __m128 Vx = _mm_set1_ps(V.x);
		const __m128 Vy = _mm_set1_ps(V.y);
		const __m128 Vz = _mm_set1_ps(V.z);

		__m128 Result = _mm_mul_ps(Vx, Row0);
		Result = _mm_add_ps(Result, _mm_mul_ps(Vy, Row1));
		Result = _mm_add_ps(Result, _mm_mul_ps(Vz, Row2));
		Result = _mm_add_ps(Result, Row3);

		float Values[4];
		_mm_storeu_ps(Values, Result);

		return FVector(Values[0], Values[1], Values[2]);
	}

	// 방향 변환 (w = 0, 이동 제외)
	[[nodiscard]] FVector TransformVector(const FVector& V) const
	{
		const __m128 Row0 = _mm_loadu_ps(M[0]);
		const __m128 Row1 = _mm_loadu_ps(M[1]);
		const __m128 Row2 = _mm_loadu_ps(M[2]);

		const __m128 Vx = _mm_set1_ps(V.x);
		const __m128 Vy = _mm_set1_ps(V.y);
		const __m128 Vz = _mm_set1_ps(V.z);

		__m128 Result = _mm_mul_ps(Vx, Row0);
		Result = _mm_add_ps(Result, _mm_mul_ps(Vy, Row1));
		Result = _mm_add_ps(Result, _mm_mul_ps(Vz, Row2));

		float Values[4];
		_mm_storeu_ps(Values, Result);

		return FVector(Values[0], Values[1], Values[2]);
	}

	// General 4x4 inverse, including perspective projection. Keep Inverse() as
	// the affine fast path used by object transforms.
	[[nodiscard]] FMatrix Inverse() const
	{
		const float m00 = M[0][0], m01 = M[0][1], m02 = M[0][2], m03 = M[0][3];
		const float m10 = M[1][0], m11 = M[1][1], m12 = M[1][2], m13 = M[1][3];
		const float m20 = M[2][0], m21 = M[2][1], m22 = M[2][2], m23 = M[2][3];
		const float m30 = M[3][0], m31 = M[3][1], m32 = M[3][2], m33 = M[3][3];

		// 위쪽 두 행(0,1)에서 뽑은 2x2 소행렬식 6개
		const float S0 = m00 * m11 - m10 * m01;
		const float S1 = m00 * m12 - m10 * m02;
		const float S2 = m00 * m13 - m10 * m03;
		const float S3 = m01 * m12 - m11 * m02;
		const float S4 = m01 * m13 - m11 * m03;
		const float S5 = m02 * m13 - m12 * m03;

		// 아래쪽 두 행(2,3)에서 뽑은 2x2 소행렬식 6개
		const float C5 = m22 * m33 - m32 * m23;
		const float C4 = m21 * m33 - m31 * m23;
		const float C3 = m21 * m32 - m31 * m22;
		const float C2 = m20 * m33 - m30 * m23;
		const float C1 = m20 * m32 - m30 * m22;
		const float C0 = m20 * m31 - m30 * m21;

		// 라플라스 전개: det = Σ (위 2x2) * (대응하는 아래 2x2)
		const float Det = S0 * C5 - S1 * C4 + S2 * C3 + S3 * C2 - S4 * C1 + S5 * C0;
		if (FMath::Abs(Det) < SMALL_NUMBER)
		{
			return FMatrix::Zero;
		}
		const float Inv = 1.0f / Det;

		FMatrix R;
		R.M[0][0] = (m11 * C5 - m12 * C4 + m13 * C3) * Inv;
		R.M[0][1] = (-m01 * C5 + m02 * C4 - m03 * C3) * Inv;
		R.M[0][2] = (m31 * S5 - m32 * S4 + m33 * S3) * Inv;
		R.M[0][3] = (-m21 * S5 + m22 * S4 - m23 * S3) * Inv;

		R.M[1][0] = (-m10 * C5 + m12 * C2 - m13 * C1) * Inv;
		R.M[1][1] = (m00 * C5 - m02 * C2 + m03 * C1) * Inv;
		R.M[1][2] = (-m30 * S5 + m32 * S2 - m33 * S1) * Inv;
		R.M[1][3] = (m20 * S5 - m22 * S2 + m23 * S1) * Inv;

		R.M[2][0] = (m10 * C4 - m11 * C2 + m13 * C0) * Inv;
		R.M[2][1] = (-m00 * C4 + m01 * C2 - m03 * C0) * Inv;
		R.M[2][2] = (m30 * S4 - m31 * S2 + m33 * S0) * Inv;
		R.M[2][3] = (-m20 * S4 + m21 * S2 - m23 * S0) * Inv;

		R.M[3][0] = (-m10 * C3 + m11 * C1 - m12 * C0) * Inv;
		R.M[3][1] = (m00 * C3 - m01 * C1 + m02 * C0) * Inv;
		R.M[3][2] = (-m30 * S3 + m31 * S1 - m32 * S0) * Inv;
		R.M[3][3] = (m20 * S3 - m21 * S1 + m22 * S0) * Inv;

		return R;
	}

/*	// 아핀 행렬(마지막 열이 0,0,0,1)의 역행렬.
	// MakeMatrix() 결과가 항상 이 형태라 일반 4x4 역행렬이 필요 없다.
	//   M = | A 0 |        M^-1 = | A^-1     0 |
	//       | t 1 |               | -t*A^-1  1 |
	// Transpose() 와 달리 비균등 스케일에도 동작한다.
	[[nodiscard]] FMatrix AffineInverse() const
	{
		const float C00 =  (M[1][1] * M[2][2] - M[1][2] * M[2][1]);
		const float C01 = -(M[1][0] * M[2][2] - M[1][2] * M[2][0]);
		const float C02 =  (M[1][0] * M[2][1] - M[1][1] * M[2][0]);

		const float Det = M[0][0] * C00 + M[0][1] * C01 + M[0][2] * C02;
		if (FMath::Abs(Det) < SMALL_NUMBER)
		{
			return FMatrix::Zero;   // 스케일 0 등 역행렬이 없는 경우
		}

		const float C10 = -(M[0][1] * M[2][2] - M[0][2] * M[2][1]);
		const float C11 =  (M[0][0] * M[2][2] - M[0][2] * M[2][0]);
		const float C12 = -(M[0][0] * M[2][1] - M[0][1] * M[2][0]);
		const float C20 =  (M[0][1] * M[1][2] - M[0][2] * M[1][1]);
		const float C21 = -(M[0][0] * M[1][2] - M[0][2] * M[1][0]);
		const float C22 =  (M[0][0] * M[1][1] - M[0][1] * M[1][0]);

		const float Inv = 1.0f / Det;

		FMatrix R = FMatrix::Identity;

		// 수반행렬 = 여인수 행렬의 전치
		R.M[0][0] = C00 * Inv;  R.M[0][1] = C10 * Inv;  R.M[0][2] = C20 * Inv;
		R.M[1][0] = C01 * Inv;  R.M[1][1] = C11 * Inv;  R.M[1][2] = C21 * Inv;
		R.M[2][0] = C02 * Inv;  R.M[2][1] = C12 * Inv;  R.M[2][2] = C22 * Inv;

		// 이동 성분 : -t * A^-1
		R.M[3][0] = -(M[3][0] * R.M[0][0] + M[3][1] * R.M[1][0] + M[3][2] * R.M[2][0]);
		R.M[3][1] = -(M[3][0] * R.M[0][1] + M[3][1] * R.M[1][1] + M[3][2] * R.M[2][1]);
		R.M[3][2] = -(M[3][0] * R.M[0][2] + M[3][1] * R.M[1][2] + M[3][2] * R.M[2][2]);
		
		return R;
	}*/

	[[nodiscard]] FMatrix AffineInverse() const
	{
		const __m128 A = _mm_loadu_ps(M[0]);
		const __m128 B = _mm_loadu_ps(M[1]);
		const __m128 C = _mm_loadu_ps(M[2]);

		const auto Cross = [](__m128 X, __m128 Y) -> __m128
		{
			const __m128 Xyzx = _mm_shuffle_ps(X, X, _MM_SHUFFLE(3, 0, 2, 1));
			const __m128 Xzxy = _mm_shuffle_ps(X, X, _MM_SHUFFLE(3, 1, 0, 2));
			const __m128 Yyzx = _mm_shuffle_ps(Y, Y, _MM_SHUFFLE(3, 0, 2, 1));
			const __m128 Yzxy = _mm_shuffle_ps(Y, Y, _MM_SHUFFLE(3, 1, 0, 2));

			return _mm_sub_ps(
				_mm_mul_ps(Xyzx, Yzxy),
				_mm_mul_ps(Xzxy, Yyzx));
		};

		__m128 Cofactor0 = Cross(B, C);
		__m128 Cofactor1 = Cross(C, A);
		__m128 Cofactor2 = Cross(A, B);

		// det = dot(A, Cofactor0). w 성분은 0이다.
		const __m128 Products = _mm_mul_ps(A, Cofactor0);
		const __m128 PairSums = _mm_add_ps(Products, _mm_movehl_ps(Products, Products));
		const __m128 DetVector = _mm_add_ss(PairSums, _mm_shuffle_ps(PairSums, PairSums, _MM_SHUFFLE(1, 1, 1, 1)));
		const float Det = _mm_cvtss_f32(DetVector);
		
		if (FMath::Abs(Det) < SMALL_NUMBER)
		{
			return FMatrix::Zero;
		}

		const __m128 InvDet = _mm_set1_ps(1.0f / Det);
		Cofactor0 = _mm_mul_ps(Cofactor0, InvDet);
		Cofactor1 = _mm_mul_ps(Cofactor1, InvDet);
		Cofactor2 = _mm_mul_ps(Cofactor2, InvDet);

		// 여인수 행렬을 전치해 역행렬의 행 3개를 만든다.
		__m128 Row3 = _mm_setzero_ps();
		_MM_TRANSPOSE4_PS(Cofactor0, Cofactor1, Cofactor2, Row3);

		const __m128 Translation = _mm_sub_ps( _mm_setzero_ps(),
			_mm_add_ps(
				_mm_add_ps(
					_mm_mul_ps(_mm_set1_ps(M[3][0]), Cofactor0),
					_mm_mul_ps(_mm_set1_ps(M[3][1]), Cofactor1)),
				_mm_mul_ps(_mm_set1_ps(M[3][2]), Cofactor2)));

		FMatrix Result;
		_mm_storeu_ps(Result.M[0], Cofactor0);
		_mm_storeu_ps(Result.M[1], Cofactor1);
		_mm_storeu_ps(Result.M[2], Cofactor2);
		_mm_storeu_ps(Result.M[3], _mm_add_ps(Translation, _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f)));

		return Result;
	}

	// 월드 transform 행렬일 때, 이동 벡터만 뽑아오는 용도
	FVector GetOrigin() const
	{
		return FVector(M[3][0], M[3][1], M[3][2]); 		
	}
	
	// end Struct Matrix
};

// 행벡터 규약: V * M. 투영 시 동차 좌표 w까지 유지한다.
inline FVector4 operator*(const FVector4& V, const FMatrix& M)
{
	return FVector4(
		V.x * M.M[0][0] + V.y * M.M[1][0] + V.z * M.M[2][0] + V.w * M.M[3][0],
		V.x * M.M[0][1] + V.y * M.M[1][1] + V.z * M.M[2][1] + V.w * M.M[3][1],
		V.x * M.M[0][2] + V.y * M.M[1][2] + V.z * M.M[2][2] + V.w * M.M[3][2],
		V.x * M.M[0][3] + V.y * M.M[1][3] + V.z * M.M[2][3] + V.w * M.M[3][3]);
}

inline const FMatrix FMatrix::Identity = {
	FVector4(1, 0, 0, 0),
	FVector4(0, 1, 0, 0),
	FVector4(0, 0, 1, 0),
	FVector4(0, 0, 0, 1)
};

inline const FMatrix FMatrix::Zero = {
	FVector4(0, 0, 0, 0),
	FVector4(0, 0, 0, 0),
	FVector4(0, 0, 0, 0),
	FVector4(0, 0, 0, 0)
};

inline const FMatrix FMatrix::UEToDX = {
	FVector4(0, 0, 1, 0),
	FVector4(1, 0, 0, 0),
	FVector4(0, 1, 0, 0),
	FVector4(0, 0, 0, 1)

};
