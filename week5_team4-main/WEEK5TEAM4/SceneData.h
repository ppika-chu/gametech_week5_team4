#pragma once

//#include "Json/json.hpp"

#include "Core.h"
#include "TArray.h"
#include "TMap.h"
#include "Vector.h"
#include "Rotator.h"
#include "enum.h"

namespace json
{
	class JSON;
}

struct FPrimitiveData
{
	FVector Location;
	FRotator Rotation;
	FVector Scale;
	EPrimitive PrimitiveType;

	FPrimitiveData();
	FPrimitiveData(json::JSON);

	json::JSON ToJson() const;
	FString ToJsonString() const;
};

struct FSceneData
{
	uint32 Version;
	uint32 NextUUID;
	TMap<uint32, FPrimitiveData> Primitives;

	FSceneData();
	FSceneData(json::JSON);

	json::JSON ToJson() const;
	FString ToJsonString() const;
};


