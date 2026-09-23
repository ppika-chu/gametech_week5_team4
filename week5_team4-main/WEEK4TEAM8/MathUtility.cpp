#include "MathUtility.h"

float FGenericPlatformMath::Fmod(const float _x, const float _y)
{
	const float AbsY = FGenericPlatformMath::Abs(_y);
	if (AbsY <= SMALL_NUMBER) // Note: this constant should match that used by VectorMod() implementations
	{
		//FmodReportError(X, Y);
		return 0.0;
	}

	return fmodf(_x, _y);
}
