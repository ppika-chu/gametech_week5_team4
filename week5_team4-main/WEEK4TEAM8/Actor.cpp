#include "Actor.h"
#include "JsonUtil.h"
#include "RenderInfo.h"
#include "SceneComponent.h"
#include "UTextComponent.h"
#include "ObjectFactory.h"
#include <format>

AActor::~AActor()
{
	for (UActorComponent* removeComponent : mComponents)
	{
		FObjectFactory::DestroyObject(removeComponent);
	}
}

void AActor::Initialize()
{
	UObject::Initialize();

	mbPressed = false;
	mbStarted = false;
}

void AActor::SerializeClass(json::JSON& outJson) const
{
	UObject::SerializeClass(outJson);
	json::JSON componentsJson = json::JSON::Make(json::JSON::Class::Array);

	for (const UActorComponent* component : mComponents)
	{
		if (!component->ShouldSerialize())
		{
			continue;
		}

		json::JSON componentJson;
		component->SerializeClass(componentJson);
		componentsJson.append(std::move(componentJson));
	}

	outJson["Properties"]["mComponents"] = componentsJson;
	outJson["Properties"]["mRootComponentUUID"] = mRootComponent ? mRootComponent->UUID : -1;
}

void AActor::DeserializeClass(const json::JSON& inJson)
{
	UObject::DeserializeClass(inJson);

	const json::JSON& propertiesJson = inJson.at("Properties");

	if (!propertiesJson.hasKey("mComponents") || propertiesJson.at("mComponents").JSONType() != json::JSON::Class::Array)
	{
		throw std::runtime_error(std::format("{}: mComponents requires an array", GetClass()->Name));
	}

	const json::JSON& componentsJson = propertiesJson.at("mComponents");

	for (const auto& componentJson : componentsJson.ArrayRange())
	{
		if (!componentJson.hasKey("ClassName") || componentJson.at("ClassName").JSONType() != json::JSON::Class::String)
		{
			throw std::runtime_error(std::format("{}: ClassName requires a string", GetClass()->Name));
		}
		FString className(componentJson.at("ClassName").ToString());

		const FClassInfo* classInfo = FObjectFactory::GetClassInfoByName(className);
		if (!classInfo)
		{
			throw std::runtime_error(std::format("{}: Unknown class name: {}", GetClass()->Name, className));
		}
		UActorComponent* component = static_cast<UActorComponent*>(FObjectFactory::LoadObject(classInfo, componentJson));
		AddComponent(component);
	}

	if (!propertiesJson.hasKey("mRootComponentUUID") || propertiesJson.at("mRootComponentUUID").JSONType() != json::JSON::Class::Integral)
	{
		throw std::runtime_error(std::format("{}: mRootComponentUUID requires an integral", GetClass()->Name));
	}
	int32 rootComponentUUID = propertiesJson.at("mRootComponentUUID").ToInt();
	if (rootComponentUUID == -1)
	{
		mRootComponent = nullptr;
	}
	else
	{
		int32 rootComponentIndex = getComponentIndex(rootComponentUUID);
		if (rootComponentIndex == -1)
		{
			throw std::runtime_error(std::format("{}: Invalid root component UUID: {}", GetClass()->Name, rootComponentUUID));
		}
		mRootComponent = static_cast<USceneComponent*>(mComponents[rootComponentIndex]);
	}
}

void AActor::AddComponent(UActorComponent* actorComponent)
{
	assert(actorComponent);
	assert(getComponentIndex(actorComponent->UUID) == -1);

	mComponents.Add(actorComponent);
	actorComponent->SetOwner(this);
}

void AActor::AddRootSceneComponent(USceneComponent* sceneComponent)
{
	assert(sceneComponent);
	assert(getComponentIndex(sceneComponent->UUID) == -1);

	mRootComponent = sceneComponent;
	AddComponent(sceneComponent);
}

USceneComponent* AActor::GetRootComponent() const
{
	return mRootComponent;
}

bool AActor::RemoveComponent(uint32 componentUUID)
{
	int32 componentIndex = getComponentIndex(componentUUID);
	if (componentIndex == -1)
	{
		return false;
	}

	//mComponents.RemoveAt(componentIndex, 1);
	mComponents.RemoveAtSwap(componentIndex);

	return true;
}

void AActor::CreateEditorComponents()
{
	UText3DComponent* Text3DComponent = FObjectFactory::ConstructObject<UText3DComponent>(FVector(0, 0, 1), FRotator(0, 0, 0), FVector(1, 1, 1));
	Text3DComponent->SetBillboard(true);
	Text3DComponent->SetText(Utf2Wide(std::format("UUID: {}", UUID)));
	Text3DComponent->SetFontAtlasAsset(FAssetManager::Get().GetAssetAs<FFontAtlasAsset>(FName("TestFontAtlas")));
	Text3DComponent->SetDepthState(false, false);
	Text3DComponent->SetEditorOnly(true);
	Text3DComponent->SetDoNotSerialize(true);

	AddComponent(Text3DComponent);
}

FTransform AActor::GetTransform() const
{
	if (mRootComponent)
	{
		return mRootComponent->GetTransformMatrix();
	}
	else
	{
		return FTransform();
	}
}

void AActor::Tick(float deltaTime)
{
	for (UActorComponent* component : mComponents)
	{
		component->Tick(deltaTime);
	}
}

void AActor::Render(FRenderCollector& RenderCollector)
{
	for (UActorComponent* component : mComponents)
	{
		component->Render(RenderCollector);

		// 렌더 정보를 모으는 김에 픽킹 대상도 같이 모은다.
		// 액터 계층을 두 번 훑지 않기 위함이다.
		component->RegisterPickTarget(RenderCollector);
	}
}

void AActor::GetRenderInfos(TArray<FRenderInfo>* outRenderInfos) const
{
	assert(outRenderInfos);

	for (const UActorComponent* component : mComponents)
	{
		component->GetRenderInfos(outRenderInfos);
	}
}

bool AActor::GetFirstRenderInfo(FRenderInfo &outRenderInfo) const
{
	TArray<FRenderInfo> renderInfos;
	GetRenderInfos(&renderInfos);

	if (renderInfos.Num() == 0)
	{
		return false;
	}

	outRenderInfo = renderInfos[0];

	return true;
}

void AActor::SetLocation(FVector location)
{
	if (mRootComponent)
	{
		mRootComponent->SetRelativeLocation(location);
	}
}

void AActor::SetRotation(FRotator rotation)
{
	if (mRootComponent)
	{
		mRootComponent->SetRelativeRotation(rotation);
	}
}

void AActor::SetScale(FVector scale)
{
	if (mRootComponent)
	{
		mRootComponent->SetRelativeScale3D(scale);
	}
}

int32 AActor::getComponentIndex(int32 componentUUID) const
{
	for (uint32 i = 0; i < mComponents.Num(); ++i)
	{
		if (mComponents[i]->UUID == componentUUID)
		{
			return i;
		}
	}

	return -1;
}
