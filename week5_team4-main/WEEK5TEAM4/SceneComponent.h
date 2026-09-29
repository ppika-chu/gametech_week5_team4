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

	//실제 행렬을 반환하는 캐시 함수 
	const FMatrix& GetWorldMatrix() const;

protected:
	virtual void OnTransformChanged() {}

private:
	FVector mRelativeLocation;
	FRotator mRelativeRotation;
	FVector mRelativeScale3D;

	void MarkTransformDirty();

	// 실제 행렬을 캐싱하는 변수
	mutable FMatrix mCachedWorldMatrix = FMatrix::Identity;

	//다시 계산해야 하는지 여부를 나타내는 플래그
	mutable bool bWorldMatrixDirty = true;
};

