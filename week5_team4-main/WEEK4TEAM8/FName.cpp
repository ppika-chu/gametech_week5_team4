#include "FName.h"
#include "TArray.h"
#include "TMap.h"
#include <format>
#include <limits>

struct FNamePool
{
	using HashValue64 = uint64;

	struct FNameBlock
	{
		int32 StartIndex;
		int32 Count;
	};

	struct FNameEntry
	{
		FNameBlock ComparisonBlock;
		TArray<FNameBlock> DisplayBlocks;
	};

	HashValue64 HashString(const FString& Str)
	{
		// FNV-1a hash algorithm
		uint64 Hash = 14695981039346656037ull;

		for (unsigned char Ch : Str)
		{
			Hash ^= Ch;
			Hash *= 1099511628211ull;
		}

		return Hash;
	}

	FNameBlock StoreString(const FString& Str)
	{
		FNameBlock Block;
		Block.StartIndex = NameStream.Num();
		Block.Count = Str.Len() + 1;
		NameStream.SetNum(NameStream.Num() + Block.Count);

		memcpy(&NameStream[Block.StartIndex], Str.CStr(), Str.Len() + 1);

		return Block;
	}

	int32 FindComparisonIndex(const FString& ComparisonName, HashValue64 Hash)
	{
		TArray<int32>* EntryIndices = ComparisonNameMap.Find(Hash);
		if (EntryIndices)
		{
			for (int32 Index : *EntryIndices)
			{
				const FNameEntry& Entry = NameEntries[Index];
				if (strncmp(&NameStream[Entry.ComparisonBlock.StartIndex], ComparisonName.CStr(), Entry.ComparisonBlock.Count) == 0)
				{
					return Index;
				}
			}
		}
		
		return -1;
	}

	int32 FindDisplayIndex(int32 ComparisonIndex, const FString& DisplayName)
	{
		const FNameEntry& Entry = NameEntries[ComparisonIndex];

		for (int32 Index = 0; Index < Entry.DisplayBlocks.Num(); ++Index)
		{
			if (strncmp(&NameStream[Entry.DisplayBlocks[Index].StartIndex], DisplayName.CStr(), Entry.DisplayBlocks[Index].Count) == 0)
			{
				return Index;
			}
		}

		return -1;
	}

	int32 StoreComparisionName(const FString& ComparisonName)
	{
		HashValue64 Hash = HashString(ComparisonName);

		int32 ComparisonIndex = FindComparisonIndex(ComparisonName, Hash);
		if (ComparisonIndex == -1)
		{
			FNameBlock ComparisonBlock = StoreString(ComparisonName);
			FNameEntry NewEntry;
			NewEntry.ComparisonBlock = ComparisonBlock;
			ComparisonIndex = NameEntries.Add(NewEntry);

			TArray<int32>* EntryIndices = ComparisonNameMap.Find(Hash);
			if (!EntryIndices)
			{
				ComparisonNameMap.Add(Hash, TArray<int32>());
				EntryIndices = ComparisonNameMap.Find(Hash);
			}

			EntryIndices->Add(ComparisonIndex);
		}
		
		return ComparisonIndex;
	}

	int32 StoreDisplayName(int32 ComparisonIndex, const FString& DisplayName)
	{
		int32 DisplayIndex = FindDisplayIndex(ComparisonIndex, DisplayName);
		if (DisplayIndex == -1)
		{
			FNameEntry& Entry = NameEntries[ComparisonIndex];
			FNameBlock DisplayBlock = StoreString(DisplayName);
			DisplayIndex = Entry.DisplayBlocks.Add(DisplayBlock);
		}

		return DisplayIndex;
	}

	friend class FName;

	TMap<HashValue64, TArray<int32>> ComparisonNameMap;
	TArray<FNameEntry> NameEntries;
	TArray<char> NameStream;
};

static FNamePool& GetNamePool()
{
	//NamePool이 가장 마지막까지 살아있게 하기위해 동적할당 후 누수를 유도(프로그램 종료니까 안전)
	static FNamePool* NamePool = new FNamePool();
	return *NamePool;
}

FName::FName()
	: DisplayIndex(-1)
	, ComparisonIndex(-1)
	, Number(0)
{
}

FName::FName(const char* pStr)
	: FName(FString(pStr))
{
}

FName::FName(const FString& Name)
{
	FString ParsedName;
	ParseName(Name, ParsedName, Number);

	FString ComparisonName = ParsedName.ToLower();
	
	FNamePool& NamePool = GetNamePool();
	ComparisonIndex = NamePool.StoreComparisionName(ComparisonName);
	DisplayIndex = NamePool.StoreDisplayName(ComparisonIndex, ParsedName);
}

int32 FName::Compare(const FName& Other) const
{
	if (ComparisonIndex != Other.ComparisonIndex)
	{
		return ComparisonIndex - Other.ComparisonIndex;
	}
	else
	{
		return Number - Other.Number;
	}
}

bool FName::operator==(const FName& Other) const
{
	return ComparisonIndex == Other.ComparisonIndex && Number == Other.Number;
}

void FName::ParseName(const FString& InName, FString& OutName, uint32& OutNumber) const
{
    OutName = InName;
    OutNumber = 0;

    const char* Text = InName.CStr();
    const int32 Length = InName.Len();

    int32 NumberStart = Length;
    while (NumberStart > 0 && std::isdigit(Text[NumberStart - 1]))
    {
        --NumberStart;
    }

    if (NumberStart == Length)
    {
        return;
    }

	if (Length - NumberStart != 1 && Text[NumberStart] == '0')
	{
		return;
	}

	FString NumberStr = InName.Mid(NumberStart, Length - NumberStart);

	uint32 ParsedNumber = 0;
	for (int32 i = NumberStart; i < Length; ++i)
	{
		uint32 Digit = static_cast<uint32>(Text[i] - '0');
		if (ParsedNumber > ((UINT32_MAX - 1) - Digit) / 10)
		{
			OutName = InName;
			OutNumber = 0;
			return;
		}

		ParsedNumber = ParsedNumber * 10 + Digit;
	}

    OutName = InName.Left(NumberStart);
    OutNumber = ParsedNumber + 1;
}

FString FName::ToString() const
{
	if (!IsValid())
	{
		return FString();
	}

	FNamePool& NamePool = GetNamePool();

	const FNamePool::FNameEntry& Entry = NamePool.NameEntries[ComparisonIndex];
	const FNamePool::FNameBlock& DisplayBlock = Entry.DisplayBlocks[DisplayIndex];

	return std::format("{}{}", &NamePool.NameStream[DisplayBlock.StartIndex], Number > 0 ? std::to_string(Number - 1) : "");
}
