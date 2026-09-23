#pragma once

#include "Core.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include "Matrix.h"
#include "Vector.h"
#include "RenderInfo.h"
#include "FRenderPipeline.h"

struct FCameraConstants
{
	FMatrix ViewProjectionMatrix;
	FVector2 ViewportSize;
	float Padding[2];
};

struct FConstants
{
	FMatrix Matrix;
	FVector4 Color;
	FVector2 UVOffset;
	int32 UseVertexColor;
	int32 HasTexture;
};

struct FLine2DConstants
{
	FMatrix Projection;
	FVector4 Color;
	FVector2 Start;
	FVector2 End;
	float Thickness;
	float Padding[3];
};

struct FCircle2DConstants
{
	FMatrix Projection;
	FVector4 Color;
	FVector2 Center;
	float Radius;
	float Padding[2];
};

struct FTriangle2DConstants
{
	FMatrix Projection;
	FVector4 Color;
	FVector2 Center;
	float Size;
	float Rotation;
};

struct FQuad2DConstants
{
	FMatrix Projection;
	FVector4 Color;
	FVector2 Position;
	FVector2 Size;
	FVector4 SubUV;
	float Rotation;
	int32 HasTexture;
	int32 GrayscaleMode;
	int32 Padding;
};

struct FWorldAxisConstants
{
	FMatrix View;
	FMatrix Projection;
	FVector4 Color;
	FVector Axis;
	float Thickness;
	FVector2 ViewportSize;
	float Padding[2];
};

struct FWorldGridConstants
{
	FMatrix ViewProjection;
	FVector CameraLocation;
	float GridGap = 1.0f;
};

struct FQuadConstants
{
	FMatrix Model;
	FVector4 Color;
	FVector4 SubUV;
	int32 HasTexture;
	int32 GrayscaleMode;
	int32 Padding[2];
};

struct FSamplerStateKey
{
	D3D11_FILTER Filter;
	D3D11_TEXTURE_ADDRESS_MODE AddressU;
	D3D11_TEXTURE_ADDRESS_MODE AddressV;

	bool operator==(const FSamplerStateKey& Other) const
	{
		return Filter == Other.Filter && AddressU == Other.AddressU && AddressV == Other.AddressV;
	}
};

struct FSamplerStateKeyHash
{
	std::size_t operator()(const FSamplerStateKey& Key) const
	{
		return std::hash<int>()(static_cast<int>(Key.Filter)) ^ (std::hash<int>()(static_cast<int>(Key.AddressU)) << 1) ^ (std::hash<int>()(static_cast<int>(Key.AddressV)) << 2);
	}
};

class FSamplerStatePool
{
public:
	ID3D11SamplerState* GetOrCreateSamplerState(ID3D11Device* Device, const FSamplerStateKey& Key)
	{
		ID3D11SamplerState** existing = SamplerStates.Find(Key);
		if (existing)
		{
			return *existing;
		}

		D3D11_SAMPLER_DESC SamplerDesc = {};
		SamplerDesc.Filter = Key.Filter;
		SamplerDesc.AddressU = Key.AddressU;
		SamplerDesc.AddressV = Key.AddressV;
		SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		SamplerDesc.MipLODBias = 0.0f;
		SamplerDesc.MaxAnisotropy = 1;
		SamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		SamplerDesc.BorderColor[0] = 0.0f;
		SamplerDesc.BorderColor[1] = 0.0f;
		SamplerDesc.BorderColor[2] = 0.0f;
		SamplerDesc.BorderColor[3] = 0.0f;
		SamplerDesc.MinLOD = 0.0f;
		SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		ID3D11SamplerState* SamplerState = nullptr;
		HRESULT Hr = Device->CreateSamplerState(&SamplerDesc, &SamplerState);
		if (FAILED(Hr))
		{
			return nullptr;
		}

		SamplerStates.Add(Key, SamplerState);

		return SamplerState;
	}

private:
	friend class URenderer;

	TMap<FSamplerStateKey, ID3D11SamplerState*, FSamplerStateKeyHash> SamplerStates;
};

struct FDepthStencilStateKey
{
	bool bEnableDepthTest;
	bool bEnableDepthWrite;
	bool bEnableStencil = false;
	D3D11_COMPARISON_FUNC StencilFunc = D3D11_COMPARISON_ALWAYS;
	D3D11_STENCIL_OP StencilPassOp = D3D11_STENCIL_OP_KEEP;
	
	bool operator==(const FDepthStencilStateKey& Other) const
	{
		return bEnableDepthTest == Other.bEnableDepthTest
			&& bEnableDepthWrite == Other.bEnableDepthWrite
			&& bEnableStencil == Other.bEnableStencil
			&& StencilFunc == Other.StencilFunc
			&& StencilPassOp == Other.StencilPassOp;
	}
};

struct FDepthStencilStateKeyHash
{
	std::size_t operator()(const FDepthStencilStateKey& Key) const
	{
		return std::hash<bool>()(Key.bEnableDepthTest)
			^ (std::hash<bool>()(Key.bEnableDepthWrite) << 1)
			^ (std::hash<bool>()(Key.bEnableStencil) << 2)
			^ (std::hash<int>()(static_cast<int>(Key.StencilFunc)) << 3)
			^ (std::hash<int>()(static_cast<int>(Key.StencilPassOp)) << 5);
	}
};

class FDepthStencilStatePool
{
public:
	ID3D11DepthStencilState* GetOrCreateDepthStencilState(ID3D11Device* Device, const FDepthStencilStateKey& Key)
	{
		ID3D11DepthStencilState** existing = DepthStencilStates.Find(Key);
		if (existing)
		{
			return *existing;
		}

		D3D11_DEPTH_STENCIL_DESC DepthStencilDesc = {};
		DepthStencilDesc.DepthEnable = Key.bEnableDepthTest ? TRUE : FALSE;
		DepthStencilDesc.DepthWriteMask = Key.bEnableDepthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
		DepthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
		DepthStencilDesc.StencilEnable = Key.bEnableStencil ? TRUE : FALSE;
		DepthStencilDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
		DepthStencilDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

		D3D11_DEPTH_STENCILOP_DESC StencilOpDesc = {};
		StencilOpDesc.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		StencilOpDesc.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		StencilOpDesc.StencilPassOp = Key.StencilPassOp;
		StencilOpDesc.StencilFunc = Key.StencilFunc;

		DepthStencilDesc.FrontFace = StencilOpDesc;
		DepthStencilDesc.BackFace = StencilOpDesc;

		ID3D11DepthStencilState* DepthStencilState = nullptr;
		HRESULT Hr = Device->CreateDepthStencilState(&DepthStencilDesc, &DepthStencilState);
		if (FAILED(Hr))
		{
			return nullptr;
		}

		DepthStencilStates.Add(Key, DepthStencilState);

		return DepthStencilState;
	}

private:
	friend class URenderer;

	TMap<FDepthStencilStateKey, ID3D11DepthStencilState*, FDepthStencilStateKeyHash> DepthStencilStates;
};

struct FBlendStateKey
{
	ERenderBlendMode BlendMode;
	bool bColorWriteEnable = true;

	bool operator==(const FBlendStateKey& Other) const
	{
		return BlendMode == Other.BlendMode && bColorWriteEnable == Other.bColorWriteEnable;
	}
};

struct FBlendStateKeyHash
{
	std::size_t operator()(const FBlendStateKey& Key) const
	{
		return std::hash<int>()(static_cast<int>(Key.BlendMode)) ^ (std::hash<bool>()(Key.bColorWriteEnable) << 1);
	}
};

class FBlendStatePool
{
public:
	ID3D11BlendState* GetOrCreateBlendState(ID3D11Device* Device, const FBlendStateKey& Key)
	{
		ID3D11BlendState** Result = BlendStates.Find(Key);
		if (Result)
		{
			return *Result;
		}

		CD3D11_BLEND_DESC BlendDesc = {};
		BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
		BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
		BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		BlendDesc.RenderTarget[0].RenderTargetWriteMask = Key.bColorWriteEnable ? D3D11_COLOR_WRITE_ENABLE_ALL : 0;

		switch (Key.BlendMode)
		{
		case ERenderBlendMode::Opaque:
		case ERenderBlendMode::Masked:
			BlendDesc.RenderTarget[0].BlendEnable = FALSE;
			break;
		case ERenderBlendMode::Transparent:
			BlendDesc.RenderTarget[0].BlendEnable = TRUE;
			BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			break;
		case ERenderBlendMode::Additive:
			BlendDesc.RenderTarget[0].BlendEnable = TRUE;
			BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
			BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			break;
		}

		ID3D11BlendState* BlendState = nullptr;
		HRESULT Hr = Device->CreateBlendState(&BlendDesc, &BlendState);
		if (FAILED(Hr))
		{
			return nullptr;
		}

		BlendStates.Add(Key, BlendState);

		return BlendState;
	}

private:
	friend class URenderer;

	TMap<FBlendStateKey, ID3D11BlendState*, FBlendStateKeyHash> BlendStates;
};

struct FRenderTarget2D
{
	Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV;
	UINT Width;
	UINT Height;
};

struct FDepthStencil
{
	Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> DSV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV;
	UINT Width;
	UINT Height;
};

struct FVertexBuffer
{
	ID3D11DeviceContext* DeviceContext;
	
	Microsoft::WRL::ComPtr<ID3D11Buffer> Buffer;
	UINT VertexSize;
	UINT VertexCount;

	void UpdateBuffer(const void* Data, uint32 DataCount)
	{
		D3D11_MAPPED_SUBRESOURCE Mapped{};
		DeviceContext->Map(Buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
		std::memcpy(Mapped.pData, Data, DataCount * VertexSize);
		DeviceContext->Unmap(Buffer.Get(), 0);
	}

	inline uint32 GetBufferSize() const
	{
		return VertexSize * VertexCount;
	}
};

struct FIndexBuffer
{
	ID3D11DeviceContext* DeviceContext;
	
	Microsoft::WRL::ComPtr<ID3D11Buffer> Buffer;
	UINT IndexCount;

	void UpdateBuffer(const void* Data, uint32 DataCount)
	{
		D3D11_MAPPED_SUBRESOURCE Mapped{};
		DeviceContext->Map(Buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
		std::memcpy(Mapped.pData, Data, DataCount * sizeof(uint32));
		DeviceContext->Unmap(Buffer.Get(), 0);
	}

	inline uint32 GetBufferSize() const
	{
		return sizeof(uint32) * IndexCount;
	}
};

struct FStructuredBuffer
{
	ID3D11DeviceContext* DeviceContext;

	Microsoft::WRL::ComPtr<ID3D11Buffer> Buffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV;
	UINT ElementSize;
	UINT ElementCount;

	void UpdateBuffer(const void* Data, uint32 DataCount)
	{
		D3D11_BOX Box = {};
		Box.left = 0;
		Box.right = DataCount * ElementSize;
		Box.top = 0;
		Box.bottom = 1;
		Box.front = 0;
		Box.back = 1;

		DeviceContext->UpdateSubresource(Buffer.Get(), 0, &Box, Data, 0, 0);
	}
};

class URenderer
{
public:
	//create
	void Create(HWND hWindow);
	void Release();

	template <typename T>
	TSharedPtr<FVertexBuffer> CreateVertexBuffer(const T* Vertices, UINT Count, D3D11_USAGE Usage = D3D11_USAGE_IMMUTABLE)
	{
		D3D11_BUFFER_DESC VertexBufferDesc = {};
		VertexBufferDesc.ByteWidth = sizeof(T) * Count;
		VertexBufferDesc.Usage = Usage;
		VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		VertexBufferDesc.CPUAccessFlags = (Usage == D3D11_USAGE_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0;

		TSharedPtr<FVertexBuffer> VertexBuffer = MakeShared<FVertexBuffer>();
		VertexBuffer->DeviceContext = DeviceContext;
		VertexBuffer->VertexSize = sizeof(T);
		VertexBuffer->VertexCount = Count;

		if (Vertices)
		{
			D3D11_SUBRESOURCE_DATA VertexBufferSRD = { Vertices };
			Device->CreateBuffer(&VertexBufferDesc, &VertexBufferSRD, VertexBuffer->Buffer.GetAddressOf());
		}
		else
		{
			Device->CreateBuffer(&VertexBufferDesc, nullptr, VertexBuffer->Buffer.GetAddressOf());
		}

		return VertexBuffer;
	}

	TSharedPtr<FIndexBuffer> CreateIndexBuffer(const uint32* Indices, UINT Count, D3D11_USAGE Usage = D3D11_USAGE_IMMUTABLE);

	Microsoft::WRL::ComPtr<ID3D11Texture2D> CreateTexture2D(const D3D11_TEXTURE2D_DESC& Desc, const void* InitialData = nullptr);
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateShaderResourceView(Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture, const D3D11_SHADER_RESOURCE_VIEW_DESC* Desc = nullptr);

	template <typename T>
	TSharedPtr<FStructuredBuffer> CreateStructuredBuffer(uint32 ElementCount)
	{
		TSharedPtr<FStructuredBuffer> StructuredBuffer = MakeShared<FStructuredBuffer>();
		StructuredBuffer->DeviceContext = DeviceContext;

		D3D11_BUFFER_DESC StructuredBufferDesc = {};
		StructuredBufferDesc.ByteWidth = sizeof(T) * ElementCount;
		StructuredBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		StructuredBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		StructuredBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		StructuredBufferDesc.StructureByteStride = sizeof(T);

		Device->CreateBuffer(&StructuredBufferDesc, nullptr, StructuredBuffer->Buffer.GetAddressOf());

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		SRVDesc.Buffer.FirstElement = 0;
		SRVDesc.Buffer.NumElements = ElementCount;

		Device->CreateShaderResourceView(StructuredBuffer->Buffer.Get(), &SRVDesc, StructuredBuffer->SRV.GetAddressOf());

		StructuredBuffer->ElementSize = sizeof(T);
		StructuredBuffer->ElementCount = ElementCount;

		return StructuredBuffer;
	}

	TSharedPtr<FRenderTarget2D> CreateRenderTarget2D(uint32 Width, uint32 Height, DXGI_FORMAT Format);
	TSharedPtr<FDepthStencil> CreateDepthStencil(uint32 Width, uint32 Height);
	
	//Rendering
	void Prepare(const FMatrix& ViewProjectionMatrix);

	TSharedPtr<FRenderPipeline> CreateRenderPipeline();

	void BindFrameBuffer();
	void BindRenderTarget(const TSharedPtr<FRenderTarget2D>& RenderTarget, const TSharedPtr<FDepthStencil>& DepthStencil, bool bClear = true);

	void Render(const TSharedPtr<FRenderPipeline>& Pipeline, UINT NumVertices) const;

	void RenderLines(const TArray<FRenderLineInfo>& Lines) const;

	void RenderQuad(const FRenderQuadInfo& Info) const;

	void RenderPrimitive(const TSharedPtr<FRenderPipeline>& Pipeline, Microsoft::WRL::ComPtr<ID3D11Buffer> Buffer, UINT NumVertices) const;
	void RenderPrimitive(Microsoft::WRL::ComPtr<ID3D11Buffer> Buffer, UINT NumVertices, const FMatrix& Model) const;
	void RenderPrimitive(Microsoft::WRL::ComPtr<ID3D11Buffer> Buffer, UINT NumVertices, const FMatrix& Model, const FVector4& Color) const;
	void RenderPrimitiveIndexed(const FRenderInfo& RenderInfo, uint32 StencilRef = 0) const;
	void RenderPrimitiveIndexed(const TSharedPtr<FRenderPipeline>& Pipeline, const FRenderInfo& RenderInfo, uint32 StencilRef = 0) const;

	void RenderQuad2D(const FRenderQuad2DInfo& Info) const;
	void RenderLine2D(const FVector2& Start, const FVector2& End, const FVector4& Color, float Thickness = 1.0f) const;
	void RenderCircle2D(const FVector2& Center, const FVector4& Color, float Radius = 1.0f) const;
	void RenderTriangle2D(const FVector2& Center, const FVector4& Color, float Size = 1.0f, float Rotation = 0.0f) const;

	void RenderWorldAxis(const FMatrix& View, const FMatrix& Projection, const FVector4& Color, const FVector& Axis, float Thickness = 0.002f) const;
	void RenderWorldGrid(const FMatrix& ViewProjection, const FVector& CameraLocation, float GridGap) const;

	void ClearAllShaderResources() const;

	void SwapBuffer();

	//해상도 변경 시 호출
	void OnResize(UINT width, UINT height);

	FORCEINLINE uint32 GetWidth() const { return Width; }
	FORCEINLINE uint32 GetHeight() const { return Height; }
	FORCEINLINE const D3D11_VIEWPORT& GetViewport() const { return ViewportInfo; }
	FORCEINLINE ID3D11Device* GetDevice() const { return Device; }
	FORCEINLINE ID3D11DeviceContext* GetDeviceContext() const { return DeviceContext; }
	FORCEINLINE void SetViewModeIndex(EViewModeIndex InViewModeIndex) { ViewModeIndex = InViewModeIndex; }
	FORCEINLINE TSharedPtr<FRenderTarget2D> GetBindedRenderTarget() const { return BindedRenderTarget; }
	FORCEINLINE TSharedPtr<FDepthStencil> GetBindedDepthStencil() const { return BindedDepthStencil; }

	mutable uint64 DrawCallCount = 0;
	uint64 GetDrawCallCount() const { return DrawCallCount; }
	void ResetDrawCallCount() { DrawCallCount = 0; }
private:
	void CreateDeviceAndSwapChain(HWND hWindow);
	void ReleaseDeviceAndSwapChain();

	void CreateFrameBuffer();
	void ReleaseFrameBuffer();

	void CreateDepthStencilBuffer();

	void BindPipeline(const TSharedPtr<FRenderPipeline>& Pipeline, uint32 StencilRef = 0) const;

private:
    ID3D11Device* Device = nullptr;
    ID3D11DeviceContext* DeviceContext = nullptr;
    IDXGISwapChain* SwapChain = nullptr;

	FSamplerStatePool SamplerStatePool;
	FDepthStencilStatePool DepthStencilStatePool;
	FBlendStatePool BlendStatePool;

    ID3D11Texture2D* FrameBuffer = nullptr;
    ID3D11RenderTargetView* FrameBufferRTV = nullptr;

	ID3D11Texture2D* DepthStencilBuffer = nullptr;			// 실제 깊이값이 저장될 메모리
	ID3D11DepthStencilView* DepthStencilView = nullptr;		// 그 메모리를 "출력 대상"으로 보는 뷰

	TSharedPtr<FRenderTarget2D> BindedRenderTarget;
	TSharedPtr<FDepthStencil> BindedDepthStencil;

	TSharedPtr<FStructuredBuffer> LineStructuredBuffer;

	TSharedPtr<FRenderPipeline> LinePipeline;
	TSharedPtr<FRenderPipeline> PrimitivePipeline;
	TSharedPtr<FRenderPipeline> Line2DPipeline;
	TSharedPtr<FRenderPipeline> Circle2DPipeline;
	TSharedPtr<FRenderPipeline> Triangle2DPipeline;
	TSharedPtr<FRenderPipeline> WorldAxisPipeline;
	TSharedPtr<FRenderPipeline> WorldGridPipeline;
	TSharedPtr<FRenderPipeline> QuadPipeline;
	TSharedPtr<FRenderPipeline> Quad2DPipeline;

	UINT Width, Height;
    FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };
    D3D11_VIEWPORT ViewportInfo;
	FMatrix Projection2D;

	// 와이어프레임 여부. Prepare에서 갱신하고 BindPipeline이 읽는다.
	// RSSetState는 드로우 직전마다 덮어써지므로 플래그로 들고 있어야 한다.
	EViewModeIndex ViewModeIndex = EViewModeIndex::VMI_Lit;
};
