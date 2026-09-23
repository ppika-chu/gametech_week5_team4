#pragma once

#include "MathUtility.h"

struct FVector2
{
	union
	{

		struct { float X, Y; };
		float v[2];
	};
	
	FVector2() : X(0), Y(0) {}
	FVector2(float InX, float InY) : X(InX), Y(InY) {}

	float LengthSquared() const { return X * X + Y * Y; }
	float Length() const { return FMath::Sqrt(LengthSquared()); }
	void Normalize()
	{
		const float Size = Length();
		if (Size > SMALL_NUMBER) { X /= Size; Y /= Size; }
	}

	FVector2 operator+(const FVector2& Other) const { return { X + Other.X, Y + Other.Y }; }
	FVector2 operator-(const FVector2& Other) const { return { X - Other.X, Y - Other.Y }; }
	FVector2 operator*(const FVector2& Other) const { return { X * Other.X, Y * Other.Y }; }
	FVector2 operator/(const FVector2& Other) const { return { X / Other.X, Y / Other.Y }; }
	FVector2 operator/(float Scalar) const { return { X / Scalar, Y / Scalar }; }
	FVector2 operator*(float Scalar) const { return { X * Scalar, Y * Scalar }; }
	FVector2& operator*=(const FVector2& Other) { X *= Other.X; Y *= Other.Y; return *this; }
	FVector2& operator/=(float Scalar) { X /= Scalar; Y /= Scalar; return *this; }

	float& operator[](int32 Index) { return v[Index]; }
	const float& operator[](int32 Index) const { return v[Index]; }

	static float LengthSquared(const FVector2& A, const FVector2& B) 
	{
		float dx = A.X - B.X;
		float dy = A.Y - B.Y;
		return dx * dx + dy * dy;
	}

	static float Dot(const FVector2& A, const FVector2& B)
	{
		return A.X * B.X + A.Y * B.Y;
	}
};

typedef struct FVector
{
	union
	{
		struct { float x, y, z; };
		float v[3];
	};

    FVector() : x(0), y(0), z(0) {}

	FVector(float n) : x(n), y(n), z(n){}
    FVector(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    const FVector operator-(const FVector& Others) const
    {
        return FVector(x - Others.x, y - Others.y, z - Others.z);
    }

    void operator+=(const FVector& Others)
    {
        x += Others.x;
        y += Others.y;
        z += Others.z;
    }

    void operator-=(const FVector& Others)
    {
        x -= Others.x;
        y -= Others.y;
        z -= Others.z;
    }

	FVector& operator*=(float Scalar)
	{
		x *= Scalar;
		y *= Scalar;
		z *= Scalar;
		return *this;
	}

	void operator/=(float Scalar)
	{
		x /= Scalar;
		y /= Scalar;
		z /= Scalar;
	}

	FVector operator-() const
	{
		return FVector(-x, -y, -z);
	}

	float& operator[](int32 Index)
	{
		return v[Index];
	}

	const float& operator[](int32 Index) const
	{
		return v[Index];
	}

	//내적
    inline static float dot(const FVector& A, const FVector& B)
    {
        return A.x * B.x + A.y * B.y + A.z * B.z;
    }

	//외적
	inline static FVector cross(const FVector& A, const FVector& B)
	{
		return	FVector(A.y * B.z - A.z * B.y, A.z * B.x - A.x * B.z, A.x * B.y - A.y * B.x);
	}

	float Length() const { return FMath::Sqrt(x * x + y * y + z * z); }
	float LengthSquared() const { return x * x + y * y + z * z; }

	void Normalize()
	{
		float len = Length();
		x /= len;
		y /= len;
		z /= len;
	}

	inline bool IsNearlyZero(float Tolerance = KINDA_SMALL_NUMBER) const
	{
		return LengthSquared() < Tolerance;
	}

	static float LengthSquared(const FVector& A, const FVector& B)
	{
		return (A - B).LengthSquared();
	}
	
} FVector3;

inline const FVector operator*(const FVector& v, float f)
{
    return FVector(v.x * f, v.y * f, v.z * f);
}

inline const FVector operator*(float f, const FVector& v)
{
    return FVector(v.x * f, v.y * f, v.z * f);
}

inline FVector operator+(const FVector& A, const FVector& B)
{
	return FVector(A.x + B.x, A.y + B.y, A.z + B.z);
}

//Vector 4
typedef struct FVector4
{
	union
	{
		struct { float x, y, z, w; };
		float v[4];
	};
	
	FVector4() : x(0), y(0), z(0), w(0) {}
	FVector4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
	FVector4(const FVector3& v, float _w) : x(v.x), y(v.y), z(v.z), w(_w) {}

	FVector4 operator+(const FVector4& Others) const
	{
		return FVector4(x + Others.x, y + Others.y, z + Others.z, w + Others.w);
	}

	FVector4 operator-(const FVector4& Others) const
	{
		return FVector4(x - Others.x, y - Others.y, z - Others.z, w - Others.w);
	}

	FVector4 operator*(float Scalar) const
	{
		return FVector4(x * Scalar, y * Scalar, z * Scalar, w * Scalar);
	}

	void operator+=(const FVector4& Others)
	{
		x += Others.x;
		y += Others.y;
		z += Others.z;
		w += Others.w;
	}

	void operator-=(const FVector4& Others)
	{
		x -= Others.x;
		y -= Others.y;
		z -= Others.z;
		w -= Others.w;
	}

	void operator*=(float Scalar)
	{
		x *= Scalar;
		y *= Scalar;
		z *= Scalar;
		w *= Scalar;
	}

	//내적
	inline static float dot(const FVector4& A, const FVector4& B)
	{
		return A.x * B.x + A.y * B.y + A.z * B.z + A.w * B.w;
	}

	//4차원에는 외적이 없다.

	float Length() const { return FMath::Sqrt(x * x + y * y + z * z + w * w); }

	FVector3 ToVec3() const { return FVector3(x, y, z); }
} FVector4;

struct FRay
{
	FVector Origin;
	FVector Direction;

	FRay() = default;
	FRay(const FVector& InOrigin, const FVector& InDirection)
		: Origin(InOrigin)
		, Direction(InDirection)
	{
	}
};

struct FRect
{
	union
	{
		struct { float X, Y, Width, Height; };
		float v[4];
	};
	
	FRect() : X(0), Y(0), Width(0), Height(0) {}
	FRect(float InX, float InY, float InWidth, float InHeight)
		: X(InX), Y(InY), Width(InWidth), Height(InHeight) {
	}
};
