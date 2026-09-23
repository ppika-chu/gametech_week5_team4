#pragma once

#include "Object.h"
#include <filesystem>

class FSceneManager;
class FGraphicsManager;
class AActor;
class UStaticMeshComponent;
class URenderer;
class FFileManager;
struct FStaticMeshPayload;

struct FRect;
struct FStaticMeshPayload;
struct FStaticMeshBuildData;

inline constexpr std::string_view kDefaultOBJPath = ".\\Assets\\Meshes\\";

class FObjViewer
{
public:
    void Initialize(FSceneManager& InSceneManager, URenderer& Renderer, FFileManager& InFileManager);
    void UpdateObjGUI(FGraphicsManager& InGraphicsManager);
    void OpenObj(const std::filesystem::path& FilePath);
    void OpenStaticMeshAsset(const std::filesystem::path& FilePath);

private:
    AActor* mViewerActor = nullptr;
    FSceneManager* mSceneManager = nullptr;
    FString mLoadedFilePath;
    FRect mViewportRcet;
    UStaticMeshComponent* mViewerComponent = nullptr;  
    URenderer* mRenderer = nullptr;

    bool bOpenedFromObj = false;

    bool BuildRuntimeMaterials(const std::filesystem::path& ObjPath,
        const FStaticMeshPayload& Payload, FStaticMeshBuildData& InOutBuildData);
};
