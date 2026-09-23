#pragma once

#include "Core.h"

struct FName
{
	FName();
	FName(const char* pStr);
	FName(const FString& Name);

	int32 Compare(const FName& Other) const;
	bool operator==(const FName& Other) const;

	void ParseName(const FString& InName, FString& OutName, uint32& OutNumber) const;

	inline bool IsValid() const { return DisplayIndex != -1 && ComparisonIndex != -1; }

	FString ToString() const;

	int32 DisplayIndex;
	int32 ComparisonIndex;
	uint32 Number;
};

template <>
struct std::hash<FName>
{
	std::size_t operator()(const FName& Name) const noexcept
	{
		std::size_t h1 = std::hash<int32>{}(Name.DisplayIndex);
		std::size_t h2 = std::hash<int32>{}(Name.ComparisonIndex);
		std::size_t h3 = std::hash<uint32>{}(Name.Number);
		return h1 ^ (h2 << 1) ^ (h3 << 2);
	}
};