#pragma once

#include "ActorComponent.h"
#include "GraphicsManager.h"

#include "Vector.h"

class FTransform;

class USceneComponent : public UActorComponent
{
	REFLECT_CLASS(USceneComponent, UActorComponent)
public:
	USceneComponent() = default;

	void Initialize(FVector location, FRotator rotation, FVector scale3D);
	virtual ~USceneComponent();

	virtual void SerializeClass(json::JSON& outJson) const override;
	virtual void DeserializeClass(const json::JSON& inJson) override;

	FVector GetRelativeLocation() const;
	void SetRelativeLocation(FVector location);

	FRotator GetRelativeRotation() const;
	void SetRelativeRotation(FRotator rotation);

	FVector GetRelativeScale3D() const;
	void SetRelativeScale3D(FVector scale);

	FTransform GetTransformMatrix() const;

private:
	FVector mRelativeLocation;
	FRotator mRelativeRotation;
	FVector mRelativeScale3D;
};

