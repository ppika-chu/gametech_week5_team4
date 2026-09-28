#include "FSceneConverter.h"
#include "ObjectFactory.h"
#include "UStaticMeshComponent.h"
#include "FAssetManager.h"
#include "FLogManager.h"
#include "EngineStatics.h"
#include "World.h"
#include "Actor.h"

TSharedPtr<FStaticMeshAsset> FSceneConverter::ResolveMeshAsset(const FString& Path)
{
    // "Data/apple_mid.obj" -> "apple_mid"
    const std::string stem = std::filesystem::path(static_cast<std::string>(Path)).stem().string();

    TSharedPtr<FStaticMeshAsset> found;
    FAssetManager::Get().ForEachMetaInfo([&](const FAssetMetaInfo& metaInfo)
    {
        if (found || metaInfo.AssetType != EAssetType::StaticMesh)
        {
            return;
        }

        const std::string candidateStem =
            std::filesystem::path(metaInfo.AssetName.ToString().CStr()).stem().string();

        if (_stricmp(candidateStem.c_str(), stem.c_str()) == 0)
        {
            found = FAssetManager::Get().GetAssetAs<FStaticMeshAsset>(metaInfo.AssetID, true);
        }
    });

    return found;
}


// json -> UWorld* 반환
UWorld* FSceneConverter::BuildWorldFromJson(const json::JSON& sceneJson, uint32& outNextUUID)
{
    // 빈 UWorld 생성
    UWorld* newWorld = FObjectFactory::ConstructObject<UWorld>();

    // Primitives 안의 Entry -> AActor + Component -> UWorld에 추가
    // 파일 내 UUID 모두 무시하고 새로 배정하는 로직임.
    for (const auto& [key, primitiveJson] : sceneJson.at("Primitives").ObjectRange())
    {
        const FString typeName = primitiveJson.at("Type").ToString();   // StaticMeshComp

        // 현재는 StaticMesh Component만 대응함.
        if (typeName == "StaticMeshComp")
        {
            FVector location = JsonUtils::FromJson<FVector>(primitiveJson.at("Location"));
            FRotator rotation = JsonUtils::FromJson<FRotator>(primitiveJson.at("Rotation"));
            FVector scale = JsonUtils::FromJson<FVector>(primitiveJson.at("Scale"));
            FString meshPath = primitiveJson.at("ObjStaticMeshAsset").ToString();

            AActor* newActor = FObjectFactory::ConstructObject<AActor>();

            UStaticMeshComponent* meshComponent = 
                FObjectFactory::ConstructObject<UStaticMeshComponent>(location, rotation, scale);

            TSharedPtr<FStaticMeshAsset> asset = ResolveMeshAsset(meshPath);

            if (asset)
            {
                meshComponent->SetMesh(asset);
            }
            // TODO: asset 등록되지 않은 경우 새로 등록하는 로직
            else
            {
                UE_LOG_WARN("static mesh asset not found: %s", meshPath.CStr());
            }

            newActor->AddRootSceneComponent(meshComponent);
            newWorld->AddActor(newActor);

        }
        else
        {
            UE_LOG_WARN("unknown primitive type: %s, skipped.", typeName.CStr());
        }
    }

    // 최종 NextUUID 반환
    outNextUUID = UEngineStatics::GetNextUUID();

    return newWorld;
}

// 카메라 필드
void FSceneConverter::ApplyCamera(const json::JSON& sceneJson, FCamera* camera)
{
    const json::JSON& cameraJson = sceneJson.at("PerspectiveCamera");
    camera->Transform.Location = JsonUtils::FromJson<FVector>(cameraJson.at("Location"));
    camera->Transform.Rotation = JsonUtils::FromJson<FRotator>(cameraJson.at("Rotation"));
    camera->mFovDegree = cameraJson.at("FOV").at(0).ToFloat();
    camera->mNear = cameraJson.at("NearClip").at(0).ToFloat();
    camera->mFar = cameraJson.at("FarClip").at(0).ToFloat();

}

