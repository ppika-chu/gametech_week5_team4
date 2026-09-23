#pragma once
#include <cmath>
#include "Vector.h"
#include "MathUtility.h"

struct TRotator
{
	float Pitch, Yaw, Roll;

	TRotator() = default;

	TRotator(float InPitch, float InYaw, float InRoll)
		: Pitch(InPitch), Yaw(InYaw), Roll(InRoll) {}

	//static const TRotator ZeroRotator = { 0, 0, 0 };

	static TRotator FromDirection(const FVector& Direction)
	{
		float Yaw = std::atan2(Direction.y, Direction.x);
		float Pitch = std::atan2(Direction.z, std::sqrt(Direction.x * Direction.x
													  + Direction.y * Direction.y));

		return TRotator(Pitch * 180 / PI, Yaw * 180 / PI, 0.0f);
	}

	// From 위치에서 To 위치를 바라보는 회전값.
	static TRotator LookAt(const FVector& From, const FVector& To)
	{
		return FromDirection(To - From);
	}

	[[nodiscard]] FVector Vector() const
	{
		const float PitchNoWinding = FMath::Fmod(Pitch, 360.0f);
		const float YawNoWinding = FMath::Fmod(Yaw, 360.0f);

		float CP, SP, CY, SY;
		FMath::sincos<float>(SP, CP, FMath::DegreesToRadians(PitchNoWinding));
		FMath::sincos<float>(SY, CY, FMath::DegreesToRadians(YawNoWinding));
		FVector V = FVector(CP * CY, CP * SY, SP);

		return V;
	}
};

using FRotator = TRotator;
