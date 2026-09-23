#pragma once

#include "Core.h"

// 에디터에서 켜고 끄는 표시 옵션.
enum class EShowFlag : uint64
{
	None      = 0,
	WorldAxis = 1ull << 0,
	UUIDText  = 1ull << 1,
	Grid      = 1ull << 2,
	Primitive = 1ull << 3,
};

inline constexpr EShowFlag operator|(EShowFlag a, EShowFlag b)
{
	return static_cast<EShowFlag>(static_cast<uint64>(a) | static_cast<uint64>(b));
}

struct FShowFlagInfo
{
	EShowFlag Flag;
	const char* Name;
};

inline constexpr FShowFlagInfo GShowFlagInfos[] =
{
	{ EShowFlag::UUIDText,  "UUID"       },
	{ EShowFlag::Grid,      "Grid"      },
	{ EShowFlag::Primitive, "Primitive" }
};

class FShowFlags
{
public:
	static FShowFlags& Get()
	{
		static FShowFlags Instance;
		return Instance;
	}

	bool IsEnabled(EShowFlag flag) const
	{
		return (mFlags & static_cast<uint64>(flag)) != 0;
	}

	void SetEnabled(EShowFlag flag, bool bEnabled)
	{
		if (bEnabled)
		{
			mFlags |= static_cast<uint64>(flag);
		}
		else
		{
			mFlags &= ~static_cast<uint64>(flag);
		}
	}

	void Toggle(EShowFlag flag)
	{
		mFlags ^= static_cast<uint64>(flag);
	}

private:
	FShowFlags() = default;

	static constexpr EShowFlag DEFAULT_FLAGS =
		EShowFlag::WorldAxis | EShowFlag::UUIDText | EShowFlag::Grid | EShowFlag::Primitive;

	uint64 mFlags = static_cast<uint64>(DEFAULT_FLAGS);
};
