#pragma once

#include "Vector.h"
#include "Matrix.h"
#include "FQuaternion.h"
#include "MathUtility.h"
#include <functional>
#include "FAABB.h"

template <typename T>
inline T Map(T Value, T InMin, T InMax, T OutMin, T OutMax)
{
	return (Value - InMin) / (InMax - InMin) * (OutMax - OutMin) + OutMin;
}

inline float PointToLineSegmentDistanceSquared(const FVector2& Point, const FVector2& LineStart, const FVector2& LineEnd)
{
	FVector2 LineVec = LineEnd - LineStart;

	float LineLength = LineVec.Length();
	if (LineLength == 0.f)
	{
		return FVector2::LengthSquared(Point,  LineStart);
	}
	LineVec /= LineLength;

	FVector2 StartToPoint = Point - LineStart;
	float ProjectedLength = FVector2::Dot(StartToPoint, LineVec);

	if (ProjectedLength < 0.f)
	{
		ProjectedLength = 0.f;
	}
	else if (ProjectedLength > LineLength)
	{
		ProjectedLength = LineLength;
	}

	FVector2 Closest = LineStart + LineVec * ProjectedLength;
	return FVector2::LengthSquared(Point, Closest);
}

inline float PointToLineSegmentDistanceSquared(const FVector& Point, const FVector& LineStart, const FVector& LineEnd)
{
	FVector LineVec = LineEnd - LineStart;

	float LineLength = LineVec.Length();
	if (LineLength == 0.f)
	{
		return FVector::LengthSquared(Point, LineStart);
	}

	LineVec /= LineLength;

	FVector StartToPoint = Point - LineStart;
	float ProjectedLength = FVector::dot(StartToPoint, LineVec);

	if (ProjectedLength < 0.f)
	{
		ProjectedLength = 0.f;
	}
	else if (ProjectedLength > LineLength)
	{
		ProjectedLength = LineLength;
	}

	FVector Closest = LineStart + LineVec * ProjectedLength;
	return FVector::LengthSquared(Point, Closest);
}

inline void GenerateCircleVertices(const std::function<void(int32 Index, const FVector2&)>& Handler, float Radius, int Segments)
{
	const float Step = 2.0f * PI / static_cast<float>(Segments);

	for (int32 Index = 0; Index < Segments; ++Index)
	{
		float Angle = Step * static_cast<float>(Index);
		float X = Radius * cos(Angle);
		float Y = Radius * sin(Angle);

		Handler(Index, FVector2(X, Y));
	}
}

inline FVector2 WorldToScreen(const FVector& WorldPos, const FMatrix& ViewProjection, float ScreenWidth, float ScreenHeight)
{
	const FVector4 ClipSpacePos = FVector4(WorldPos, 1.f) * ViewProjection;

	FVector2 NdcPos(ClipSpacePos.x / ClipSpacePos.w, ClipSpacePos.y / ClipSpacePos.w);
	FVector2 ScreenPos(
		((NdcPos.X + 1.0f) * 0.5f * ScreenWidth),
		((1.0f - (NdcPos.Y + 1.0f) * 0.5f) * ScreenHeight)
	);

	return ScreenPos;
}

inline FVector ScreenToWorld(const FVector2& ScreenPos, const FMatrix& InverseViewProjection, float ScreenWidth, float ScreenHeight, float Depth = 1.0f)
{
	FVector2 NdcPos(
		(ScreenPos.X / ScreenWidth) * 2.0f - 1.0f,
		1.0f - (ScreenPos.Y / ScreenHeight) * 2.0f
	);

	FVector4 ClipSpacePos(NdcPos.X, NdcPos.Y, Depth, 1.0f);
	FVector4 WorldSpacePos = ClipSpacePos * InverseViewProjection;

	return FVector(WorldSpacePos.x / WorldSpacePos.w, WorldSpacePos.y / WorldSpacePos.w, WorldSpacePos.z / WorldSpacePos.w);
}

inline FMatrix ToMatrix(const FQuaternion& Q)
{
	float XX = Q.X * Q.X;
	float YY = Q.Y * Q.Y;
	float ZZ = Q.Z * Q.Z;
	float XY = Q.X * Q.Y;
	float XZ = Q.X * Q.Z;
	float YZ = Q.Y * Q.Z;
	float WX = Q.W * Q.X;
	float WY = Q.W * Q.Y;
	float WZ = Q.W * Q.Z;

	return FMatrix(
		FVector4(1.0f - 2.0f * (YY + ZZ), 2.0f * (XY + WZ), 2.0f * (XZ - WY), 0.0f),
		FVector4(2.0f * (XY - WZ), 1.0f - 2.0f * (XX + ZZ), 2.0f * (YZ + WX), 0.0f),
		FVector4(2.0f * (XZ + WY), 2.0f * (YZ - WX), 1.0f - 2.0f * (XX + YY), 0.0f),
		FVector4(0.0f, 0.0f, 0.0f, 1.0f)
	);
}

inline FQuaternion ToQuaternion(const FMatrix& Matrix)
{
	float Trace = Matrix.M[0][0] + Matrix.M[1][1] + Matrix.M[2][2];

	FQuaternion Q;
	if (Trace > 0.f)
	{
		float S = sqrt(Trace + 1.f);
		Q[3] = S * 0.5f;

		float T = 0.5f / S;

		Q[0] = (Matrix.M[1][2] - Matrix.M[2][1]) * T;
		Q[1] = (Matrix.M[2][0] - Matrix.M[0][2]) * T;
		Q[2] = (Matrix.M[0][1] - Matrix.M[1][0]) * T;
	}
	else
	{
		int32 I = 0;
		if (Matrix.M[1][1] > Matrix.M[0][0]) I = 1;
		if (Matrix.M[2][2] > Matrix.M[I][I]) I = 2;

		static const int32 Next[3] = { 1, 2, 0 };

		int32 J = Next[I];
		int32 K = Next[J];

		float S = sqrt((Matrix.M[I][I] - (Matrix.M[J][J] + Matrix.M[K][K])) + 1.f);
		Q[I] = S * 0.5f;

		float T = S;
		if (S != 0.f) T = 0.5f / S;

		Q[3] = (Matrix.M[J][K] - Matrix.M[K][J]) * T;
		Q[J] = (Matrix.M[J][I] + Matrix.M[I][J]) * T;
		Q[K] = (Matrix.M[K][I] + Matrix.M[I][K]) * T;
	}

	return Q;
}

inline FVector ExtractRotationFromMatrix(const FMatrix& Matrix)
{
	float Y = asin(FMath::Clamp(Matrix.M[0][2], -1.f, 1.f));
	float X = atan2(-Matrix.M[1][2], Matrix.M[2][2]);
	float Z = atan2(Matrix.M[0][1], Matrix.M[0][0]);
	return FVector(X, Y, Z);
}

inline FVector ToEulerAngles(const FQuaternion& Q)
{
	FMatrix Matrix = ToMatrix(Q);
	return ExtractRotationFromMatrix(Matrix);
}

inline FVector Lerp(const FVector& A, const FVector& B, float T)
{
	return A * (1.0f - T) + B * T;
}

inline bool RayIntersectsTriangle(const FVector& Origin, const FVector& Dir, const FVector& V0, const FVector& V1, const FVector& V2, float& OutT, float& OutU, float& OutV)
{
	static const float EPSILON = 1e-6f;

	//삼각형판정 => O +tD = V0+ uE1+vE2
	// -tD + uE1 + vE2 = O - V0
	//E2=v2-v0. E1=v1-v0

	FVector D = Dir - Origin;
	FVector T = Origin - V0;
	FVector E2 = V2 - V0;
	FVector E1 = V1 - V0;
	FVector P = FVector::cross(D, E2);
	float Det = FVector::dot(E1, P);

	if (fabsf(Det) < EPSILON) return false;   // 평면과 평행

	float InvDet = 1.0f / Det;

	OutU = FVector::dot(T, P) * InvDet;
	if (OutU < 0.0f || OutU > 1.0f) return false;

	FVector Q = FVector::cross(T, E1);
	OutV = FVector::dot(D, Q) * InvDet;
	if (OutV < 0.0f || OutU + OutV > 1.0f) return false;

	OutT = FVector::dot(E2, Q) * InvDet;

	return (OutT > EPSILON);                  // 광선 앞쪽만

	// OutT : 맞은물체가 얼마나 가까이있나(float)
	// OutU, OutV 정확환 클릭지점을 확인하려면 필요
}

inline bool RayIntersectsAABB(const FRay& Ray, float Distance, const FAABB& AABB, float& OutEnter)
{
	if (Distance < 0.f)	return false;

	__m128 Origin = _mm_set_ps(1.f, Ray.Origin.z, Ray.Origin.y, Ray.Origin.x);
	__m128 InvDir = _mm_set_ps(1.f, 1.f/Ray.Direction.z, 1.f/Ray.Direction.y, 1.f/Ray.Direction.x);
	__m128 Min = _mm_set_ps(0.f, AABB.Min.z, AABB.Min.y, AABB.Min.x);
	__m128 Max = _mm_set_ps(0.f, AABB.Max.z, AABB.Max.y, AABB.Max.x);

	__m128 T0 = _mm_mul_ps(_mm_sub_ps(Min, Origin), InvDir);
	__m128 T1 = _mm_mul_ps(_mm_sub_ps(Max, Origin), InvDir);
	__m128 TMin = _mm_min_ps(T0, T1);
	__m128 TMax = _mm_max_ps(T0, T1);

	float MinArr[4], MaxArr[4];
	_mm_storeu_ps(MinArr, TMin);
	_mm_storeu_ps(MaxArr, TMax);

	float Enter = (std::max)({0.f, MinArr[0], MinArr[1], MinArr[2]});
	float Exit = (std::min)({Distance, MaxArr[0], MaxArr[1], MaxArr[2]});
	OutEnter = Enter;

	return Enter <= Exit;
}

struct  alignas(16) FFrustum
{
	// Left, Right, Bottom, Top, Near, Far
	FVector4 Planes[6];

	__m128 PlaneX[2];
	__m128 PlaneY[2];
	__m128 PlaneZ[2];
	__m128 PlaneW[2];

	__m128 AbsPlaneX[2];
	__m128 AbsPlaneY[2];
	__m128 AbsPlaneZ[2];

	// Frustum 각 평면의 A,B,C,D 저장
	static FFrustum FromViewProjection(const FMatrix& VP)
	{
		// VP 행렬에서 k열 반환
		auto Col = [&VP](int32 k)
		{
			return FVector4(VP.M[0][k], VP.M[1][k], VP.M[2][k], VP.M[3][k]);
		};

		const FVector4 Col0 = Col(0);
		const FVector4 Col1 = Col(1);
		const FVector4 Col2 = Col(2);
		const FVector4 Col3 = Col(3);

		FFrustum F{};

		F.Planes[0] = Col0 + Col3;
		F.Planes[1] = Col3 - Col0;
		F.Planes[2] = Col1 + Col3;
		F.Planes[3] = Col3 - Col1;
		F.Planes[4] = Col2;			// -w 가 0 이므로 
		F.Planes[5] = Col3 - Col2;

		// 첫 번째 그룹: Left, Right, Bottom, Top.
		F.PlaneX[0] = _mm_setr_ps(F.Planes[0].x, F.Planes[1].x,	F.Planes[2].x, F.Planes[3].x);
		F.PlaneY[0] = _mm_setr_ps(F.Planes[0].y, F.Planes[1].y, F.Planes[2].y, F.Planes[3].y);
		F.PlaneZ[0] = _mm_setr_ps(F.Planes[0].z, F.Planes[1].z, F.Planes[2].z, F.Planes[3].z);
		F.PlaneW[0] = _mm_setr_ps(F.Planes[0].w, F.Planes[1].w, F.Planes[2].w, F.Planes[3].w);

		// 두 번째 그룹: Near, Far, Dummy, Dummy.
		// Dummy plane의 모든 계수가 0이므로 항상 판정을 통과
		F.PlaneX[1] = _mm_setr_ps(F.Planes[4].x, F.Planes[5].x, 0.0f, 0.0f);
		F.PlaneY[1] = _mm_setr_ps(F.Planes[4].y, F.Planes[5].y,	0.0f, 0.0f);
		F.PlaneZ[1] = _mm_setr_ps(F.Planes[4].z, F.Planes[5].z, 0.0f, 0.0f);
		F.PlaneW[1] = _mm_setr_ps(F.Planes[4].w, F.Planes[5].w, 0.0f, 0.0f);

		// andnot을 사용하면 float 절댓값을 구할 수 있다.
		const __m128 SignMask = _mm_set1_ps(-0.0f);

		for (int32 Group = 0; Group < 2; ++Group)
		{
			F.AbsPlaneX[Group] = _mm_andnot_ps(SignMask, F.PlaneX[Group]);
			F.AbsPlaneY[Group] = _mm_andnot_ps(SignMask, F.PlaneY[Group]);
			F.AbsPlaneZ[Group] = _mm_andnot_ps(SignMask, F.PlaneZ[Group]);
		}

		return F;
	}

	inline bool Intersects(const FAABB& AABB) const
	{


		const __m128 CenterX = _mm_set1_ps((AABB.Min.x + AABB.Max.x) * 0.5f);
		const __m128 CenterY = _mm_set1_ps((AABB.Min.y + AABB.Max.y) * 0.5f);
		const __m128 CenterZ = _mm_set1_ps((AABB.Min.z + AABB.Max.z) * 0.5f);

		// AABB extent도 각각 네 SIMD lane에 복제한다.
		const __m128 ExtentX = _mm_set1_ps((AABB.Max.x - AABB.Min.x) * 0.5f);
		const __m128 ExtentY = _mm_set1_ps((AABB.Max.y - AABB.Min.y) * 0.5f);
		const __m128 ExtentZ = _mm_set1_ps((AABB.Max.z - AABB.Min.z) * 0.5f);

		const __m128 Zero = _mm_setzero_ps();

		// Group 0에서 Left/Right/Bottom/Top을 동시에 검사하고,
		// Group 1에서 Near/Far/Dummy/Dummy를 동시에 검사한다.
		for (int32 Group = 0; Group < 2; ++Group)
		{
			// Distance =
			// Nx * CenterX +
			// Ny * CenterY +
			// Nz * CenterZ +
			// D
			__m128 Distance = _mm_mul_ps(PlaneX[Group],	CenterX);

			Distance = _mm_add_ps(Distance,	_mm_mul_ps(PlaneY[Group], CenterY));
			Distance = _mm_add_ps(Distance,	_mm_mul_ps(PlaneZ[Group], CenterZ));
			Distance = _mm_add_ps(Distance,	PlaneW[Group]);

			// Radius =
			// abs(Nx) * ExtentX +
			// abs(Ny) * ExtentY +
			// abs(Nz) * ExtentZ
			__m128 Radius = _mm_mul_ps(AbsPlaneX[Group], ExtentX);

			Radius = _mm_add_ps(Radius,	_mm_mul_ps(AbsPlaneY[Group], ExtentY));

			Radius = _mm_add_ps(Radius,	_mm_mul_ps(AbsPlaneZ[Group], ExtentZ));

			// 기존 판정식:
			//     Distance + Radius < 0
			//
			// 네 평면 중 하나라도 true이면 해당 AABB는
			// Frustum 바깥에 있으므로 즉시 탈락한다.
			const __m128 Outside = _mm_cmplt_ps(_mm_add_ps(Distance, Radius), Zero);

			if (_mm_movemask_ps(Outside) != 0)
			{
				return false;
			}
		}

		return true;
	}

	enum class EFrustumTestResult { Outside, Inside, Intersect };

	EFrustumTestResult ClassifyAABB(const FAABB& AABB, uint32& PlaneMask) const
	{
		FVector Center;
		FVector Extent;

		Center = (AABB.Max + AABB.Min) * 0.5f;
		Extent = (AABB.Max - AABB.Min) * 0.5f;

		for (int32 i = 0; i < 6; i++)
		{
			if ((PlaneMask & (1u << i)) == 0)
			{
				continue;
			}

			const FVector4& Plane = Planes[i];

			float Distance = Plane.x * Center.x + Plane.y * Center.y + Plane.z * Center.z + Plane.w;
			float ProjectedExtent = FMath::Abs(Plane.x) * Extent.x
				+ FMath::Abs(Plane.y) * Extent.y
				+ FMath::Abs(Plane.z) * Extent.z;

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


/*struct FFrustum
{
	// Left, Right, Bottom, Top, Near, Far
	FVector4 Planes[6];

	// Frustum 각 평면의 A,B,C,D 저장
	static FFrustum FromViewProjection(const FMatrix& VP)
	{
		// VP 행렬에서 k열 반환
		auto Col = [&VP](int32 k)
			{
				return FVector4(VP.M[0][k], VP.M[1][k], VP.M[2][k], VP.M[3][k]);
			};

		const FVector4 Col0 = Col(0);
		const FVector4 Col1 = Col(1);
		const FVector4 Col2 = Col(2);
		const FVector4 Col3 = Col(3);

		FFrustum F;
		F.Planes[0] = Col0 + Col3;
		F.Planes[1] = Col3 - Col0;
		F.Planes[2] = Col1 + Col3;
		F.Planes[3] = Col3 - Col1;
		F.Planes[4] = Col2;			// -w 가 0 이므로 
		F.Planes[5] = Col3 - Col2;

		return F;
	}

	enum class EFrustumTestResult { Outside, Inside, Intersect };

	EFrustumTestResult ClassifyAABB(const FAABB& AABB, uint32& PlaneMask) const
	{
		FVector Center;
		FVector Extent;

		Center = (AABB.Max + AABB.Min) * 0.5f;
		Extent = (AABB.Max - AABB.Min) * 0.5f;

		for (int32 i = 0; i < 6; i++)
		{
			if ((PlaneMask & (1u << i)) == 0)
			{
				continue;
			}

			const FVector4& Plane = Planes[i];

			float Distance = Plane.x * Center.x + Plane.y * Center.y + Plane.z * Center.z + Plane.w;
			float ProjectedExtent = FMath::Abs(Plane.x) * Extent.x
				+ FMath::Abs(Plane.y) * Extent.y
				+ FMath::Abs(Plane.z) * Extent.z;

			if (Distance < -ProjectedExtent)
				return (EFrustumTestResult::Outside);
			else if (Distance > ProjectedExtent)
				PlaneMask &= ~(1u << i);
		}
		if (PlaneMask == 0)
			return (EFrustumTestResult::Inside);

		return (EFrustumTestResult::Intersect);
	}

	inline bool Intersects(const FAABB& AABB) const
	{
		// 각 축의 중심
		__m128 Center = _mm_set_ps(1.f, (AABB.Min.z + AABB.Max.z) * 0.5f, (AABB.Min.y + AABB.Max.y) * 0.5f, (AABB.Min.x + AABB.Max.x) * 0.5f);

		// 각 축의 뻗어나가는 방향
		__m128 Extent = _mm_set_ps(0.f, (AABB.Max.z - AABB.Min.z) * 0.5f, (AABB.Max.y - AABB.Min.y) * 0.5f, (AABB.Max.x - AABB.Min.x) * 0.5f);

		for (const FVector4& P : Planes)
		{
			__m128 Plane = _mm_set_ps(P.w, P.z, P.y, P.x);
			__m128 AbsPlane = _mm_andnot_ps(_mm_set1_ps(-0.0f), Plane);	// fabs
			__m128 DistV = _mm_mul_ps(Plane, Center);
			__m128 RadV = _mm_mul_ps(AbsPlane, Extent);

			float DistArr[4], RadArr[4];
			_mm_storeu_ps(DistArr, DistV);
			_mm_storeu_ps(RadArr, RadV);
			float Dist = DistArr[0] + DistArr[1] + DistArr[2] + DistArr[3];
			float Radius = RadArr[0] + RadArr[1] + RadArr[2];

			// 이 평면의 가장 유리한 꼭짓점조차 Frustum 바깥에 있으므로 false
			if (Dist + Radius < 0.f) return false;
		}
		return true;
	}
};*/