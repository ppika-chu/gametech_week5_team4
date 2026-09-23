#pragma once

#include "Core.h"
#include "FAsset.h"
#include "FFontAtlas.h"
#include "TArray.h"
#include "Vector.h"
#include "Matrix.h"
#include "FAABB.h"
#include "FObjImporter.h"
#include "FGuid.h"
#include "FMeshDescription.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <filesystem>
#include <ft2build.h>
#include FT_FREETYPE_H

class FFontManager;
class URenderer;
class FAssetManager;
struct FVertexBuffer;
struct FIndexBuffer;

namespace BuiltInAssetID
{
	inline const FGuid CubeMesh(0xB17B0001, 0x00000000, 0x00000000, 0x00000001);
	inline const FGuid SphereMesh(0xB17B0001, 0x00000000, 0x00000000, 0x00000002);
	inline const FGuid PlaneMesh(0xB17B0001, 0x00000000, 0x00000000, 0x00000003);
	inline const FGuid CircleMesh(0xB17B0001, 0x00000000, 0x00000000, 0x00000004);
	inline const FGuid ConeMesh(0xB17B0001, 0x00000000, 0x00000000, 0x00000005);
	inline const FGuid GizmoArrowMesh(0xB17B0001, 0x00000000, 0x00000000, 0x00000006);
	inline const FGuid DefaultFont(0xB17B0001, 0x00000000, 0x00000000, 0x00000100);
	inline const FGuid TriangleMesh(0xB17B0001, 0x00000000, 0x00000000, 0x00001000);
	inline const FGuid ExplosionSpriteAtlas(0xB17B0001, 0x00000000, 0x00000000, 0x00002000); // NOTE: 현재 텍스쳐를 통해 스프라이트 아틀라스를 생성하는 에디터 기능이 없어서 임시로 고정 Guid를 부여함. 후에 에디터에서 스프라이트 아틀라스를 생성할 수 있는 기능이 생기면 제거해야 함.
	inline const FGuid ExplosionTexture(0xB17B0001, 0x00000000, 0x00000000, 0x00002001);
	inline const FGuid SpotLightIcon(0xB17B0001, 0x00000000, 0x00000000, 0x00003000);
}

class FFileAssetSource : public FAssetSource
{
public:
	FFileAssetSource(const std::filesystem::path& InFilePath) : FilePath(InFilePath) {}

	TSharedPtr<FArchive> CreateArchive() override;

private:
	std::filesystem::path FilePath;
};

class FStaticMeshAsset : public FAsset
{
public:
	FStaticMeshAsset() = default;
	FStaticMeshAsset(const FGuid& InAssetID, const FName& InAssetName, URenderer& InRenderer, const FVertex* InVertices, uint32 InVertexCount);
	FStaticMeshAsset(const FGuid& InAssetID, const FName& InAssetName, URenderer& InRenderer, const FVertex* InVertices, uint32 InVertexCount, const uint32* InIndices, uint32 InIndexCount);
	FStaticMeshAsset(const FGuid& InAssetID, const FName& InAssetName, URenderer& InRenderer, const FStaticMeshBuildData& InBuildData);

	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer() const;
	uint32 GetVertexCount() const;
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer() const;
	uint32 GetIndexCount() const;
	inline uint32 GetSubMeshCount() const { return Sections.Num(); }
	inline const FAABB& GetLocalBoundingBox() const { return BoundingBox; }
	inline const TArray<FStaticMeshSection>& GetSections() const { return Sections; }
	inline const TArray<FVertex>& GetVertices() const { return Vertices; }
	inline const TArray<uint32>& GetIndices() const { return Indices; }

private:
	TSharedPtr<FVertexBuffer> VertexBuffer;
	
	TSharedPtr<FIndexBuffer> IndexBuffer;

	FAABB BoundingBox;

	TArray<FVertex> Vertices;
	TArray<uint32> Indices;

	TArray<FStaticMeshSection> Sections;
};

class FStaticMeshAssetLoader : public FAssetLoader
{
public:
	FStaticMeshAssetLoader(URenderer& InRenderer) : Renderer(InRenderer) {}
	~FStaticMeshAssetLoader() = default;

	virtual TSharedPtr<FAsset> LoadAsset(const FGuid& AssetID, const FName& AssetName, FArchive& Ar) override;
	virtual void UnloadAsset(TSharedPtr<FAsset> Asset) override;
	virtual EAssetType GetAssetType() const override { return EAssetType::StaticMesh; }

private:
	URenderer& Renderer;
};

class FTexture2DAsset : public FAsset
{
public:
	FTexture2DAsset() = default;
	FTexture2DAsset(const FGuid& InAssetID, const FName& InAssetName, Microsoft::WRL::ComPtr<ID3D11Texture2D> InTexture, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> InSRV)
		: FTexture2DAsset(InAssetID, InAssetName, EAssetType::Texture2D, InTexture, InSRV)
	{
	}

	inline Microsoft::WRL::ComPtr<ID3D11Texture2D> GetTexture() const { return Texture; }
	inline Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSRV() const { return SRV; }

	inline uint32 GetWidth() const { return Width; }
	inline uint32 GetHeight() const { return Height; }
	inline DXGI_FORMAT GetFormat() const { return Format; }

protected:
	FTexture2DAsset(const FGuid& InAssetID, const FName& InAssetName, EAssetType InAssetType, Microsoft::WRL::ComPtr<ID3D11Texture2D> InTexture, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> InSRV)
		: FAsset(InAssetID, InAssetName, InAssetType)
		, Texture(InTexture)
		, SRV(InSRV)
	{
		if (Texture)
		{
			D3D11_TEXTURE2D_DESC TextureDesc = {};
			Texture->GetDesc(&TextureDesc);

			Width = TextureDesc.Width;
			Height = TextureDesc.Height;
			Format = TextureDesc.Format;
		}
	}

protected:
	Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV;

	uint32 Width = 0;
	uint32 Height = 0;
	DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
};

class FTexture2DAssetLoader : public FAssetLoader
{
public:
	FTexture2DAssetLoader(URenderer& InRenderer) : Renderer(InRenderer) {}
	~FTexture2DAssetLoader() = default;

	virtual TSharedPtr<FAsset> LoadAsset(const FGuid& AssetID, const FName& AssetName, FArchive& Ar) override;
	virtual void UnloadAsset(TSharedPtr<FAsset> Asset) override;
	virtual EAssetType GetAssetType() const override { return EAssetType::Texture2D; }

private:
	URenderer& Renderer;
};

class FFontAsset : public FAsset
{
public:
	FFontAsset() = default;
	FFontAsset(const FGuid& InAssetID, const FName& InAssetName, FT_Face InFace, TArray<int8>&& InFileData)
		: FAsset(InAssetID, InAssetName, EAssetType::Font)
		, Face(InFace)
		, FileData(std::move(InFileData))
	{
	}

	~FFontAsset()
	{
		if (Face)
		{
			FT_Done_Face(Face);
		}
	}

	inline FT_Face GetFace() const { return Face; }

private:
	TArray<int8> FileData;
	FT_Face Face;
};

class FFontAssetLoader : public FAssetLoader
{
public:
	FFontAssetLoader(FFontManager& InFontManager) : FontManager(InFontManager) {}
	~FFontAssetLoader() = default;

	virtual TSharedPtr<FAsset> LoadAsset(const FGuid& AssetID, const FName& AssetName, FArchive& Ar) override;
	virtual void UnloadAsset(TSharedPtr<FAsset> Asset) override;
	virtual EAssetType GetAssetType() const override { return EAssetType::Font; }

private:
	FFontManager& FontManager;
};

class FFontAtlasAsset : public FTexture2DAsset, private FFontAtlasHandler
{
public:
	FFontAtlasAsset(const FGuid& InAssetID, const FName& InAssetName, URenderer& InRenderer, TSharedPtr<FFontAsset>& InFontAsset, uint32 InWidth, uint32 InHeight, uint32 InPaddingW, uint32 InPaddingH);

	inline TSharedPtr<FFontAtlas> GetFontAtlas() const { return FontAtlas; }
	void UpdateRegion(uint32 Left, uint32 Top, uint32 Right, uint32 Bottom, const void* Data, uint32 RowPitch);

protected:
	URenderer& Renderer;
private:
	bool HandleAddGlyph(FFontAtlas& FontAtlas, const FFontGlyph& InGlyph, const FFontGlyphBitmap& InBitmap) override;

private:
	TSharedPtr<FFontAsset> FontAsset;
	TSharedPtr<FFontAtlas> FontAtlas;
};

//Texture2DAsset을 받아 UV를 계산 후 저장하는 에셋
class FSpriteAtlasAsset : public FTexture2DAsset
{
public:
	//Cols. Rows : 아틀라스 텍스쳐에 들어가있는 스프라이트 col x row
	FSpriteAtlasAsset(const FGuid& InAssetID, const FName& InAssetName, URenderer& InRenderer, const TSharedPtr<FTexture2DAsset>& InSource, uint32 InCols, uint32 InRows, uint32 InFrameCount = 0);

	//FrameSUbUV : (시작 UV.x, 시작 UV.y, width, height)
	FSpriteAtlasAsset(const FGuid& InAssetID, const FName& InAssetName, URenderer& InRenderer, const TSharedPtr<FTexture2DAsset>& InSource, const TArray<FVector4>& InFrameSubUVs);

	inline int32 GetFrameCount() const { return FrameSubUVs.Num(); }
	const FVector4& GetFrameSubUV(int32 FrameIndex) const;

protected:
	URenderer& Renderer;
private:
	TArray<FVector4> FrameSubUVs;
};

class FMaterialAsset : public FAsset
{
public:
	FMaterialAsset(const FGuid& InAssetID, const FName& InAssetName, const FVector& InAmbientColor, const FVector& InDiffuseColor, 
		const FVector& InSpecularColor, const FGuid& InDiffuseTexture, const FGuid& InSpecularTexture, const FGuid& InNormalTexture, const float InOpacity)
		: FAsset(InAssetID, InAssetName, EAssetType::Material)
		, AmbientColor(InAmbientColor)
		, DiffuseColor(InDiffuseColor)
		, SpecularColor(InSpecularColor)
		, DiffuseTexture(InDiffuseTexture)
		, SpecularTexture(InSpecularTexture)
		, NormalTexture(InNormalTexture)
		, Opacity(InOpacity)
	{
	}

	const FVector& GetDiffuseColor() const { return DiffuseColor; }
	const float& GetOpacity() const { return Opacity; }

	inline bool HasDiffuseTexture() const { return DiffuseTexture.IsValid(); }
	TSharedPtr<FTexture2DAsset> GetDiffuseTexture() const;

	inline bool HasSpecularTexture() const { return SpecularTexture.IsValid(); }
	TSharedPtr<FTexture2DAsset> GetSpecularTexture() const;

	inline bool HasNormalTexture() const { return NormalTexture.IsValid(); }
	TSharedPtr<FTexture2DAsset> GetNormalTexture() const;

private:
	FVector AmbientColor;
	FVector DiffuseColor;
	FVector SpecularColor;
	FGuid DiffuseTexture;
	FGuid SpecularTexture;
	FGuid NormalTexture;
	float Opacity;
};

class FMaterialAssetLoader : public FAssetLoader
{
public:
	FMaterialAssetLoader() = default;
	~FMaterialAssetLoader() = default;

	virtual TSharedPtr<FAsset> LoadAsset(const FGuid& AssetID, const FName& AssetName, FArchive& Ar) override;
	virtual void UnloadAsset(TSharedPtr<FAsset> Asset) override;
	virtual EAssetType GetAssetType() const override { return EAssetType::Material; }
};
