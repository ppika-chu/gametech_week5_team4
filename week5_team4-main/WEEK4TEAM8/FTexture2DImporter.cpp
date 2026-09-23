#include "FTexture2DImporter.h"
#include "AssetFileIOs.h"
#include "FArchive.h"
#include "FLogManager.h"
#include "FGuid.h"
#include "Serializers.h"

// .png -> .uasset 변환
bool FTexture2DImporter::Import(const std::filesystem::path& InPath, const std::filesystem::path& OutPath, FAssetFileHeader& OutHead)
{
    FImagePayload Payload;

    if (!FImageFileIO::Load(InPath, Payload))
    {
        UE_LOG_ERROR("Failed to Load to %s", InPath.string().c_str());
        return false;
    }

    OutHead.Version = 1;
    OutHead.AssetType = EAssetType::Texture2D;
    OutHead.AssetID = FGuid::NewGuid();
    try
    {
        FWindowsBinWriter FileWriter(OutPath);
    
        FileWriter << OutHead;
        
        if (!FImageFileIO::Save(FileWriter, Payload))
        {
            UE_LOG_ERROR("Failed to Save to %s", OutPath.string().c_str());
            return false;
        }
    }
    catch(const std::exception& e)
    {
        UE_LOG_ERROR("Failed to write to %s", OutPath.string().c_str());
        return false;
    }

    return true;
}   

// .uasset 반환 or 변환 후 반환
std::optional<std::filesystem::path> FTexture2DImporter::GetorImport(const std::filesystem::path& InPath)
{
    std::filesystem::path CheckPath = InPath;
    CheckPath.replace_extension(".uasset");
    
    // .uasset 파일 있으면 그 경로 반환
    if(std::filesystem::exists(CheckPath))
        return CheckPath;

    // 없으면 Import 함수로 생성
    FAssetFileHeader OutHeader;
    if (Import(InPath, CheckPath, OutHeader))
        return CheckPath;
    else
        return std::nullopt;
}

std::optional<std::filesystem::path> FTexture2DImporter::GetorImport(
    const std::filesystem::path& InPath,
    const std::filesystem::path& OutDirectory)
{
    std::filesystem::path CheckPath = OutDirectory / InPath.filename();
    CheckPath.replace_extension(".uasset");

    if (std::filesystem::exists(CheckPath))
        return CheckPath;

    FAssetFileHeader OutHeader;
    if (Import(InPath, CheckPath, OutHeader))
        return CheckPath;

    return std::nullopt;
}
