#pragma once

#include "FName.h"
#include "Object.h"
#include "Assets.h"
#include "FObjInfo.h"
#include "TMap.h"

class UStaticMesh : public UObject
{
	REFLECT_CLASS(UStaticMesh, UObject)

public:
    void SetStaticMeshAsset(
        const TSharedPtr<FStaticMeshAsset>& InStaticMeshAsset,
        const FString& InAssetPathFileName,
        const TArray<FObjMaterialInfo>& InMaterials,
        const TMap<FString, TSharedPtr<FTexture2DAsset>>& InDiffuseTextures)
    {
        StaticMeshAsset = InStaticMeshAsset;
        AssetPathFileName = InAssetPathFileName;
        Materials = InMaterials;
        DiffuseTextures = InDiffuseTextures;
    }

    void SetCookedStaticMeshAsset(
        const TSharedPtr<FStaticMeshAsset>& InStaticMeshAsset,
        const FString& InAssetPathFileName,
        const TArray<TSharedPtr<FMaterialAsset>>& InSectionMaterials)
    {
        StaticMeshAsset = InStaticMeshAsset;
        AssetPathFileName = InAssetPathFileName;
        SectionMaterials = InSectionMaterials;
    }

    const TSharedPtr<FStaticMeshAsset>& GetStaticMeshAsset() const { return StaticMeshAsset; }
    const FString& GetAssetPathFileName() const { return AssetPathFileName; }

    const FObjMaterialInfo* FindMaterial(const std::string& MaterialName) const
    {
        for (const FObjMaterialInfo& Material : Materials)
        {
            if (Material.Name == FString(MaterialName))
            {
                return &Material;
            }
        }
        return nullptr;
    }

    TSharedPtr<FTexture2DAsset> GetDiffuseTexture( const FString& MaterialName) const
    {
        const TSharedPtr<FTexture2DAsset>* FoundTexture = DiffuseTextures.Find(MaterialName);
        return FoundTexture ? *FoundTexture : nullptr;
    }

    TSharedPtr<FMaterialAsset> GetSectionMaterial(int32 SectionIndex) const
    {
        if (SectionIndex < 0 || SectionIndex >= SectionMaterials.Num())
        {
            return nullptr;
        }
        return SectionMaterials[SectionIndex];
    }

private:
    TSharedPtr<FStaticMeshAsset> StaticMeshAsset;
    FString AssetPathFileName;

    TArray<FObjMaterialInfo> Materials;
    TMap < FString, TSharedPtr<FTexture2DAsset>> DiffuseTextures;

    TArray<TSharedPtr<FMaterialAsset>> SectionMaterials;
};