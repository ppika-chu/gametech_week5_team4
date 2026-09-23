#pragma once
#include <filesystem>
#include "FAsset.h"
#include <optional>
#include "FObjInfo.h"

class FMaterialImporter
{
public:
    // .png -> .uasset 변환
    static bool Import(
        const FObjMaterialInfo& MaterialInfo,   // Material
        const std::filesystem::path& InPath,    
        const std::filesystem::path& OutPath,
        FAssetFileHeader& OutHead
        );

private:

};