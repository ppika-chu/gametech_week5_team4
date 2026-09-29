#include "SceneComponent.h"

#include <format>

#include "Transform.h"
#include "JsonUtil.h"
#include "OptimizationFlags.h"

void USceneComponent::Initialize(FVector location, FRotator rotation, FVector scale3D)
{
	UActorComponent::Initialize();

	mRelativeLocation = location;
	mRelativeRotation = rotation;
	mRelativeScale3D = scale3D;
	MarkTransformDirty();
}

USceneComponent::~USceneComponent()
{
}

void USceneComponent::SerializeClass(json::JSON& outJson) const
{
	UActorComponent::SerializeClass(outJson);
	outJson["Properties"]["mRelativeLocation"] = JsonUtils::ToJson(mRelativeLocation);
	outJson["Properties"]["mRelativeRotation"] = JsonUtils::ToJson(mRelativeRotation);
	outJson["Properties"]["mRelativeScale3D"] = JsonUtils::ToJson(mRelativeScale3D);
}

void USceneComponent::DeserializeClass(const json::JSON& inJson)
{
	UActorComponent::DeserializeClass(inJson);

	const json::JSON& propertiesJson = inJson.at("Properties");

	if (!propertiesJson.hasKey("mRelativeLocation")
		|| propertiesJson.at("mRelativeLocation").JSONType() != json::JSON::Class::Array
		|| propertiesJson.at("mRelativeLocation").length() != 3)
	{
		throw std::runtime_error(std::format("{}: mRelativeLocation property requires an array of length 3", GetClass()->Name));
	}

	if (!propertiesJson.hasKey("mRelativeRotation")
		|| propertiesJson.at("mRelativeRotation").JSONType() != json::JSON::Class::Array
		|| propertiesJson.at("mRelativeRotation").length() != 3)
	{
		throw std::runtime_error(std::format("{}: mRelativeRotation property requires an array of length 3", GetClass()->Name));
	}

	if (!propertiesJson.hasKey("mRelativeScale3D")
		|| propertiesJson.at("mRelativeScale3D").JSONType() != json::JSON::Class::Array
		|| propertiesJson.at("mRelativeScale3D").length() != 3)
	{
		throw std::runtime_error(std::format("{}: mRelativeScale3D property requires an array of length 3", GetClass()->Name));
	}

	mRelativeLocation = JsonUtils::FromJson<FVector>(propertiesJson.at("mRelativeLocation"));
	mRelativeRotation = JsonUtils::FromJson<FRotator>(propertiesJson.at("mRelativeRotation"));
	mRelativeScale3D = JsonUtils::FromJson<FVector>(propertiesJson.at("mRelativeScale3D"));
	
	MarkTransformDirty();
}

FVector USceneComponent::GetRelativeLocation() const
{
	return mRelativeLocation;
}

void USceneComponent::SetRelativeLocation(FVector location)
{
	mRelativeLocation = location;
	MarkTransformDirty();
}

FRotator USceneComponent::GetRelativeRotation() const
{
	return mRelativeRotation;
}

void USceneComponent::SetRelativeRotation(FRotator rotation)
{
	mRelativeRotation = rotation;
	MarkTransformDirty();
}

FVector USceneComponent::GetRelativeScale3D() const
{
	return mRelativeScale3D;
}

void USceneComponent::SetRelativeScale3D(FVector scale)
{
	mRelativeScale3D = scale;
	MarkTransformDirty();
}

FTransform USceneComponent::GetTransformMatrix() const
{
	return FTransform(mRelativeLocation, mRelativeRotation, mRelativeScale3D);
}

const FMatrix& USceneComponent::GetCacheWorldMatrix() const
{	
	// 캐싱을 끄면 매번 다시 계산한다 (비교 측정용).
	if (mbMatrixDirty || !IsOptEnabled(EOptFlag::TransformCache))
	{
		mCacheWorldMatrix = GetTransformMatrix().MakeMatrix();
		mbMatrixDirty = false;
	}
	return mCacheWorldMatrix;
}