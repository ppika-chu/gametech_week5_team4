
#include "Object.h"
#include "EngineStatics.h"
#include "Json/json.hpp"

TSparseArray<UObject*> UObject::GUObjectArray;
TMap<const FClassInfo*, TArray<uint32>> UObject::GUObjectMap;

UObject* FClassInfo::CreateInstance() const
{
	if (Constructor)
	{
		return Constructor();
	}
	return nullptr;
}

bool FClassInfo::IsChildOf(const FClassInfo* other) const
{
	const FClassInfo* currentClass = this;
	while (currentClass)
	{
		if (currentClass == other)
		{
			return true;
		}
		currentClass = currentClass->SuperClass;
	}
	return false;
}

UObject::UObject()
	: UUID(0)
	, InternalIndex(0)
	, ObjectMapIndex(0)
{
	GUObjectRevision++;
}

UObject::~UObject()
{
	GUObjectRevision++;
}

void UObject::Initialize()
{
}

const FClassInfo* UObject::GetStaticClass()
{
	static FClassInfo classInstance = FClassInfo(
		"UObject",
		nullptr,
		[]() -> UObject* { return new UObject(); }
	);
	return &classInstance;
}

void UObject::SerializeClass(json::JSON& outJson) const
{
	outJson["ClassName"] = GetClass()->Name;

	json::JSON propertiesJson = json::JSON::Make(json::JSON::Class::Object);
	propertiesJson["UUID"] = UUID;
	outJson["Properties"] = propertiesJson;
}

void UObject::DeserializeClass(const json::JSON& inJson)
{
	if (!inJson.hasKey("Properties") || inJson.at("Properties").JSONType() != json::JSON::Class::Object)
	{
		throw std::runtime_error("Invalid JSON format for Properties");
	}
	const json::JSON& propertiesJson = inJson.at("Properties");

	if (!propertiesJson.hasKey("UUID") || propertiesJson.at("UUID").JSONType() != json::JSON::Class::Integral)
	{
		throw std::runtime_error("Invalid JSON format for UUID");
	}

	UUID = propertiesJson.at("UUID").ToInt();
}

bool UObject::IsA(const FClassInfo* classInfo) const
{
	return GetClass()->IsChildOf(classInfo);
}

UObject* UObject::GetObjectByUUID(int32 uuid)
{
	for (const auto& object : GUObjectArray)
	{
		if (object && object->UUID == uuid)
		{
			return object;
		}
	}
	return nullptr;
}

UObject* UObject::GetObjectByInternalIndex(uint32 internalIndex)
{
	if (GUObjectArray.IsValidIndex(internalIndex))
	{
		return GUObjectArray[internalIndex];
	}
	return nullptr;
}
