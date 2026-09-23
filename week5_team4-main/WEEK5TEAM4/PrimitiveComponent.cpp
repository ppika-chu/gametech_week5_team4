
#include "PrimitiveComponent.h"

#include <format>

#include "RenderInfo.h"
#include "enum.h"
#include "JsonUtil.h"
#include "Console.h"
#include "Actor.h"
#include "FAssetManager.h"
#include "EngineMathLibrary.h"

#include "Cube.h"
#include "Sphere.h"
#include "Triangle.h"
#include "GizmoArrow.h"
#include "Circle.h"
#include "Plane.h"
#include "ShowFlags.h"

UPrimitiveComponent::UPrimitiveComponent()
{
}

/*
void UPrimitiveComponent::Initialize(GraphicsManager* graphicsManager, EPrimitive ePrimitive, FVector location, FRotator rotation, FVector scale3D)
{
	USceneComponent::Initialize(location, rotation, scale3D);

	mGraphicsManager = graphicsManager;
	mePrimitive = ePrimitive;
}
*/

void UPrimitiveComponent::Initialize(EPrimitive ePrimitive)
{
	Initialize(ePrimitive, FVector(0.f, 0.f, 0.f), FRotator(0.f, 0.f, 0.f), FVector(0.f, 0.f, 0.f));
}

void UPrimitiveComponent::Initialize(EPrimitive ePrimitive, FVector location, FRotator rotation, FVector scale3D)
{
	USceneComponent::Initialize(location, rotation, scale3D);
}

UPrimitiveComponent::~UPrimitiveComponent()
{
}

void UPrimitiveComponent::SerializeClass(json::JSON& outJson) const
{
	USceneComponent::SerializeClass(outJson);
}

void UPrimitiveComponent::DeserializeClass(const json::JSON& inJson)
{
	USceneComponent::DeserializeClass(inJson);
}

void UPrimitiveComponent::Render(FRenderCollector& RenderCollector)
{
}

void UPrimitiveComponent::RegisterPickTarget(FRenderCollector& RenderCollector)
{
	RenderCollector.PickTargets.Add(this);
}

FAABB UPrimitiveComponent::GetBoundingBox() const
{
	return FAABB();
}

const TArray<FVertex>& UPrimitiveComponent::GetMeshVertices() const
{
	static const TArray<FVertex> EmptyVertices; return EmptyVertices;
}

const TArray<uint32>& UPrimitiveComponent::GetMeshIndices() const
{
	static const TArray<uint32> EmptyIndices; return EmptyIndices;
}

bool UPrimitiveComponent::RayCastComponent(const FPickingRay& PickingRay, float& OutHitT) const
{
	const FMatrix WorldMatrix = GetTransformMatrix().MakeMatrix();

	// AABB 충돌체를 이용한 광선-메시 충돌 최적화
	const FAABB BoundingBox = GetBoundingBox();
	if (!RayIntersectsAABB(PickingRay.ToRay(), PickingRay.Length, BoundingBox))
	{
		return false;
	}

	// 메시 충돌체를 이용한 광선-삼각형 충돌 판정
	const TArray<FVertex>& vertices = GetMeshVertices();
	const TArray<uint32>& indices = GetMeshIndices();

	const FMatrix WorldToLocal = WorldMatrix.AffineInverse();
	if (WorldToLocal == FMatrix::Zero)
	{
		// 역행렬이 존재하지 않으면(스케일이 작아 det이 0에 가까운 경우) RayCast 대상에서 제외
		return false;
	}

	const FVector LocalNear = WorldToLocal.TransformPosition(PickingRay.Near);
	const FVector LocalFar = WorldToLocal.TransformPosition(PickingRay.Far);

	bool bHit = false;
	float NearestT = FLT_MAX;

	// 삼각형 리스트라 정점 3개씩 묶인다
	for (int32 i = 0; i < indices.Num(); i += 3)
	{
		const FVector V0 = vertices[indices[i]].GetPosition();
		const FVector V1 = vertices[indices[i + 1]].GetPosition();
		const FVector V2 = vertices[indices[i + 2]].GetPosition();

		float OutT, OutU, OutV;
		if (RayIntersectsTriangle(LocalNear, LocalFar, V0, V1, V2, OutT, OutU, OutV) && OutT < NearestT)
		{
			// 같은 메시 안에서도 더 가까운 삼각형이 뒤에 나올 수 있으므로 break 하지 않는다
			NearestT = OutT;
			bHit = true;
		}
	}

	if (bHit)
	{
		OutHitT = NearestT;
	}

	return bHit;
}


/*
void UPrimitiveComponent::Render(FStruct)
{
	// Todo: Fix renderer
	mGraphicsManager->Render(GetTransformMatrix(), mePrimitive);
}
*/


