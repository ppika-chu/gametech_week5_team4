#pragma once

#include "ActorComponent.h"
#include "GraphicsManager.h"

#include "Vector.h"

class FTransform;
struct FMatrix;

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
	const FMatrix& GetCacheWorldMatrix() const;

protected:
	void MarkWorldAABBDirty() { mbWorldAABBDirty = true; }
	bool IsWorldAABBDirty() const { return mbWorldAABBDirty; }
	void SetCacheWorldAABB(const FAABB& InAABB) const { mCacheWorldAABB = InAABB; mbWorldAABBDirty = false; }
	const FAABB& GetCacheWorldAABB() const { return mCacheWorldAABB; }

private:
	void MarkTransformDirty() { mbMatrixDirty = true; mbWorldAABBDirty = true; }

	FVector mRelativeLocation;
	FRotator mRelativeRotation;
	FVector mRelativeScale3D;

	mutable FMatrix mCacheWorldMatrix;
	mutable bool mbMatrixDirty = true;

	mutable FAABB mCacheWorldAABB;
	mutable bool mbWorldAABBDirty = true;
};

