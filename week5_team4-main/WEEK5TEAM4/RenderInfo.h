#pragma once

#include "Transform.h"
#include "Object.h"
#include "FName.h"
#include "Assets.h"
#include "TArray.h"

class FCamera;
class UPrimitiveComponent;

enum class ERenderBlendMode
{
	Opaque,
	Masked,
	Transparent,
	Additive,
	Count
};

struct FRenderInfo
{
	Microsoft::WRL::ComPtr<ID3D11Buffer> VertexBuffer;
	uint32 VertexCount = 0;
	Microsoft::WRL::ComPtr<ID3D11Buffer> IndexBuffer;
	uint32 StartIndex = 0;
	uint32 IndexCount = 0;
	TSharedPtr<FTexture2DAsset> Texture;
	FVector2 UVOffset = { 0.f, 0.f };
	FMatrix Model;
	uint32 ObjectInternalIndex;
	FVector4 Color = { 1.f, 1.f, 1.f, 1.f };
	bool UseVertexColor = true;
};

struct FRenderQuadInfo
{
	FMatrix Model;
	FVector4 Color = { 1.f, 1.f, 1.f, 1.f };
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> TextureSRV;
	FVector4 SubUV = { 0.f, 0.f, 1.f, 1.f };
	ERenderBlendMode BlendMode = ERenderBlendMode::Opaque;
	bool EnableDepthTest = true;
	bool EnableDepthWrite = true;
};

struct FRenderQuad2DInfo
{
	FVector2 Position;
	FVector2 Size;
	FVector4 Color = { 1.f, 1.f, 1.f, 1.f };
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> TextureSRV;
	FVector4 SubUV = { 0.f, 0.f, 1.f, 1.f };
	float Rotation = 0.f;
	ERenderBlendMode BlendMode = ERenderBlendMode::Opaque;
};

struct FRenderLineInfo
{
	FVector4 Color;
	FVector3 Start;
	float Thickness;
	FVector3 End;
	float Padding;
};

// 이번 프레임에 그릴 것들을 한데 모은다. 소유자는 FGraphicsManager.
struct FRenderCollector
{
public:
	enum { DEFAULT_RESERVE_MEM = 1024U };

	FCamera* Camera = nullptr;

	TArray<FRenderInfo>     RenderInfos;   // 메시 패스
	TArray<FRenderLineInfo> LineInfos;     // 라인 패스
	TArray<UPrimitiveComponent*> PickTargets;

	inline void AddQuadInfo(const FRenderQuadInfo& QuadInfo)
	{
		if (QuadInfo.EnableDepthTest)
		{
			if (QuadInfo.EnableDepthWrite)
			{
				OpaqueQuadInfos.Add(QuadInfo);
			}
			else
			{
				TransparentQuadInfos.Add(QuadInfo);
			}
		}
		else
		{
			OverlayQuadInfos.Add(QuadInfo);
		}
	}

	inline void AddQuad2DInfo(const FRenderQuad2DInfo& Quad2DInfo)
	{
		Quad2DInfos.Add(Quad2DInfo);
	}

	inline void Clear()
	{
		RenderInfos.Empty();
		LineInfos.Empty();
		PickTargets.Empty();
		OpaqueQuadInfos.Empty();
		TransparentQuadInfos.Empty();
		OverlayQuadInfos.Empty();
		Quad2DInfos.Empty();
	}

	inline const TArray<FRenderQuadInfo>& GetOpaqueQuadInfos() const { return OpaqueQuadInfos; }
	inline const TArray<FRenderQuadInfo>& GetTransparentQuadInfos() const { return TransparentQuadInfos; }
	inline const TArray<FRenderQuadInfo>& GetOverlayQuadInfos() const { return OverlayQuadInfos; }
	inline const TArray<FRenderQuad2DInfo>& GetQuad2DInfos() const { return Quad2DInfos; }

private:
	TArray<FRenderQuadInfo> OpaqueQuadInfos;
	TArray<FRenderQuadInfo> TransparentQuadInfos;
	TArray<FRenderQuadInfo> OverlayQuadInfos;

	TArray<FRenderQuad2DInfo> Quad2DInfos;
};