#include "World.h"

#include <format>

#include "RenderInfo.h"
#include "JsonUtil.h"
#include "Console.h"
#include "ObjectFactory.h"

#include "OptimizationFlags.h"

UWorld::~UWorld()
{
	for (AActor* removeActor : mActors)
	{
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

	TArray<UPrimitiveComponent*> AllPrimitives;
	for (AActor* Actor : mActors)
	{
		for (UActorComponent* Component : Actor->GetComponents())
		{
			if (UPrimitiveComponent* Primitive = Component->Cast<UPrimitiveComponent>())
			{
				AllPrimitives.Add(Primitive);
			}
		}
	}
	mBVH.Build(AllPrimitives);
}

void UWorld::AddActor(AActor* actor)
{
	assert(actor != nullptr);
	assert(getActorIndex(actor->UUID) == -1);

	mActors.Add(actor);

	// TODO: 전처리를 통해 에디터 모드가 아니면 아래 코드를 컴파일하지 않게 막아야함.
	actor->CreateEditorComponents();

	for (UActorComponent* Component : actor->GetComponents())
	{
		if (UPrimitiveComponent* Primitive = Component->Cast<UPrimitiveComponent>())
		{
			mBVH.Insert(Primitive);
		}
		RegisterTick(Component);
	}
}

bool UWorld::RemoveActor(uint32 componentUUID)
{
	int32 componentIndex = getActorIndex(componentUUID);
	if (componentIndex == -1)
	{
		return false;
	}

	AActor* actor = mActors[componentIndex];
	for (UActorComponent* Component : actor->GetComponents())
	{
		if (UPrimitiveComponent* Primitive = Component->Cast<UPrimitiveComponent>())
		{
			mBVH.Remove(Primitive);
		}
		UnregisterTick(Component);
	}

	//mActors.RemoveAt(componentIndex, 1);
	mActors.RemoveAtSwap(componentIndex);

	return true;
}

void UWorld::Tick(float deltaTime)
{
	for (UActorComponent* ActorComponent : TickComponents)
	{
		ActorComponent->Tick(deltaTime);
	}
}

void UWorld::RegisterTick(UActorComponent* Component)
{
	// bCanEverTick && bTickEnabled && 미등록이면 추가

	if (Component->IsActiveTick() && Component->GetTickListIndex() == -1)
	{
		Component->SetTickListIndex(TickComponents.Num());
		TickComponents.Add(Component);
	}
}

void UWorld::UnregisterTick(UActorComponent* Component)
{
	if (Component->GetTickListIndex() == -1)
		return;

	int32 Index = Component->GetTickListIndex();
	UActorComponent* LastComponent = TickComponents.Last();

	TickComponents[Index] = LastComponent;
	LastComponent->SetTickListIndex(Index);
	TickComponents.RemoveLast();
	Component->SetTickListIndex(-1);
}

void UWorld::Render(float deltaTime, FRenderCollector& outCollector)
{
	// 쿼드/라인 정보는 Render()가 그린 뒤 스스로 비운다. 월드 바깥(엔진 루프의 AABB 디버그 라인 등)에서도
	// 채워지므로 여기서 Reset 하면 남의 것까지 날린다. 메시/픽킹 배열만 여기서 갈아끼운다.
	outCollector.RenderInfos.Reset(DEFAULT_RESERVE_MEM);
	outCollector.PickTargets.Reset(DEFAULT_RESERVE_MEM);

	const bool bUseBVH = outCollector.Frustum && GCullingMode == ECullingMode::BVH;

	if (bUseBVH)
	{
		mBVH.QueryFrustum(*outCollector.Frustum, [&outCollector](UPrimitiveComponent* Component)
		{	
			const FAABB WorldBounds = Component->GetBoundingBox();
			Component->Render(outCollector, WorldBounds);

			// 브루트포스 피킹을 쓸 때만 후보 목록이 필요하다.
			if (GPickingMode == EPickingMode::BruteForce)
			{
				Component->RegisterPickTarget(outCollector, WorldBounds);
			}
		});

		if (FShowFlags::Get().IsEnabled(EShowFlag::UUIDText))
		{
			for (AActor* actor : mActors)
			{
				for (UActorComponent* component : actor->GetComponents())
				{
					if (!component->Cast<UPrimitiveComponent>())
					{
						component->Render(outCollector, component->GetBoundingBox());
					}
				}
			}
		}
	}
	else{
		
		for (AActor* actor : mActors)
		{
			actor->Render(outCollector);
		}
	}
	outCollector.CulledObjectCount = GetBVH().GetComponentNum() - outCollector.DrawnObjectCount;
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
