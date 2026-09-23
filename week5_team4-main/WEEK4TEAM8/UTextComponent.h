#pragma once

#include "SceneComponent.h"
#include "PrimitiveComponent.h"
#include "Assets.h"
#include "Camera.h"
#include "Actor.h"
#include "FAssetManager.h"
#include "ShowFlags.h"
#include "MathUtility.h"
#include "Json/json.hpp"
#include "JsonUtil.h"
#include "FTextBuilder.h"

class UPlaneComponent : public UPrimitiveComponent
{
	REFLECT_CLASS(UPlaneComponent, UPrimitiveComponent)

public:
	UPlaneComponent()
	{
		mMeshAsset = FAssetManager::Get().GetAssetAs<FStaticMeshAsset>(FName("PlaneMesh"), true);
	}

	void SerializeClass(json::JSON& outJson) const override
	{
		UPrimitiveComponent::SerializeClass(outJson);

		if (mTextureAsset)
		{
			FGuid AssetID = mTextureAsset->GetAssetID();
			outJson["Properties"]["ObjTextureAsset"] = JsonUtils::ToJson(AssetID);
		}
	}

	void DeserializeClass(const json::JSON& inJson) override
	{
		UPrimitiveComponent::DeserializeClass(inJson);

		const json::JSON& PropertiesJson = inJson.at("Properties");
		if (!PropertiesJson.hasKey("ObjTextureAsset"))
		{
			throw std::runtime_error("UPlaneComponent: ObjTextureAsset property is required");
		}

		if (PropertiesJson.at("ObjTextureAsset").JSONType() != json::JSON::Class::Object)
		{
			throw std::runtime_error("UPlaneComponent: ObjTextureAsset property requires an object");
		}

		FGuid AssetID = JsonUtils::FromJson<FGuid>(PropertiesJson.at("ObjTextureAsset"));
		if (AssetID.IsValid())
		{
			mTextureAsset = FAssetManager::Get().GetAssetAs<FTexture2DAsset>(AssetID, true);
		}
	}

	void Tick(float DeltaTime) override
	{
		// NOTE: SpotLightComponent의 위치와 회전을 부모 액터에 맞춘다. 현재 Hierarchy가 없으므로 부모 액터의 위치와 회전만 가져와서 적용한다.
		FTransform ParentTransform = mOwner->GetTransform();
		SetRelativeLocation(ParentTransform.Location);
		SetRelativeRotation(ParentTransform.Rotation);
	}

	void Render(FRenderCollector& RenderCollector) override
	{
		if (!FShowFlags::Get().IsEnabled(EShowFlag::Primitive))
		{
			return;
		}

		FTransform PivotTransform = GetTransformMatrix();

		if (mbBillboard && RenderCollector.Camera)
		{
			PivotTransform.Rotation = FRotator::LookAt(PivotTransform.Location, PivotTransform.Location + RenderCollector.Camera->GetForwardVector());
		}

		FRenderQuadInfo QuadInfo;
		QuadInfo.Model = PivotTransform.MakeMatrix();
		QuadInfo.Color = FVector4(1.f, 1.f, 1.f, 1.f);
		QuadInfo.TextureSRV = mTextureAsset ? mTextureAsset->GetSRV() : nullptr;
		QuadInfo.SubUV = mSubUV + FVector4(mSubUVOffset.X, mSubUVOffset.Y, 0.f, 0.f);
		QuadInfo.BlendMode = mBlendMode;
		QuadInfo.EnableDepthTest = mEnableDepthTest;
		QuadInfo.EnableDepthWrite = mEnableDepthWrite;

		RenderCollector.AddQuadInfo(QuadInfo);
	}

	FAABB GetBoundingBox() const override
	{
		if (!mMeshAsset)
		{
			return FAABB();
		}

		return mMeshAsset->GetLocalBoundingBox().ToWorld(GetTransformMatrix().MakeMatrix());
	}

	const TArray<FVertex>& GetMeshVertices() const override
	{
		if (!mMeshAsset)
		{
			return UPrimitiveComponent::GetMeshVertices();
		}
		return mMeshAsset->GetVertices();
	}

	const TArray<uint32>& GetMeshIndices() const override
	{
		if (!mMeshAsset)
		{
			return UPrimitiveComponent::GetMeshIndices();
		}

		return mMeshAsset->GetIndices();
	}

	inline void SetTexture(const TSharedPtr<FTexture2DAsset>& textureAsset) { mTextureAsset = textureAsset; }
	inline const TSharedPtr<FTexture2DAsset>& GetTexture() const { return mTextureAsset; }

	inline void SetBillboard(bool billboard) { mbBillboard = billboard; }
	inline void SetDepthState(bool enableDepthTest, bool enableDepthWrite) { mEnableDepthTest = enableDepthTest; mEnableDepthWrite = enableDepthWrite; }
	void SetBlendState(ERenderBlendMode InBlendMode) { mBlendMode = InBlendMode; }

protected:
	TSharedPtr<FStaticMeshAsset> mMeshAsset;
	TSharedPtr<FTexture2DAsset> mTextureAsset;
	FVector4 mSubUV = { 0.f, 0.f, 1.f, 1.f };
	FVector2 mSubUVOffset = { 0.f, 0.f };

	ERenderBlendMode mBlendMode = ERenderBlendMode::Opaque;
	bool mbBillboard = false;
	bool mEnableDepthTest = true;
	bool mEnableDepthWrite = true;
};

class USpotLightComponent : public USceneComponent
{
	REFLECT_CLASS(USpotLightComponent, USceneComponent)

public:
	void Tick(float DeltaTime) override
	{
		// NOTE: SpotLightComponent의 위치와 회전을 부모 액터에 맞춘다. 현재 Hierarchy가 없으므로 부모 액터의 위치와 회전만 가져와서 적용한다.
		FTransform ParentTransform = mOwner->GetTransform();
		SetRelativeLocation(ParentTransform.Location);
		SetRelativeRotation(ParentTransform.Rotation);
		SetRelativeScale3D(ParentTransform.Scale);
	}

	inline float GetRange() const { return Range; }
	inline float GetInnerConeAngle() const { return mInnerConeAngle; }
	inline float GetOuterConeAngle() const { return mOuterConeAngle; }
	inline const FVector4& GetColor() const { return mColor; }

	inline void SetColor(const FVector4& InColor) { mColor = InColor; }

	inline void SetOuterConeAngle(float InAngle)
	{
		mOuterConeAngle = FMath::Clamp(InAngle, 0.f, MAX_CONE_ANGLE);
		mInnerConeAngle = FMath::Min(mInnerConeAngle, mOuterConeAngle);
	}

	inline void SetInnerConeAngle(float InAngle)
	{
		mInnerConeAngle = FMath::Clamp(InAngle, 0.f, mOuterConeAngle);
	}

private:
	static constexpr float MAX_CONE_ANGLE = 89.f;

	float Range = 5.0f;
	FVector4 mColor = { 1.f, 1.f, 1.f, 1.f };
	float mInnerConeAngle = 30.0f;
	float mOuterConeAngle = 45.0f;
};

class ASpotLight : public AActor
{
	REFLECT_CLASS(ASpotLight, AActor)

public:
	ASpotLight() = default;
	
	void Initialize()
	{
		Super::Initialize();
		USpotLightComponent* SpotLightComponent = FObjectFactory::ConstructObject<USpotLightComponent>(FVector(0, 0, 0), FRotator(0, 0, 0), FVector(1, 1, 1));
		AddRootSceneComponent(SpotLightComponent);
	}

	void DeserializeClass(const json::JSON& inJson) override
	{
		Super::DeserializeClass(inJson);

		for (UActorComponent* Component : GetComponents())
		{
			UPlaneComponent* PlaneComponent = Component->Cast<UPlaneComponent>();

			if (!PlaneComponent)
			{
				continue;
			}

			// SetTexture는 필요 없음

			PlaneComponent->SetBlendState(ERenderBlendMode::Transparent);
			PlaneComponent->SetBillboard(true);
			PlaneComponent->SetDepthState(true, false);
		}
	}

	void CreateEditorComponents() override
	{
		Super::CreateEditorComponents();

		UPlaneComponent* PlaneComponent = FObjectFactory::ConstructObject<UPlaneComponent>(FVector(0, 0, 1), FRotator(0, 0, 0), FVector(1, 1, 1));
		PlaneComponent->SetTexture(FAssetManager::Get().GetAssetAs<FTexture2DAsset>(BuiltInAssetID::SpotLightIcon, true));
		PlaneComponent->SetBillboard(true);
		PlaneComponent->SetBlendState(ERenderBlendMode::Transparent);
		PlaneComponent->SetBillboard(true);
		PlaneComponent->SetDepthState(true, false);
		PlaneComponent->SetEditorOnly(true);
		PlaneComponent->SetDoNotSerialize(true);

		AddComponent(PlaneComponent);
	}
};

class UText3DComponent : public USceneComponent
{
	REFLECT_CLASS(UText3DComponent, USceneComponent)

public:
	UText3DComponent() = default;

	void SerializeClass(json::JSON& outJson) const override
	{
		Super::SerializeClass(outJson);

		// std::wstring을 UTF-8 문자열로 변환하여 저장
		outJson["Properties"]["mText"] = Wide2Utf(mText).CStr();
	}

	void DeserializeClass(const json::JSON& inJson) override
	{
		Super::DeserializeClass(inJson);

		const json::JSON& propertiesJson = inJson.at("Properties");

		// 이전 버전 씬 파일과의 호환성을 위해 필수가 아닌 값으로 처리
		if (propertiesJson.hasKey("mText") && propertiesJson.at("mText").JSONType() == json::JSON::Class::String)
		{
			mText = Utf2Wide(FString(propertiesJson.at("mText").ToString()));
		}
		else
		{
			mText.clear();
		}
	}

	void Tick(float DeltaTime) override
	{
		// NOTE: Text3DComponent의 위치와 회전을 부모 액터에 맞춘다. 현재 Hierarchy 매트릭스 구현이 없으므로 부모 액터의 위치와 회전만 가져와서 적용한다.
		FTransform ParentTransform = mOwner->GetTransform();
		SetRelativeLocation(ParentTransform.Location + FVector(0.f, 0.f, 1.f));
	}

	void Render(FRenderCollector& RenderCollector) override
	{
		// Show Flags에서 끄면 쿼드를 아예 만들지 않는다.
		// 만들고 거르는 게 아니라 글자 수만큼의 계산 자체가 사라진다.
		if (!FShowFlags::Get().IsEnabled(EShowFlag::UUIDText))
		{
			return;
		}

		if (!mFontAtlasAsset)
		{
			return;
		}

		const TSharedPtr<FFontAtlas>& FontAtlas = mFontAtlasAsset->GetFontAtlas();
		if (!FontAtlas)
		{
			return;
		}

		FTextBuilder TextBuilder(FontAtlas, WorldUnitPerPixel);

		float TotalWidth = 0.0f;
		float TotalHeight = 0.0f;
		TextBuilder.CalculateSize(mText, TotalWidth, TotalHeight);

		const FTransform OwnerTransform = mOwner->GetTransform();
		FTransform PivotTransform = GetTransformMatrix();

		UPrimitiveComponent* Primitive = mOwner->GetRootComponent()->Cast<UPrimitiveComponent>();
		if (Primitive)
		{
			const FAABB Bounds = Primitive->GetBoundingBox();

			PivotTransform.Location = FVector(
				PivotTransform.Location.x,
				PivotTransform.Location.y,
				Bounds.Max.z + 0.2f
			);
		}

		if (mbBillboard && RenderCollector.Camera)
		{
			PivotTransform.Rotation = RenderCollector.Camera->Transform.Rotation;
		}

		const FMatrix PivotMatrix = PivotTransform.MakeMatrix();

		TextBuilder.Build(mText, TotalWidth, TotalHeight, [&](const FRect& Rect, const FRect& UV) {
			// 공백 등은 Builder에서 advance만 적용하고, 쿼드는 생략한다.
			if (Rect.Width <= 0.f || Rect.Height <= 0.f)
			{
				return;
			}

			const FVector GlyphCenter(0.f, Rect.X, Rect.Y);

			FRenderQuadInfo QuadInfo;
			QuadInfo.Model = FMatrix::Scale(FVector3(1.f, Rect.Width, Rect.Height)) * FMatrix::Translation(GlyphCenter) * PivotMatrix;
			QuadInfo.Color = mColor;
			QuadInfo.TextureSRV = mFontAtlasAsset->GetSRV();
			QuadInfo.SubUV = FVector4(UV.X, UV.Y, UV.Width, UV.Height);
			QuadInfo.BlendMode = ERenderBlendMode::Transparent;
			QuadInfo.EnableDepthTest = mEnableDepthTest;
			QuadInfo.EnableDepthWrite = mEnableDepthWrite;

			RenderCollector.AddQuadInfo(QuadInfo);
		});
	}

	inline void SetBillboard(bool billboard) { mbBillboard = billboard; }

	inline void SetText(const std::wstring& text) { mText = text; }
	inline const std::wstring& GetText() const { return mText; }

	inline void SetFontAtlasAsset(const TSharedPtr<FFontAtlasAsset>& fontAtlasAsset) { mFontAtlasAsset = fontAtlasAsset; }

	inline void SetColor(const FVector4& color) { mColor = color; }
	inline void SetDepthState(bool enableDepthTest, bool enableDepthWrite) { mEnableDepthTest = enableDepthTest; mEnableDepthWrite = enableDepthWrite; }

private:
	bool mbBillboard = false;
	std::wstring mText;
	TSharedPtr<FFontAtlasAsset> mFontAtlasAsset;
	FVector4 mColor = FVector4(1, 1, 1, 1);
	bool mEnableDepthTest = true;
	bool mEnableDepthWrite = true;
};
