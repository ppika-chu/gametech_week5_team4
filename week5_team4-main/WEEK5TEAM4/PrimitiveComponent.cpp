
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
#include "OptimizationFlags.h"

#include <xmmintrin.h>

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

void UPrimitiveComponent::Render(FRenderCollector& RenderCollector, const FAABB& WorldBounds)
{
}

void UPrimitiveComponent::RegisterPickTarget(FRenderCollector& RenderCollector, const FAABB& WorldBounds)
{
	// Picking 후보에 넣기 전 Frustum 안에 있는지 확인
	if (RenderCollector.Frustum && !RenderCollector.Frustum->Intersects(WorldBounds))
	{
		return;
	}

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

bool UPrimitiveComponent::RayCastComponent(const FPickingRay& PickingRay, float MaxT, float& OutHitT) const
{

	if (PickingRay.Length <= 0.f) return false;

	// 월드 거리 상한 -> 로컬 파라미터(0~1) 상한
	const float LocalMaxT = (std::min) ((MaxT / PickingRay.Length), 1.0f);
	const FMatrix& WorldMatrix = GetCacheWorldMatrix(); 

	// 메시 충돌체를 이용한 광선-삼각형 충돌 판정
	const TArray<FVertex>& vertices = GetMeshVertices();
	const TArray<uint32>& indices = GetMeshIndices();

	const FMatrix WorldToLocal = WorldMatrix.AffineInverse();
	// 역행렬이 존재하지 않으면(스케일이 작아 det이 0에 가까운 경우) RayCast 대상에서 제외
	if (WorldToLocal == FMatrix::Zero)
		return false;

	const FVector LocalNear = WorldToLocal.TransformPosition(PickingRay.Near);
	const FVector LocalFar = WorldToLocal.TransformPosition(PickingRay.Far);
	const FVector D = LocalFar - LocalNear;

	// FMeshBVH 있으면 삼각형 검사는 이걸로.
	if (IsOptEnabled(EOptFlag::MeshBVHPicking))
	if (const FMeshBVH* BVH = GetMeshBVH())
	{
		if (BVH->IsValid())
		{
			float T = LocalMaxT;	// 이미 찾은 것보다 먼 것은 bvh가 잘라냄.
			if (BVH->RayCast(LocalNear, D, T))
			{
				OutHitT = T * PickingRay.Length;
				return true;
			}
			return false;
		}
	}

	// 없으면 FallBack
	const __m128 OriginX = _mm_set1_ps(LocalNear.x);
	const __m128 OriginY = _mm_set1_ps(LocalNear.y);
	const __m128 OriginZ = _mm_set1_ps(LocalNear.z);
	const __m128 DirX = _mm_set1_ps(D.x);
	const __m128 DirY = _mm_set1_ps(D.y);
	const __m128 DirZ = _mm_set1_ps(D.z);
	const __m128 Epsilon = _mm_set1_ps(1e-6f);
	const __m128 Zero = _mm_setzero_ps();
	const __m128 One = _mm_set1_ps(1.0f);

	bool bHit = false;
	float NearestT = LocalMaxT;

	const int32 TriCount = indices.Num() / 3;
	for (int32 TriBase = 0; TriBase < TriCount; TriBase += 4)
	{
		const int32 Remaining = (std::min)(4, TriCount - TriBase);

		float V0x[4], V0y[4], V0z[4];
		float E1x[4], E1y[4], E1z[4];
		float E2x[4], E2y[4], E2z[4];

		for (int32 k = 0; k < 4; ++k)
		{
			const int32 TriIdx = TriBase + ((k < Remaining) ? k : Remaining - 1);
			const FVector& V0 = vertices[indices[TriIdx*3+0]].GetPosition();
			const FVector& V1 = vertices[indices[TriIdx*3+1]].GetPosition();
			const FVector& V2 = vertices[indices[TriIdx*3+2]].GetPosition();

			V0x[k]=V0.x; V0y[k]=V0.y; V0z[k]=V0.z;
			E1x[k]=V1.x-V0.x; E1y[k]=V1.y-V0.y; E1z[k]=V1.z-V0.z;
			E2x[k]=V2.x-V0.x; E2y[k]=V2.y-V0.y; E2z[k]=V2.z-V0.z;
		}

		__m128 E1X=_mm_loadu_ps(E1x), E1Y=_mm_loadu_ps(E1y), E1Z=_mm_loadu_ps(E1z);
		__m128 E2X=_mm_loadu_ps(E2x), E2Y=_mm_loadu_ps(E2y), E2Z=_mm_loadu_ps(E2z);

		// P = cross(D, E2)
		__m128 PX = _mm_sub_ps(_mm_mul_ps(DirY,E2Z), _mm_mul_ps(DirZ,E2Y));
		__m128 PY = _mm_sub_ps(_mm_mul_ps(DirZ,E2X), _mm_mul_ps(DirX,E2Z));
		__m128 PZ = _mm_sub_ps(_mm_mul_ps(DirX,E2Y), _mm_mul_ps(DirY,E2X));

		__m128 Det = _mm_add_ps(_mm_add_ps(_mm_mul_ps(E1X,PX), _mm_mul_ps(E1Y, PY)), _mm_mul_ps(E1Z, PZ));
		__m128 ValidDet = _mm_cmpge_ps(_mm_andnot_ps(_mm_set1_ps(-0.0f), Det), Epsilon);
		__m128 InvDet = _mm_div_ps(One, Det);

		__m128 V0X = _mm_loadu_ps(V0x), V0Y = _mm_loadu_ps(V0y), V0Z = _mm_loadu_ps(V0z);
		__m128 TX = _mm_sub_ps(OriginX, V0X);
		__m128 TY = _mm_sub_ps(OriginY, V0Y);
		__m128 TZ = _mm_sub_ps(OriginZ, V0Z);

		__m128 U = _mm_mul_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(TX,PX), _mm_mul_ps(TY,PY)), _mm_mul_ps(TZ,PZ)), InvDet);
		__m128 UValid = _mm_and_ps(_mm_cmpge_ps(U, Zero), _mm_cmple_ps(U, One));

		// Q = cross(T, E1)
		__m128 QX = _mm_sub_ps(_mm_mul_ps(TY, E1Z), _mm_mul_ps(TZ, E1Y));
		__m128 QY = _mm_sub_ps(_mm_mul_ps(TZ, E1X), _mm_mul_ps(TX, E1Z));
		__m128 QZ = _mm_sub_ps(_mm_mul_ps(TX, E1Y), _mm_mul_ps(TY, E1X));

		__m128 V = _mm_mul_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(DirX,QX), _mm_mul_ps(DirY,QY)), _mm_mul_ps(DirZ, QZ)), InvDet);
		__m128 VValid = _mm_and_ps(_mm_cmpge_ps(V, Zero), _mm_cmple_ps(_mm_add_ps(U,V), One));

		__m128 T = _mm_mul_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(E2X,QX), _mm_mul_ps(E2Y,QY)), _mm_mul_ps(E2Z,QZ)), InvDet);
		__m128 TValid = _mm_cmpgt_ps(T, Epsilon);

		__m128 Mask = _mm_and_ps(_mm_and_ps(ValidDet, UValid), _mm_and_ps(VValid, TValid));
		int MaskBits = _mm_movemask_ps(Mask);

		float TArr[4];
		_mm_storeu_ps(TArr, T);

		for (int32 k = 0; k < Remaining; ++k)
		{
			if ((MaskBits & (1 << k)) && TArr[k] < NearestT)
			{
				NearestT = TArr[k];
				bHit = true;
			}
		}
	}
	if (bHit)
		OutHitT = NearestT * PickingRay.Length;

	return bHit;
}


/*
void UPrimitiveComponent::Render(FStruct)
{
	// Todo: Fix renderer
	mGraphicsManager->Render(GetTransformMatrix(), mePrimitive);
}
*/


