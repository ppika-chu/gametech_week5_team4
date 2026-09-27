#pragma once
#include "JsonUtil.h"

class UWorld;
class FCamera;
class FStaticMeshAsset;

class FSceneConverter
{
public:
    // json -> UWorld* 반환
    static UWorld* BuildWorldFromJson(const json::JSON& sceneJson, uint32& outNextUUID);

    // 카메라 필드
    static void ApplyCamera(const json::JSON& sceneJson, FCamera* camera);

private:

    // MeshAsset 찾는 헬퍼 함수
    static TSharedPtr<FStaticMeshAsset> ResolveMeshAsset(const FString& Path);

};