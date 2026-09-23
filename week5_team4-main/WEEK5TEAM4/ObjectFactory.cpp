#include "ObjectFactory.h"

#include "Json/json.hpp"

#include "Actor.h"
#include "PrimitiveComponent.h"
#include "UAtlasAnimationComponent.h"
#include "EngineStatics.h"

UObject* FObjectFactory::ConstructUnInitializedObject(const FClassInfo* classInfo)
{
	if (!classInfo || !classInfo->Constructor)
	{
		return nullptr;
	}

	UObject* Instance = classInfo->CreateInstance();
	if (Instance)
	{
		Instance->UUID = UEngineStatics::GenerateUUID();

		Instance->InternalIndex = UObject::GUObjectArray.Add(Instance);

		const FClassInfo* ClassInfo = Instance->GetClass();
		if (UObject::GUObjectMap.Contains(ClassInfo))
		{
			TArray<uint32>& ObjectIndices = UObject::GUObjectMap[ClassInfo];
			Instance->ObjectMapIndex = ObjectIndices.Num();
			ObjectIndices.Add(Instance->InternalIndex);
		}
		else
		{
			Instance->ObjectMapIndex = 0;

			TArray<uint32> NewArray;
			NewArray.Add(Instance->InternalIndex);

			UObject::GUObjectMap.Add(ClassInfo, NewArray);
		}
	}

	return Instance;
}

UObject* FObjectFactory::LoadObject(const FClassInfo* classInfo, const json::JSON& inJson)
{
	UObject* instance = ConstructUnInitializedObject(classInfo);

	if (instance)
	{
		instance->DeserializeClass(inJson);
	}
	return instance;
}

void FObjectFactory::DestroyObject(UObject* Object)
{
	if (!Object)
	{
		return;
	}

	const FClassInfo* ClassInfo = Object->GetClass();
	TArray<uint32>& ObjectIndices = UObject::GUObjectMap[ClassInfo];

	UObject* LastObject = UObject::GUObjectArray[ObjectIndices.Last()];
	LastObject->ObjectMapIndex = Object->ObjectMapIndex;
	ObjectIndices[Object->ObjectMapIndex] = LastObject->InternalIndex;

	ObjectIndices.RemoveLast();
	if (ObjectIndices.Num() == 0)
	{
		UObject::GUObjectMap.Remove(ClassInfo);
	}

	UObject::GUObjectArray.RemoveAt(Object->InternalIndex);

	delete Object;
}

AActor* FObjectFactory::SpawnPrimitiveActor(
	EPrimitive primitiveType,
	FVector3 Location, FRotator Rotation, FVector3 Scale)
{
	// Create a new actor
	AActor* actor = ConstructObject<AActor>();

	UPrimitiveComponent* component = ConstructObject<UPrimitiveComponent>(primitiveType, Location, Rotation, Scale);

	actor->AddRootSceneComponent(component);

	return actor;
}

const FClassInfo* FObjectFactory::GetClassInfoByName(const FString& className)
{
	if (!mClassInfoMap.Contains(className))
	{
		return nullptr;
	}

	return mClassInfoMap[className]();
}

bool FObjectFactory::RegisterClassInfo(FString className, const FClassInfo* classInfo)
{
	if (mClassInfoMap.Contains(className))
	{
		return false;
	}
	mClassInfoMap.Add(className, [classInfo]() -> const FClassInfo* { return classInfo; });
	return true;
}

#include "SceneComponent.h"
#include "CubeComponent.h"
#include "SphereComponent.h"
#include "UTextComponent.h"
#include "World.h"
#include "UStaticMeshComponent.h"

TMap<FString, std::function<const FClassInfo* ()>> FObjectFactory::mClassInfoMap = {
	{"UObject", &UObject::GetStaticClass },
	{"AActor", &AActor::GetStaticClass },
	{"UActorComponent", &UActorComponent::GetStaticClass },
	{"USceneComponent", &USceneComponent::GetStaticClass },
	{"UPrimitiveComponent", &UPrimitiveComponent::GetStaticClass },
	{"UCubeComponent", &UCubeComponent::GetStaticClass },
	{"USphereComponent", &USphereComponent::GetStaticClass },
	{"ASpotLight", &ASpotLight::GetStaticClass },
	{"USpotLightComponent", &USpotLightComponent::GetStaticClass },
	{"UPlaneComponent", &UPlaneComponent::GetStaticClass },
	{"UText3DComponent", &UText3DComponent::GetStaticClass },
	{"UAtlasAnimationComponent", &UAtlasAnimationComponent::GetStaticClass },
	{"UWorld", &UWorld::GetStaticClass },
	{"UStaticMeshComponent", &UStaticMeshComponent::GetStaticClass},
};
