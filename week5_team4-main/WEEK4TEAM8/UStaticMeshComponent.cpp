#include "UStaticMeshComponent.h"

#include "FAssetManager.h"
#include "RenderInfo.h"
#include "ShowFlags.h"
#include "Actor.h"
#include "JsonUtil.h"
#include "EngineMathLibrary.h"
#include "FLogManager.h"

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

void UStaticMeshComponent::Render(FRenderCollector& RenderCollector)
{
    if (!mMeshAsset)
    {
        return;
    }

    if (!FShowFlags::Get().IsEnabled(EShowFlag::Primitive))
    {
        return;
    }

    for (int32 SectionIndex = 0; SectionIndex < mMeshAsset->GetSections().Num(); ++SectionIndex)
    {
        const FStaticMeshSection& Section = mMeshAsset->GetSections()[SectionIndex];

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
        RenderInfo.VertexBuffer = mMeshAsset->GetVertexBuffer();
        RenderInfo.IndexBuffer = mMeshAsset->GetIndexBuffer();
        RenderInfo.StartIndex = Section.FirstIndex;
        RenderInfo.IndexCount = Section.IndexCount;
        RenderInfo.Texture = SectionTexture;
        RenderInfo.UVOffset = mUVOffsets[SectionIndex];
        RenderInfo.Model = GetTransformMatrix().MakeMatrix();
        RenderInfo.Color = Material ? MaterialColor : Color;
        RenderInfo.UseVertexColor = Material == nullptr;
        RenderInfo.ObjectInternalIndex = mOwner->InternalIndex;

        RenderCollector.RenderInfos.Add(RenderInfo);
    }
}

FAABB UStaticMeshComponent::GetBoundingBox() const
{
    if (!mMeshAsset)
    {
        return FAABB();
    }

    return mMeshAsset->GetLocalBoundingBox().ToWorld(GetTransformMatrix().MakeMatrix());
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
