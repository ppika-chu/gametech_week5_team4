#include "SceneData.h"

#include "Json/json.hpp"

#include "JsonUtil.h"

FPrimitiveData::FPrimitiveData()
	: Location(0.f, 0.f, 0.f)
	, Rotation(0.f, 0.f, 0.f)
	, Scale(1.f, 1.f, 1.f)
	, PrimitiveType(EPrimitive::EP_Sphere)
{
}

FPrimitiveData::FPrimitiveData(json::JSON json)
{
	if (!json.hasKey("Location") || !json.hasKey("Rotation") || !json.hasKey("Scale") || !json.hasKey("PrimitiveType"))
	{
		throw std::runtime_error("Invalid JSON format for FPrimitiveData");
		return;
	}

	Location = JsonUtils::FromJson<FVector>(json["Location"]);
	Rotation = JsonUtils::FromJson<FRotator>(json["Rotation"]);
	Scale = JsonUtils::FromJson<FVector>(json["Scale"]);
	PrimitiveType = JsonUtils::FromJson<EPrimitive>(json["PrimitiveType"]);
}

json::JSON FPrimitiveData::ToJson() const
{
	json::JSON json;
	json["Location"] = JsonUtils::ToJson(Location);
	json["Rotation"] = JsonUtils::ToJson(Rotation);
	json["Scale"] = JsonUtils::ToJson(Scale);
	json["PrimitiveType"] = JsonUtils::ToJson(PrimitiveType);
	return json;
}

FString FPrimitiveData::ToJsonString() const
{
	return FString(ToJson().dump());
}

FSceneData::FSceneData()
	: Version(0)
	, NextUUID(0)
{
}

FSceneData::FSceneData(json::JSON json)
{
	if (!json.hasKey("Version") || !json.hasKey("NextUUID") || !json.hasKey("Primitives"))
	{
		throw std::runtime_error("Invalid JSON format for FSceneData");
		return;
	}

	Version = json["Version"].ToInt();
	NextUUID = json["NextUUID"].ToInt();

	for (const auto& [key, value] : json["Primitives"].ObjectRange())
	{
		uint32 UUID = std::stoul(key);
		FPrimitiveData PrimitiveData(value);
		Primitives.Add(UUID, PrimitiveData);
	}
}

json::JSON FSceneData::ToJson() const
{
	json::JSON json;
	json["Version"] = Version;
	json["NextUUID"] = NextUUID;
	json::JSON primitivesJson = json::JSON::Make(json::JSON::Class::Object);
	for (const auto& [UUID, PrimitiveData] : Primitives)
	{
		primitivesJson[std::to_string(UUID)] = PrimitiveData.ToJson();
	}
	json["Primitives"] = primitivesJson;
	return json;
}

FString FSceneData::ToJsonString() const
{
	return FString(ToJson().dump());
}
