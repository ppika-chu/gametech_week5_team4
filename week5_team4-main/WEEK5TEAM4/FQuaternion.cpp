#include "FQuaternion.h"

FQuaternion::FQuaternion()
	: X(0)
	, Y(0)
	, Z(0)
	, W(1)
{
}

FQuaternion::FQuaternion(float InX, float InY, float InZ, float InW)
	: X(InX)
	, Y(InY)
	, Z(InZ)
	, W(InW)
{
}

FQuaternion::FQuaternion(FVector Axis, float Angle)
{
	float HalfAngle = Angle * 0.5f;
	float SinHalfAngle = sin(HalfAngle);
	X = Axis.x * SinHalfAngle;
	Y = Axis.y * SinHalfAngle;
	Z = Axis.z * SinHalfAngle;
	W = cos(HalfAngle);
}

void FQuaternion::Normalize()
{
	float Length = sqrt(X * X + Y * Y + Z * Z + W * W);
	if (Length > 0.0f)
	{
		X /= Length;
		Y /= Length;
		Z /= Length;
		W /= Length;
	}
}

FQuaternion FQuaternion::Inverse() const
{
	return FQuaternion(-X, -Y, -Z, W);
}

FQuaternion FQuaternion::operator*(const FQuaternion& Other) const
{
	FVector Axis = Other.V * W + V * Other.S + FVector::cross(V, Other.V);
	float Scalar = S * Other.S - FVector::dot(V, Other.V);
	return FQuaternion(Axis.x, Axis.y, Axis.z, Scalar);
}

float& FQuaternion::operator[](int Index)
{
	return Data[Index];
}
