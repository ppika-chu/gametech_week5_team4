#pragma once

#include <cmath>
#include "Core.h"

constexpr float PI = 3.1415926535897932f;
constexpr double DOUBLE_PI = 3.1415926535897932;
constexpr float SMALL_NUMBER = 1.e-37f;
constexpr float KINDA_SMALL_NUMBER = 1.e-4f;

// [[nodiscard]] -> 반환값을 버리면 경고 표시
// FORCEINLINE -> 인라인 강제(한줄짜리 함수에 사용)

//플랫폼과 관계없이 똑같은 함수
struct FGenericPlatformMath
{
	[[nodiscard]] static FORCEINLINE float Sin(float Value) { return sinf(Value); }
	[[nodiscard]] static FORCEINLINE double Sin(double Value) { return sin(Value); }

	[[nodiscard]] static FORCEINLINE float Cos(float Value) { return cosf(Value); }
	[[nodiscard]] static FORCEINLINE double Cos(double Value) { return cos(Value); }
	
	[[nodiscard]] static FORCEINLINE float Tan(float Value) { return tanf(Value); }
	[[nodiscard]] static FORCEINLINE double Tan(double Value) { return tan(Value); }

	template< class T >
	[[nodiscard]] static constexpr FORCEINLINE T Abs(const T A)
	{
		return (A < (T)0) ? (T)-A : A;
	}


	[[nodiscard]] static float Abs(const float _x) { return fabsf(_x); }
	[[nodiscard]] static double Abs(const double _x) { return fabs(_x); }
	[[nodiscard]] static float Fmod(const float _x, const float _y);

	[[nodiscard]] static FORCEINLINE float Sqrt(float Value) { return sqrtf(Value); }
	[[nodiscard]] static FORCEINLINE double Sqrt(double Value) { return sqrt(Value); }

	[[nodiscard]] static FORCEINLINE float Pow(float A, float B) { return powf(A, B); }
	[[nodiscard]] static FORCEINLINE double Pow(double A, double B) { return pow(A, B); }

	template <typename T>
	[[nodiscard]] static constexpr FORCEINLINE T Max(T A, T B)
	{
		return (B < A) ? A : B;
	}

	template <typename T>
	[[nodiscard]] static constexpr FORCEINLINE T Min(T A, T B)
	{
		return (A < B) ? A : B;
	}
};

typedef FGenericPlatformMath FPlatformMath;

//엔진에 사용되는 알고리즘
struct FMath : FPlatformMath
{
	template<typename T>
	static inline void sincos(T& sin, T& cos, T value)
	{
		sin = std::sin(value);
		cos = std::cos(value);
	}

	/** Clamps X to be between Min and Max, inclusive */
	template< class T >
	[[nodiscard]] static constexpr FORCEINLINE T Clamp(const T X, const T MinValue, const T MaxValue)
	{
		return Max(Min(X, MaxValue), MinValue);
	}

	[[nodiscard]] static constexpr FORCEINLINE float Clamp(const float X, const float Min, const float Max) { return Clamp<float>(X, Min, Max); }
	[[nodiscard]] static constexpr FORCEINLINE double Clamp(const double X, const double Min, const double Max) { return Clamp<double>(X, Min, Max); }
	[[nodiscard]] static constexpr FORCEINLINE int64 Clamp(const int64 X, const int32 Min, const int32 Max) { return Clamp<int64>(X, Min, Max); }

	[[nodiscard]] static FORCEINLINE float Exp(float Value) { return expf(Value); }

	static constexpr FORCEINLINE float RadiansToDegrees(float const& RadVal) { return RadVal * (180.f / PI); }
	static constexpr FORCEINLINE double RadiansToDegrees(double const& RadVal) { return RadVal * (180.0 / DOUBLE_PI); }
	static constexpr FORCEINLINE float DegreesToRadians(float const& DegVal) { return DegVal * (PI / 180.f); }
	static constexpr FORCEINLINE double DegreesToRadians(double const& DegVal) { return DegVal * (DOUBLE_PI / 180.0); }
};
