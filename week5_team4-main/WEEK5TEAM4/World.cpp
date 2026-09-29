#include "World.h"

#include <format>

#include "RenderInfo.h"
#include "JsonUtil.h"
#include "Console.h"
#include "ObjectFactory.h"
#include "PrimitiveComponent.h"

UWorld::~UWorld()
{
	for (AActor* removeActor : mActors)
	{
		removeActor->SetWorld(nullptr);
		FObjectFactory::DestroyObject(removeActor);
	}
}

void UWorld::SerializeClass(json::JSON& outJson) const
{
	UObject::SerializeClass(outJson);
	json::JSON actorsJson = json::JSON::Make(json::JSON::Class::Array);

	for (const AActor* actor : mActors)
	{
		json::JSON actorJson;
		actor->SerializeClass(actorJson);
		actorsJson.append(std::move(actorJson));
	}

	outJson["Properties"]["mActors"] = actorsJson;
}

void UWorld::DeserializeClass(const json::JSON& inJson)
{
	UObject::DeserializeClass(inJson);

	const json::JSON& propertiesJson = inJson.at("Properties");

	if (!propertiesJson.hasKey("mActors") || propertiesJson.at("mActors").JSONType() != json::JSON::Class::Array)
	{
		throw std::runtime_error(std::format("{}: mActors requires an array", GetClass()->Name));
	}

	const json::JSON& actorsJson = propertiesJson.at("mActors");

	for (const auto& actorJson : actorsJson.ArrayRange())
	{
		if (!actorJson.hasKey("ClassName") || actorJson.at("ClassName").JSONType() != json::JSON::Class::String)
		{
			throw std::runtime_error(std::format("{}: ClassName requires a string", GetClass()->Name));
		}
		FString className(actorJson.at("ClassName").ToString());

		const FClassInfo* classInfo = FObjectFactory::GetClassInfoByName(className);
		if (!classInfo)
		{
			throw std::runtime_error(std::format("{}: Unknown class name: {}", GetClass()->Name, className));
		}
		AActor* actor = static_cast<AActor*>(FObjectFactory::LoadObject(classInfo, actorJson));
		AddActor(actor);
	}
}

void UWorld::AddActor(AActor* actor)
{
	assert(actor != nullptr);
	assert(getActorIndex(actor->UUID) == -1);

	mActors.Add(actor);
	actor->SetWorld(this);

	// TODO: 전처리를 통해 에디터 모드가 아니면 아래 코드를 컴파일하지 않게 막아야함.
	actor->CreateEditorComponents();
	RequestBVHRebuild();
}

bool UWorld::RemoveActor(uint32 componentUUID)
{
	int32 componentIndex = getActorIndex(componentUUID);
	if (componentIndex == -1)
	{
		return false;
	}

	AActor* Actor = mActors[componentIndex];
	Actor->SetWorld(nullptr);

	//mActors.RemoveAt(componentIndex, 1);
	mActors.RemoveAtSwap(componentIndex);
	RequestBVHRebuild();

	return true;
}

void UWorld::Tick(float deltaTime)
{
	for (AActor* actor : mActors)
	{
		actor->Tick(deltaTime);
	}

	// 씬 구성 변경으로 인한 최초 Build는 피킹 시간에 포함시키지 않는다.
	// 이동한 오브젝트의 refit은 실제 피킹 직전에만 수행한다.
	if (bBVHRebuildPending)
	{
		RebuildBVH();
	}
}

void UWorld::Render(float deltaTime, FRenderCollector& outCollector)
{
	// 쿼드/라인 정보는 Render()가 그린 뒤 스스로 비운다. 월드 바깥(엔진 루프의 AABB 디버그 라인 등)에서도
	// 채워지므로 여기서 Reset 하면 남의 것까지 날린다. 메시 배열만 여기서 갈아끼운다.
	outCollector.RenderInfos.Reset(DEFAULT_RESERVE_MEM);

	for (AActor* actor : mActors)
	{
		actor->Render(outCollector);
	}
}

UPrimitiveComponent* UWorld::RayCastBVH(const FPickingRay& Ray, uint64& OutTestCount)
{
	OutTestCount = 0;
	FlushBVHUpdates();

	TArray<FBVHRayHit> Candidates;
	BVH.QueryRay(Ray, Candidates);

	UPrimitiveComponent* NearestComponent = nullptr;
	float NearestT = FLT_MAX;
	for (const FBVHRayHit& Candidate : Candidates)
	{
		// 후보는 AABB 진입 거리순이다. 이미 찾은 삼각형보다 뒤에서 시작하면
		// 이후 후보 역시 더 가까운 결과가 될 수 없다.
		if (NearestComponent && Candidate.EntryDistance > NearestT * Ray.Length)
		{
			break;
		}

		++OutTestCount;
		float HitT = FLT_MAX;
		if (Candidate.Component->RayCastComponent(Ray, HitT) && HitT < NearestT)
		{
			NearestT = HitT;
			NearestComponent = Candidate.Component;
		}
	}

	return NearestComponent;
}

void UWorld::RequestBVHRebuild()
{
	bBVHRebuildPending = true;
	BVHBoundsUpdatePending.clear();
}

void UWorld::RequestBVHBoundsUpdate(UPrimitiveComponent* Component)
{
	if (!bBVHRebuildPending && Component)
	{
		BVHBoundsUpdatePending.insert(Component);
	}
}

void UWorld::RebuildBVH()
{
	TArray<UPrimitiveComponent*> Components;
	Components.Reserve(mActors.Num());
	for (AActor* Actor : mActors)
	{
		for (UActorComponent* Component : Actor->GetComponents())
		{
			if (UPrimitiveComponent* Primitive = Component->Cast<UPrimitiveComponent>())
			{
				Components.Add(Primitive);
			}
		}
	}

	BVH.Build(Components);
	BVHBoundsUpdatePending.clear();
	bBVHRebuildPending = false;
}

void UWorld::FlushBVHUpdates()
{
	if (bBVHRebuildPending)
	{
		RebuildBVH();
		return;
	}

	if (BVHBoundsUpdatePending.empty())
	{
		return;
	}

	// Set이 동일 컴포넌트의 위치/회전/스케일 변경을 하나로 합친다.

	const float EstimatedDepth = FMath::Max(1.0f,std::ceil(std::log2(static_cast<float>(BVH.GetLeafCount()))));
	
	// 개별 갱신 비용이 전체 갱신 비용보다 크면 전체 갱신으로 바꾼다. (BVH의 깊이 * 갱신할 리프 수 >= 전체 리프 수)
	const bool bRefitAll = EstimatedDepth * BVHBoundsUpdatePending.size() >= BVH.GetLeafCount();
	for (UPrimitiveComponent* Component : BVHBoundsUpdatePending)
	{

		if (!Component)
		{
			continue;
		}

		const FAABB Bounds = Component->GetBoundingBox();
		const bool bUpdated = bRefitAll
			? BVH.SetLeafBounds(Component, Bounds)
			: BVH.UpdateLeafAndRefit(Component, Bounds);

		if (!bUpdated)
		{
			RequestBVHRebuild();
			RebuildBVH();
			return;
		}
	}

	if (bRefitAll)
	{
		BVH.RefitAll();
	}
	BVHBoundsUpdatePending.clear();
}

int32 UWorld::getActorIndex(uint32 actorUUID) const
{
	for (uint32 i = 0; i < mActors.Num(); ++i)
	{
		if (mActors[i]->UUID == actorUUID)
		{
			return i;
		}
	}

	return -1;
}
