#include "UStaticMeshComponent.h"

#include "FAssetManager.h"
#include "RenderInfo.h"
#include "ShowFlags.h"
#include "OptimizationFlags.h"
#include "Actor.h"
#include "JsonUtil.h"
#include "EngineMathLibrary.h"
#include "FLogManager.h"
#include "EngineMathLibrary.h"

void UStaticMeshComponent::Initialize(const FString& InAssetPathFileName, FVector Location,
    FRotator Rotation, FVector Scale)
{
    USceneComponent::Initialize(Location, Rotation, Scale);
}

void UStaticMeshComponent::SerializeClass(json::JSON& outJson) const
{
    USceneComponent::SerializeClass(outJson);

    FGuid AssetID = mMeshAsset ? mMeshAsset->GetAssetID() : FGuid();
    outJson["Properties"]["ObjStaticMeshAsset"] = JsonUtils::ToJson(AssetID);

	TArray<FGuid> MaterialAssetIDs;
	for (int32 i = 0; i < mMaterialAssets.Num(); ++i)
	{
		const TSharedPtr<FMaterialAsset>& MaterialAsset = mMaterialAssets[i];
		FGuid MaterialAssetID = MaterialAsset ? MaterialAsset->GetAssetID() : FGuid();
		MaterialAssetIDs.Add(MaterialAssetID);
	}
	outJson["Properties"]["ObjMaterialAssets"] = JsonUtils::ToJson(MaterialAssetIDs);
	outJson["Properties"]["UVOffsets"] = JsonUtils::ToJson(mUVOffsets);
}

void UStaticMeshComponent::DeserializeClass(const json::JSON& inJson)
{
    USceneComponent::DeserializeClass(inJson);

    const json::JSON& PropertiesJson = inJson.at("Properties");

    if (!PropertiesJson.hasKey("ObjStaticMeshAsset"))
    {
        throw std::runtime_error("UStaticMeshComponent: ObjStaticMeshAsset property is required");
    }

    if (PropertiesJson.at("ObjStaticMeshAsset").JSONType() != json::JSON::Class::Object)
    {
        throw std::runtime_error("UStaticMeshComponent: ObjStaticMeshAsset property requires an object");
    }

    FGuid AssetID = JsonUtils::FromJson<FGuid>(PropertiesJson.at("ObjStaticMeshAsset"));

    if (AssetID.IsValid())
    {
        SetMesh(FAssetManager::Get().GetAssetAs<FStaticMeshAsset>(AssetID, true));
    }

	TArray<FGuid> MaterialAssetIDs;
	if (PropertiesJson.hasKey("ObjMaterialAssets"))
	{
		JsonUtils::FromJson(PropertiesJson.at("ObjMaterialAssets"), MaterialAssetIDs);
	}

	for (int32 i = 0; i < MaterialAssetIDs.Num(); ++i)
	{
		if (i >= mMaterialAssets.Num())
		{
            break;
		}

		mMaterialAssets[i] = FAssetManager::Get().GetAssetAs<FMaterialAsset>(MaterialAssetIDs[i], true);
	}

	TArray<FVector2> UVOffsets;
	if (PropertiesJson.hasKey("UVOffsets"))
	{
		JsonUtils::FromJson(PropertiesJson.at("UVOffsets"), UVOffsets);
	}

	for (int32 i = 0; i < UVOffsets.Num(); ++i)
	{
		if (i >= mUVOffsets.Num())
		{
			break;
		}

		mUVOffsets[i] = UVOffsets[i]; 
	}
}

void UStaticMeshComponent::Render(FRenderCollector& RenderCollector, const FAABB& WorldBounds)
{
    if (!mMeshAsset)
    {
        return;
    }

    if (!FShowFlags::Get().IsEnabled(EShowFlag::Primitive))
    {
        return;
    }


    // Frustum Culling
    // BVH 모드에서도 남겨둔다. BVH는 리프(4개 묶음) 단위라 경계에 걸친 것은 여기서 정밀하게 걸러진다.
    if (GCullingMode != ECullingMode::Off
        && RenderCollector.Frustum && !RenderCollector.Frustum->Intersects(WorldBounds))
    {
        ++RenderCollector.CulledObjectCount;
        return;
    }

    const TArray<FStaticMeshSection>* SectionsToUse = &mMeshAsset->GetSections();
    Microsoft::WRL::ComPtr<ID3D11Buffer> VertexBufferToUse = mMeshAsset->GetVertexBuffer();
    Microsoft::WRL::ComPtr<ID3D11Buffer> IndexBufferToUse = mMeshAsset->GetIndexBuffer();

    // 너무 작은 픽셀은 LOD
    if (RenderCollector.Camera && IsOptEnabled(EOptFlag::LOD))
    {
        const float RadiusSq = ((WorldBounds.Max - WorldBounds.Min) * 0.5f).LengthSquared();
        const FVector Center = (WorldBounds.Min + WorldBounds.Max) * 0.5f;
        const float DistanceSq = (Center - RenderCollector.Camera->Transform.Location).LengthSquared();

        const float LODDistanceRatios[] = { GLOD1DistanceRatio, GetLOD2DistanceRatio() }; // LOD1, LOD2
    
        for (int32 i = 0; i < mMeshAsset->GetLODCount() && i < 2; ++i)
        {
            if (DistanceSq > RadiusSq * LODDistanceRatios[i] * LODDistanceRatios[i])
            {
                const FMeshLOD& LOD = mMeshAsset->GetLOD(i);
                SectionsToUse = &LOD.Sections;
                VertexBufferToUse = LOD.VertexBuffer->Buffer;
                IndexBufferToUse = LOD.IndexBuffer->Buffer;
            }   
        }
    }

    // actor 당 한 번 count
    ++RenderCollector.DrawnObjectCount;

    for (int32 SectionIndex = 0; SectionIndex < SectionsToUse->Num(); ++SectionIndex)
    {
        const FStaticMeshSection& Section = (*SectionsToUse)[SectionIndex];

        TSharedPtr<FMaterialAsset> Material = mMaterialAssets[SectionIndex];

        const FVector4 MaterialColor = Material
            ? FVector4(
                Material->GetDiffuseColor().x,
                Material->GetDiffuseColor().y,
                Material->GetDiffuseColor().z,
                Material->GetOpacity())
            : FVector4(1, 1, 1, 1);

        TSharedPtr<FTexture2DAsset> SectionTexture = Material ? Material->GetDiffuseTexture() : nullptr;

        if (!SectionTexture && StaticMesh)
        {
            SectionTexture = StaticMesh->GetDiffuseTexture(Section.MaterialName);
        }
        if (!SectionTexture)
        {
            SectionTexture = mTextureAsset;
        }

        FRenderInfo RenderInfo;
        RenderInfo.VertexBuffer = VertexBufferToUse;
        RenderInfo.IndexBuffer = IndexBufferToUse;
        RenderInfo.StartIndex = Section.FirstIndex;
        RenderInfo.IndexCount = Section.IndexCount;
        RenderInfo.Texture = SectionTexture;
        RenderInfo.UVOffset = mUVOffsets[SectionIndex];
        RenderInfo.Model = GetCacheWorldMatrix();
        RenderInfo.Color = Material ? MaterialColor : Color;
        RenderInfo.UseVertexColor = Material == nullptr;
        RenderInfo.ObjectInternalIndex = mOwner->InternalIndex;

        RenderCollector.RenderInfos.Add(std::move(RenderInfo));
    }
}

FAABB UStaticMeshComponent::GetBoundingBox() const
{
    if (!mMeshAsset)
    {
        return FAABB();
    }

    return mMeshAsset->GetLocalBoundingBox().ToWorld(GetCacheWorldMatrix());
}

void UStaticMeshComponent::SetMesh(const TSharedPtr<FStaticMeshAsset>& InMesh)
{
    if (!InMesh)
    {
		mMeshAsset = nullptr;
		mMaterialAssets.Empty();
		mUVOffsets.Empty();
		return;
    }

    const auto& Sections = InMesh->GetSections();
    mMaterialAssets.SetNum(Sections.Num());
    mUVOffsets.SetNum(Sections.Num());
    for (int32 i = 0; i < Sections.Num(); i++)
    {
        auto& Section = Sections[i];
        mMaterialAssets[i] = FAssetManager::Get().GetAssetAs<FMaterialAsset>(Section.MaterialAssetID, true);
    }
    mMeshAsset = InMesh;
}
