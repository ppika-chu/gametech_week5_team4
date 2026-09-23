#pragma once

#include "Core.h"

class UEngineStatics
{
public:
	static uint32 GenerateUUID();

	// Used for scene load / save 
	static uint32 GetNextUUID();
	static void SetNextUUID(uint32 nextUUID);

	inline static uint32 sTotalAllocationCount = 0;
	inline static uint32 sTotalAllocationBytes = 0;

private:
	static uint32 msNextUUID;
};

inline uint32 UEngineStatics::GenerateUUID()
{
	return msNextUUID++;
}

inline uint32 UEngineStatics::GetNextUUID()
{
	return msNextUUID;
}

inline void UEngineStatics::SetNextUUID(uint32 nextUUID)
{
	msNextUUID = nextUUID;
}
