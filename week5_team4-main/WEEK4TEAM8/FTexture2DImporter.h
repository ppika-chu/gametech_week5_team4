#pragma once
#include <filesystem>
#include "FAsset.h"
#include <optional>

class FTexture2DImporter
{
public:
    // .png -> .uasset 변환
    static bool Import(
        const std::filesystem::path& InPath,
        const std::filesystem::path& OutPath,
        FAssetFileHeader& OutHead
        );

    // .uasset 반환 or 변환 후 반환
    static std::optional<std::filesystem::path> GetorImport(
        const std::filesystem::path& InPath
        );

    // 원본은 외부 경로에서도 읽되, 생성한 에셋은 프로젝트 내 지정 폴더에 둔다.
    static std::optional<std::filesystem::path> GetorImport(
        const std::filesystem::path& InPath,
        const std::filesystem::path& OutDirectory
        );

private:

};
