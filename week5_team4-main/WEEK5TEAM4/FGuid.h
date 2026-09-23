#pragma once

#include "Core.h"
#include "Json/json.hpp"
#include <random>
#include <format>

struct FGuid
{
	uint32 A;
	uint32 B;
	uint32 C;
	uint32 D;

	FGuid() : A(0), B(0), C(0), D(0) {}
	FGuid(uint32 InA, uint32 InB, uint32 InC, uint32 InD) : A(InA), B(InB), C(InC), D(InD) {}

	inline bool IsValid() const
	{
		return A != 0 || B != 0 || C != 0 || D != 0;
	}

	bool operator==(const FGuid& Other) const
	{
		return A == Other.A && B == Other.B && C == Other.C && D == Other.D;
	}

	bool operator!=(const FGuid& Other) const
	{
		return !(*this == Other);
	}

	FString ToString() const
	{
		return std::format("{}-{}-{}-{}", A, B, C, D);
	}
	
	static FGuid NewGuid();
};

template <>
struct std::hash<FGuid>
{
	std::size_t operator()(const FGuid& Guid) const noexcept
	{
		std::size_t h1 = std::hash<uint32>{}(Guid.A);
		std::size_t h2 = std::hash<uint32>{}(Guid.B);
		std::size_t h3 = std::hash<uint32>{}(Guid.C);
		std::size_t h4 = std::hash<uint32>{}(Guid.D);

		return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
	}
};