#pragma once

#include "Vector.h"

struct FQuaternion
{
	union
	{
		struct
		{
			float X, Y, Z, W;
		};
		struct
		{
			FVector V;
			float S;
		};
		float Data[4];
	};

	FQuaternion();
	FQuaternion(float InX, float InY, float InZ, float InW);
	FQuaternion(FVector Axis, float Angle);

	void Normalize();
	FQuaternion Inverse() const;

	FQuaternion operator*(const FQuaternion& Other) const;
	float& operator[](int Index);
};