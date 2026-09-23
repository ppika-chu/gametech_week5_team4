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
	virtual void Render(FRenderCollector& RenderCollector) override;

	// 프리미티브는 전부 픽킹 대상이다.
	virtual void RegisterPickTarget(FRenderCollector& RenderCollector) override;

	virtual FAABB GetBoundingBox() const;
	virtual const TArray<FVertex>& GetMeshVertices() const;
	virtual const TArray<uint32>& GetMeshIndices() const;

	// 광선과 이 컴포넌트의 충돌을 판정한다.
	// 맞으면 true를 돌려주고 OutHitT에 광선의 매개변수(Near가 0, Far가 1)를 채운다.
	// 값이 작을수록 카메라에 가까우므로 그대로 비교해서 가장 가까운 대상을 고를 수 있다.
	// 기본 구현은 AABB로 먼저 거르고 메시의 삼각형과 판정한다.
	// 다른 충돌 모양이 필요한 컴포넌트는 이 함수를 재정의한다.
	virtual bool RayCastComponent(const FPickingRay& PickingRay, float& OutHitT) const;
};



