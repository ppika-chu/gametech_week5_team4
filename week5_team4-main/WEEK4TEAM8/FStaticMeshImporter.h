#pragma once
#include <filesystem>
#include "FAsset.h"
#include <optional>
#include "Transform.h"

class FStaticMeshImporter
{
public:
    // .obj -> .uasset 변환
    static bool Import(
        const std::filesystem::path& InPath,
        const std::filesystem::path& OutPath,
        FAssetFileHeader& OutHead
        );

    static bool Export(
        const std::filesystem::path& InPath,
        const std::filesystem::path& OutPath,
        FAssetFileHeader& OutHead,
        const FTransform& Transform,
        const FVector4& OverrideColor
    );

    // .uasset 반환 or 변환 후 반환
    static std::optional<std::filesystem::path> GetorImport(
        const std::filesystem::path& InPath
        );

private:

};