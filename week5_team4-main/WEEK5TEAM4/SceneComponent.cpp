#include "SceneComponent.h"

#include <format>

#include "Transform.h"
#include "JsonUtil.h"

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
	OnTransformChanged();
}

FVector USceneComponent::GetRelativeLocation() const
{
	return mRelativeLocation;
}

void USceneComponent::SetRelativeLocation(FVector location)
{
	if (mRelativeLocation.x == location.x
		&& mRelativeLocation.y == location.y
		&& mRelativeLocation.z == location.z)
	{
		return;
	}

	
	mRelativeLocation = location;
	OnTransformChanged();
	MarkTransformDirty();
}

FRotator USceneComponent::GetRelativeRotation() const
{
	return mRelativeRotation;
}

void USceneComponent::SetRelativeRotation(FRotator rotation)
{
	if (mRelativeRotation.Pitch == rotation.Pitch
		&& mRelativeRotation.Yaw == rotation.Yaw
		&& mRelativeRotation.Roll == rotation.Roll)
	{
		return;
	}

	mRelativeRotation = rotation;
	OnTransformChanged();
	MarkTransformDirty();
}

FVector USceneComponent::GetRelativeScale3D() const
{
	return mRelativeScale3D;
}

void USceneComponent::SetRelativeScale3D(FVector scale)
{
	if (mRelativeScale3D.x == scale.x
		&& mRelativeScale3D.y == scale.y
		&& mRelativeScale3D.z == scale.z)
	{
		return;
	}

	mRelativeScale3D = scale;
	OnTransformChanged();
	MarkTransformDirty();
}

FTransform USceneComponent::GetTransformMatrix() const
{
	return FTransform(mRelativeLocation, mRelativeRotation, mRelativeScale3D);
}

const FMatrix& USceneComponent::GetWorldMatrix() const
{
	if(bWorldMatrixDirty)
	{
		const FTransform Transform(mRelativeLocation, mRelativeRotation, mRelativeScale3D);
		mCachedWorldMatrix = Transform.MakeMatrix();
		bWorldMatrixDirty = false;
	}
	return mCachedWorldMatrix;
}


void USceneComponent::MarkTransformDirty()
{
	bWorldMatrixDirty = true;
}