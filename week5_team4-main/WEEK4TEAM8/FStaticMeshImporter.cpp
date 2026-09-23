#include "FStaticMeshImporter.h"
#include "FObjImporter.h"
#include "FArchive.h"
#include "FLogManager.h"
#include "FGuid.h"
#include "Serializers.h"
#include "FStaticMeshBuilder.h"
#include "FMaterialImporter.h"
#include "AssetFileIOs.h"
#include "Matrix.h"

// .obj -> .uasset 변환
bool FStaticMeshImporter::Import(const std::filesystem::path& InPath, const std::filesystem::path& OutPath, FAssetFileHeader& OutHead)
{
    FStaticMeshPayload Payload;
    FStaticMeshBuildData BuildData;

    if (!FStaticMeshFileIO::Load(InPath, Payload))
    {
        UE_LOG_ERROR("Failed to load static mesh source: %s", InPath.string().c_str());
        return false;
    }
    if (!FStaticMeshBuilder::Build(Payload.MeshDescription, BuildData))
    {
        UE_LOG_ERROR("Failed to build cooked mesh data: %s", InPath.string().c_str());
        return false;
    }
    
    OutHead.Version = 1;
    OutHead.AssetType = EAssetType::StaticMesh;
    OutHead.AssetID = FGuid::NewGuid();

    try
    {
        for (FObjMaterialInfo Material : Payload.Materials)
        {
            std::filesystem::path MaterialPath = OutPath.parent_path() / (OutPath.stem().string() + "_" + Material.Name.CStr() + "_material" + ".uasset");
            FAssetFileHeader MaterialHeader;
            FMaterialImporter::Import(Material, InPath.parent_path(), MaterialPath, MaterialHeader);
            
            for (FStaticMeshSection& Section : BuildData.Sections)
            {
                if (Section.MaterialName == Material.Name)
                {
                    Section.MaterialAssetID = MaterialHeader.AssetID;
                }
            }
        }
        FWindowsBinWriter FileWriter(OutPath);
        
        FileWriter << OutHead;

        FStaticMeshFileIO::Save(FileWriter, BuildData);
    }
    catch(const std::exception& e)
    {
        UE_LOG_ERROR("Failed to write to %s", OutPath.string().c_str());
        return false;
    }
    
    return true;
}   

bool FStaticMeshImporter::Export(const std::filesystem::path& InPath, const std::filesystem::path& OutPath, 
    FAssetFileHeader& OutHead, const FTransform& Transform, const FVector4& OverrideColor)
{
    FStaticMeshPayload Payload;
    FStaticMeshBuildData BuildData;

    if (!FStaticMeshFileIO::Load(InPath, Payload))
    {
        UE_LOG_ERROR("Failed to load static mesh source: %s", InPath.string().c_str());
        return false;
    }
    if (!FStaticMeshBuilder::Build(Payload.MeshDescription, BuildData))
    {
        UE_LOG_ERROR("Failed to build cooked mesh data: %s", InPath.string().c_str());
        return false;
    }

    const FMatrix BakeMatrix = Transform.MakeMatrix();
    const FMatrix NormalMatrix = BakeMatrix.AffineInverse().Transpose();
    for (FVertex& Vertex : BuildData.Vertices)
    {
        Vertex.Pos = BakeMatrix.TransformPosition(Vertex.Pos);
        Vertex.Normal = NormalMatrix.TransformVector(Vertex.Normal);
        Vertex.Normal.Normalize();
    }


    OutHead.Version = 1;
    OutHead.AssetType = EAssetType::StaticMesh;
    OutHead.AssetID = FGuid::NewGuid();

    try
    {
        for (FObjMaterialInfo Material : Payload.Materials)
        {
            std::filesystem::path MaterialPath = OutPath.parent_path() / (OutPath.stem().string() + "_" + Material.Name.CStr() + "_material" +".uasset");
            FAssetFileHeader MaterialHeader;
            FMaterialImporter::Import(Material, InPath.parent_path(), MaterialPath, MaterialHeader);

            for (FStaticMeshSection& Section : BuildData.Sections)
            {
                if (Section.MaterialName == Material.Name)
                {
                    Section.MaterialAssetID = MaterialHeader.AssetID;
                }
            }
        }
        FWindowsBinWriter FileWriter(OutPath);

        FileWriter << OutHead;

        FStaticMeshFileIO::Save(FileWriter, BuildData);
    }
    catch (const std::exception& e)
    {
        UE_LOG_ERROR("Failed to write to %s", OutPath.string().c_str());
        return false;
    }

    return true;
}

// .uasset 반환 or 변환 후 반환
std::optional<std::filesystem::path> FStaticMeshImporter::GetorImport(const std::filesystem::path& InPath)
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