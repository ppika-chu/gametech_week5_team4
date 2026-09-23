#pragma once

//충돌판정

#include "Vector.h"

// 화면의 한 점에서 쏜 광선. 근평면 점(Near)과 원평면 점(Far)이 본체이고,
// Direction/Length 는 그 둘에서 미리 구해둔 값이다.
// 컴포넌트마다 다시 계산하지 않으려고 한 번만 만들어 돌려 쓴다.
struct FPickingRay
{
	FVector Near;
	FVector Far;
	FVector Direction;   // Near -> Far 방향, 정규화되어 있다
	float   Length = 0.f;// Near ~ Far 거리

	FPickingRay() = default;

	FPickingRay(const FVector& InNear, const FVector& InFar)
		: Near(InNear)
		, Far(InFar)
	{
		Direction = InFar - InNear;
		Length = Direction.Length();
		if (Length > 0.f)
		{
			Direction /= Length;
		}
	}

	// AABB 판정용. Origin 은 Near, 길이는 Length 까지다.
	FRay ToRay() const { return FRay(Near, Direction); }
};
