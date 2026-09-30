#pragma once

#include "SceneComponent.h"
#include "Assets.h"
#include "RayCast.h"

class UPrimitiveComponent : public USceneComponent
{
	REFLECT_CLASS(UPrimitiveComponent, USceneComponent)

public:
	UPrimitiveComponent();

	//void Initialize(GraphicsManager* graphicsManager, EPrimitive ePrimitive);
	//void Initialize(GraphicsManager* graphicsManager, EPrimitive ePrimitive, FVector location, FRotator rotation, FVector scale3D);

	using USceneComponent::Initialize;
	void Initialize(EPrimitive ePrimitive);
	void Initialize(EPrimitive ePrimitive, FVector location, FRotator rotation, FVector scale3D);

	virtual ~UPrimitiveComponent();

	virtual void SerializeClass(json::JSON& outJson) const override;
	virtual void DeserializeClass(const json::JSON& inJson) override;

	//virtual void Render();
	virtual void Render(FRenderCollector& RenderCollector, const FAABB& WorldBounds) override;

	// 프리미티브는 전부 픽킹 대상이다.
	virtual void RegisterPickTarget(FRenderCollector& RenderCollector, const FAABB& WorldBounds) override;

	virtual FAABB GetBoundingBox() const;
	virtual const TArray<FVertex>& GetMeshVertices() const;
	virtual const TArray<uint32>& GetMeshIndices() const;

	// 기본 primitive들은 삼각형 몇 개 없으니까 override 하고 그냥 brute force
	virtual const FMeshBVH* GetMeshBVH() const { return nullptr; }

	// 광선과 이 컴포넌트의 충돌을 판정한다.
	// MaxT : 지금까지 찾은 가장 가까운 hitT
	// OutT : Near로부터의 월드 거리
	virtual bool RayCastComponent(const FPickingRay& PickingRay, float MaxT, float& OutHitT) const;
};

