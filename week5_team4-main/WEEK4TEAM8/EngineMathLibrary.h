#pragma once

#include "Vector.h"
#include "Matrix.h"
#include "FQuaternion.h"
#include "MathUtility.h"
#include <functional>

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

inline bool RayIntersectsAABB(const FRay& Ray, float Distance, const FAABB& AABB)
{
	if (Distance < 0.f)
	{
		return false;
	}

	float Enter = 0.f;
	float Exit = Distance;

	for (int32 i = 0; i < 3; i++)
	{
		if (Ray.Direction[i] == 0.f)
		{
			if (Ray.Origin[i] < AABB.Min[i] || Ray.Origin[i] > AABB.Max[i])
			{
				return false;
			}

			continue;
		}

		float AxisEnter = (AABB.Min[i] - Ray.Origin[i]) / Ray.Direction[i];
		float AxisExit = (AABB.Max[i] - Ray.Origin[i]) / Ray.Direction[i];
		if (AxisEnter > AxisExit)
		{
			std::swap(AxisEnter, AxisExit);
		}

		if (AxisEnter > Enter) Enter = AxisEnter;
		if (AxisExit < Exit) Exit = AxisExit;
		if (Enter > Exit) return false;
	}

	return true;
}

