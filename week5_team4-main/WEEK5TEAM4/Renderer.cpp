#include "Renderer.h"

constexpr uint32 MaxLineInstances = 1024;

namespace
{
	UINT GetByteSizeFromFormat(DXGI_FORMAT Format)
	{
		switch (Format)
		{
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
			return 16;
		case DXGI_FORMAT_R32G32B32_FLOAT:
			return 12;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
			return 8;
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return 4;
		default:
			return 0; // Unknown format
		}
	}
}

void URenderer::Create(HWND hWindow)
{
	CreateDeviceAndSwapChain(hWindow);
	CreateFrameBuffer();
	CreateDepthStencilBuffer();

	LineStructuredBuffer = CreateStructuredBuffer<FRenderLineInfo>(MaxLineInstances);

	LinePipeline = CreateRenderPipeline();
	LinePipeline->SetRasterRizerState(D3D11_CULL_NONE);
	LinePipeline->SetShader("Assets/Shaders/Line.hlsl");
	LinePipeline->AddConstantBuffer<FCameraConstants>();
	LinePipeline->SetShaderResource(0, LineStructuredBuffer->SRV);

	PrimitivePipeline = CreateRenderPipeline();
	PrimitivePipeline->SetRasterRizerState(D3D11_CULL_BACK, 0, {EViewModeIndex::VMI_Lit, EViewModeIndex::VMI_Wireframe});
	PrimitivePipeline->SetShader("Assets/Shaders/StaticMeshShader.hlsl");
	PrimitivePipeline->AddConstantBuffer<FConstants>();
	PrimitivePipeline->AddConstantBuffer<FMatrix>();
	PrimitivePipeline->SetSamplerState(0, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);

	Line2DPipeline = CreateRenderPipeline();
	Line2DPipeline->SetRasterRizerState(D3D11_CULL_NONE);
	Line2DPipeline->SetDepthStencilState(false, false);
	Line2DPipeline->SetShader("Assets/Shaders/Line2D.hlsl");
	Line2DPipeline->AddConstantBuffer<FLine2DConstants>();

	Circle2DPipeline = CreateRenderPipeline();
	Circle2DPipeline->SetRasterRizerState(D3D11_CULL_NONE);
	Circle2DPipeline->SetDepthStencilState(false, false);
	Circle2DPipeline->SetShader("Assets/Shaders/Circle2D.hlsl");
	Circle2DPipeline->AddConstantBuffer<FCircle2DConstants>();

	Triangle2DPipeline = CreateRenderPipeline();
	Triangle2DPipeline->SetRasterRizerState(D3D11_CULL_NONE);
	Triangle2DPipeline->SetDepthStencilState(false, false);
	Triangle2DPipeline->SetShader("Assets/Shaders/Triangle2D.hlsl");
	Triangle2DPipeline->AddConstantBuffer<FTriangle2DConstants>();

	Quad2DPipeline = CreateRenderPipeline();
	Quad2DPipeline->SetRasterRizerState(D3D11_CULL_NONE);
	Quad2DPipeline->SetDepthStencilState(false, false);
	Quad2DPipeline->SetBlendState(ERenderBlendMode::Transparent);
	Quad2DPipeline->SetShader("Assets/Shaders/Quad2D.hlsl");
	Quad2DPipeline->AddConstantBuffer<FQuad2DConstants>();
	Quad2DPipeline->SetSamplerState(0, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);

	WorldAxisPipeline = CreateRenderPipeline();
	WorldAxisPipeline->SetRasterRizerState(D3D11_CULL_NONE);
	WorldAxisPipeline->SetBlendState(ERenderBlendMode::Transparent);
	WorldAxisPipeline->SetShader("Assets/Shaders/WorldAxis.hlsl");
	WorldAxisPipeline->AddConstantBuffer<FWorldAxisConstants>();

	WorldGridPipeline = CreateRenderPipeline();
	WorldGridPipeline->SetRasterRizerState(D3D11_CULL_NONE);
	WorldGridPipeline->SetDepthStencilState(true, true);
	WorldGridPipeline->SetBlendState(ERenderBlendMode::Transparent);
	WorldGridPipeline->SetShader("Assets/Shaders/WorldGrid.hlsl");
	WorldGridPipeline->AddConstantBuffer<FWorldGridConstants>();

	QuadPipeline = CreateRenderPipeline();
	QuadPipeline->SetRasterRizerState(D3D11_CULL_NONE);
	QuadPipeline->SetDepthStencilState(false, true);
	QuadPipeline->SetBlendState(ERenderBlendMode::Transparent);
	QuadPipeline->SetShader("Assets/Shaders/Quad.hlsl");
	QuadPipeline->AddConstantBuffer<FQuadConstants>();
	QuadPipeline->AddConstantBuffer<FMatrix>();
	QuadPipeline->SetSamplerState(0, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
}

void URenderer::CreateDeviceAndSwapChain(HWND hWindow)
{
	D3D_FEATURE_LEVEL FeatureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

	DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
	SwapChainDesc.BufferDesc.Width = 0;
	SwapChainDesc.BufferDesc.Height = 0;
	SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.BufferCount = 2;
	SwapChainDesc.OutputWindow = hWindow;
	SwapChainDesc.Windowed = TRUE;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	UINT CreateDeviceFlags = 0;

#if defined(_DEBUG)
	CreateDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
		nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT | CreateDeviceFlags,
		FeatureLevels, ARRAYSIZE(FeatureLevels), D3D11_SDK_VERSION,
		&SwapChainDesc, &SwapChain, &Device, nullptr, &DeviceContext);

	SwapChain->GetDesc(&SwapChainDesc);
	Width = SwapChainDesc.BufferDesc.Width;
	Height = SwapChainDesc.BufferDesc.Height;
	ViewportInfo = { 0.0f, 0.0f, (float)Width, (float)Height, 0.0f, 1.0f };
	Projection2D = FMatrix::Ortho(0.f, Width, Height, 0.f, 0.0f, 1.0f);
}

void URenderer::ReleaseDeviceAndSwapChain()
{
	if (DeviceContext)
	{
		DeviceContext->Flush();
	}

	if (SwapChain)
	{
		SwapChain->Release();
		SwapChain = nullptr;
	}

	if (Device)
	{
		Device->Release();
		Device = nullptr;
	}

	if (DeviceContext)
	{
		DeviceContext->Release();
		DeviceContext = nullptr;
	}
}

void URenderer::CreateFrameBuffer()
{
	SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

	D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
	framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

	Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV);
}

void URenderer::ReleaseFrameBuffer()
{
	if (FrameBuffer)
	{
		FrameBuffer->Release();
		FrameBuffer = nullptr;
	}

	if (FrameBufferRTV)
	{
		FrameBufferRTV->Release();
		FrameBufferRTV = nullptr;
	}
}

void URenderer::Release()
{
	DeviceContext->ClearState();

	WorldGridPipeline.reset();
	WorldAxisPipeline.reset();
	Quad2DPipeline.reset();
	Triangle2DPipeline.reset();
	Circle2DPipeline.reset();
	Line2DPipeline.reset();
	PrimitivePipeline.reset();
	LinePipeline.reset();
	QuadPipeline.reset();
	LineStructuredBuffer.reset();

	for (auto& Pair : SamplerStatePool.SamplerStates)
	{
		Pair.second->Release();
	}
	SamplerStatePool.SamplerStates.Empty();

	for (auto& Pair : DepthStencilStatePool.DepthStencilStates)
	{
		Pair.second->Release();
	}
	DepthStencilStatePool.DepthStencilStates.Empty();

	for (auto& Pair : BlendStatePool.BlendStates)
	{
		Pair.second->Release();
	}
	BlendStatePool.BlendStates.Empty();	

	DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	DepthStencilView->Release();
	DepthStencilBuffer->Release();
	ReleaseFrameBuffer();
	ReleaseDeviceAndSwapChain();
}

void URenderer::SwapBuffer()
{
	SwapChain->Present(1, 0);
}

void URenderer::Prepare(const FMatrix& ViewProjectionMatrix)
{
	DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);
	DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	DeviceContext->RSSetViewports(1, &ViewportInfo);

	DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, DepthStencilView);
	DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);

	FCameraConstants CameraConstants;
	CameraConstants.ViewProjectionMatrix = ViewProjectionMatrix;
	CameraConstants.ViewportSize = FVector2((float)Width, (float)Height);

	LinePipeline->UpdateConstantBuffer(0, CameraConstants);
	PrimitivePipeline->UpdateConstantBuffer(1, ViewProjectionMatrix);
	QuadPipeline->UpdateConstantBuffer(1, ViewProjectionMatrix);

}

TSharedPtr<FIndexBuffer> URenderer::CreateIndexBuffer(const uint32* Indices, UINT Count, D3D11_USAGE Usage)
{
	D3D11_BUFFER_DESC IndexBufferDesc = {};
	IndexBufferDesc.ByteWidth = Count * sizeof(uint32);
	IndexBufferDesc.Usage = Usage;
	IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	IndexBufferDesc.CPUAccessFlags = (Usage == D3D11_USAGE_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0;

	TSharedPtr<FIndexBuffer> IndexBuffer = MakeShared<FIndexBuffer>();
	if (Indices)
	{
		D3D11_SUBRESOURCE_DATA IndexBufferSRD = { Indices };
		Device->CreateBuffer(&IndexBufferDesc, &IndexBufferSRD, IndexBuffer->Buffer.GetAddressOf());
	}
	else
	{
		Device->CreateBuffer(&IndexBufferDesc, nullptr, IndexBuffer->Buffer.GetAddressOf());
	}

	IndexBuffer->DeviceContext = DeviceContext;
	IndexBuffer->Buffer = IndexBuffer->Buffer;
	IndexBuffer->IndexCount = Count;

	return IndexBuffer;
}

Microsoft::WRL::ComPtr<ID3D11Texture2D> URenderer::CreateTexture2D(const D3D11_TEXTURE2D_DESC& Desc, const void* InitialData)
{
	Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture;

	if (InitialData)
	{
		D3D11_SUBRESOURCE_DATA TextureData = {};
		TextureData.pSysMem = InitialData;
		TextureData.SysMemPitch = Desc.Width * GetByteSizeFromFormat(Desc.Format);

		Device->CreateTexture2D(&Desc, &TextureData, &Texture);
	}
	else
	{
		Device->CreateTexture2D(&Desc, nullptr, &Texture);
	}

	return Texture;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> URenderer::CreateShaderResourceView(Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture, const D3D11_SHADER_RESOURCE_VIEW_DESC* Desc)
{
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV;
	Device->CreateShaderResourceView(Texture.Get(), Desc, &SRV);
	return SRV;
}

TSharedPtr<FRenderPipeline> URenderer::CreateRenderPipeline()
{
	return MakeShared<FRenderPipeline>(Device, DeviceContext, &SamplerStatePool, &DepthStencilStatePool, &BlendStatePool);
}

TSharedPtr<FRenderTarget2D> URenderer::CreateRenderTarget2D(uint32 Width, uint32 Height, DXGI_FORMAT Format)
{
	TSharedPtr<FRenderTarget2D> RenderTarget = MakeShared<FRenderTarget2D>();

	D3D11_TEXTURE2D_DESC TextureDesc = {};
	TextureDesc.Width = Width;
	TextureDesc.Height = Height;
	TextureDesc.MipLevels = 1;
	TextureDesc.ArraySize = 1;
	TextureDesc.Format = Format;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.Usage = D3D11_USAGE_DEFAULT;
	TextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	RenderTarget->Texture = CreateTexture2D(TextureDesc);

	D3D11_RENDER_TARGET_VIEW_DESC RTVDesc = {};
	RTVDesc.Format = Format;
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	Device->CreateRenderTargetView(RenderTarget->Texture.Get(), &RTVDesc, RenderTarget->RTV.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = Format;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.MipLevels = 1;
	Device->CreateShaderResourceView(RenderTarget->Texture.Get(), &SRVDesc, RenderTarget->SRV.GetAddressOf());

	RenderTarget->Width = Width;
	RenderTarget->Height = Height;

	return RenderTarget;
}

TSharedPtr<FDepthStencil> URenderer::CreateDepthStencil(uint32 Width, uint32 Height)
{
	TSharedPtr<FDepthStencil> DepthStencil = MakeShared<FDepthStencil>();

	D3D11_TEXTURE2D_DESC TextureDesc = {};
	TextureDesc.Width = Width;
	TextureDesc.Height = Height;
	TextureDesc.MipLevels = 1;
	TextureDesc.ArraySize = 1;
	TextureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.Usage = D3D11_USAGE_DEFAULT;
	TextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

	DepthStencil->Texture = CreateTexture2D(TextureDesc);

	D3D11_DEPTH_STENCIL_VIEW_DESC DsvDesc = {};
	DsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	DsvDesc.Texture2D.MipSlice = 0;
	Device->CreateDepthStencilView(DepthStencil->Texture.Get(), &DsvDesc, DepthStencil->DSV.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
	SRVDesc.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.MipLevels = 1;
	Device->CreateShaderResourceView(DepthStencil->Texture.Get(), &SRVDesc, DepthStencil->SRV.GetAddressOf());

	DepthStencil->Width = Width;
	DepthStencil->Height = Height;

	return DepthStencil;
}

void URenderer::BindPipeline(const TSharedPtr<FRenderPipeline>& Pipeline, uint32 StencilRef) const
{
	// RSSetState는 드로우 직전마다 갈아치워지므로 뷰 모드 선택은 여기서 해야 한다.
	// 이 모드를 지원하지 않는 파이프라인(2D/기즈모)은 Lit 상태로 폴백된다.
	DeviceContext->RSSetState(Pipeline->GetRasterizerState(ViewModeIndex));
	DeviceContext->OMSetDepthStencilState(Pipeline->DepthStencilState, StencilRef);
	DeviceContext->OMSetBlendState(Pipeline->BlendState, nullptr, 0xffffffff);
	DeviceContext->IASetPrimitiveTopology(Pipeline->PrimitiveTopology);
	DeviceContext->IASetInputLayout(Pipeline->InputLayout);
	DeviceContext->VSSetShader(Pipeline->VertexShader, nullptr, 0);
	DeviceContext->PSSetShader(Pipeline->PixelShader, nullptr, 0);
	
	if (Pipeline->ConstantBuffers.Num())
	{
		DeviceContext->VSSetConstantBuffers(0, Pipeline->ConstantBuffers.Num(), &Pipeline->ConstantBuffers[0]);
		DeviceContext->PSSetConstantBuffers(0, Pipeline->ConstantBuffers.Num(), &Pipeline->ConstantBuffers[0]);
	}
	else
	{
		ID3D11Buffer* nullCB = nullptr;
		DeviceContext->VSSetConstantBuffers(0, 1, &nullCB);
		DeviceContext->PSSetConstantBuffers(0, 1, &nullCB);
	}

	if (Pipeline->ShaderResourceViews.Num())
	{
		DeviceContext->VSSetShaderResources(0, Pipeline->ShaderResourceViews.Num(), &Pipeline->ShaderResourceViews[0]);
		DeviceContext->PSSetShaderResources(0, Pipeline->ShaderResourceViews.Num(), &Pipeline->ShaderResourceViews[0]);
	}
	else
	{
		ID3D11ShaderResourceView* nullSRV = nullptr;
		DeviceContext->VSSetShaderResources(0, 1, &nullSRV);
		DeviceContext->PSSetShaderResources(0, 1, &nullSRV);
	}

	if (Pipeline->SamplerStates.Num())
	{
		DeviceContext->PSSetSamplers(0, Pipeline->SamplerStates.Num(), &Pipeline->SamplerStates[0]);
	}
	else
	{
		ID3D11SamplerState* nullSampler = nullptr;
		DeviceContext->PSSetSamplers(0, 1, &nullSampler);
	}
}

void URenderer::BindFrameBuffer()
{
	DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, nullptr);
	DeviceContext->RSSetViewports(1, &ViewportInfo);

	Projection2D = FMatrix::Ortho(0.f, Width, Height, 0.f, 0.0f, 1.0f);

	BindedRenderTarget = nullptr;
	BindedDepthStencil = nullptr;
}

void URenderer::BindRenderTarget(const TSharedPtr<FRenderTarget2D>& RenderTarget, const TSharedPtr<FDepthStencil>& DepthStencil, bool bClear)
{
	DeviceContext->OMSetRenderTargets(1, RenderTarget->RTV.GetAddressOf(), DepthStencil ? DepthStencil->DSV.Get() : nullptr);
	if (bClear)
	{
		DeviceContext->ClearRenderTargetView(RenderTarget->RTV.Get(), ClearColor);

		if (DepthStencil)
		{
			DeviceContext->ClearDepthStencilView(DepthStencil->DSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		}
	}

	D3D11_VIEWPORT Viewport = {};
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.Width = static_cast<float>(RenderTarget->Width);
	Viewport.Height = static_cast<float>(RenderTarget->Height);
	Viewport.MinDepth = 0.0f;
	Viewport.MaxDepth = 1.0f;

	DeviceContext->RSSetViewports(1, &Viewport);

	Projection2D = FMatrix::Ortho(0.f, RenderTarget->Width, RenderTarget->Height, 0.f, 0.0f, 1.0f);

	BindedRenderTarget = RenderTarget;
	BindedDepthStencil = DepthStencil;
}

void URenderer::Render(const TSharedPtr<FRenderPipeline>& Pipeline, UINT NumVertices) const
{
	BindPipeline(Pipeline);

	UINT Offset = 0;
	ID3D11Buffer* NullVB = nullptr;
	UINT Stride = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &NullVB, &Stride, &Offset);
	DeviceContext->Draw(NumVertices, 0);
	++DrawCallCount;
}

void URenderer::RenderLines(const TArray<FRenderLineInfo>& Lines) const
{
	uint32 Remaining = Lines.Num();
	const FRenderLineInfo* Offset = Lines.Data();

	BindPipeline(LinePipeline);

	while (Remaining > 0)
	{
		uint32 BatchSize = FGenericPlatformMath::Min(Remaining, MaxLineInstances);
		LineStructuredBuffer->UpdateBuffer(Offset, BatchSize);

		UINT OffsetIndex = 0;
		DeviceContext->IASetVertexBuffers(0, 0, NULL, NULL, &OffsetIndex);
		DeviceContext->DrawInstanced(6, BatchSize, 0, 0);
		++DrawCallCount;

		Remaining -= BatchSize;
		Offset += BatchSize;
	}
}

void URenderer::RenderQuad(const FRenderQuadInfo& Info) const
{
	QuadPipeline->SetShaderResource(0, Info.TextureSRV);
	
	DXGI_FORMAT TextureFormat = DXGI_FORMAT_UNKNOWN;
	if (Info.TextureSRV)
	{

		D3D11_SHADER_RESOURCE_VIEW_DESC Desc{};
		Info.TextureSRV->GetDesc(&Desc);

		TextureFormat = Desc.Format;
	}

	QuadPipeline->SetBlendState(Info.BlendMode);
	QuadPipeline->SetDepthStencilState(Info.EnableDepthTest, Info.EnableDepthWrite);

	BindPipeline(QuadPipeline);

	QuadPipeline->UpdateConstantBuffer(0, FQuadConstants{ Info.Model, Info.Color, Info.SubUV, Info.TextureSRV ? 1 : 0, TextureFormat == DXGI_FORMAT_R8_UNORM });

	UINT Offset = 0;
	ID3D11Buffer* NullVB = nullptr;
	UINT Stride = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &NullVB, &Stride, &Offset);
	DeviceContext->Draw(6, 0);
	++DrawCallCount;
}

void URenderer::RenderPrimitive(const TSharedPtr<FRenderPipeline>& Pipeline, Microsoft::WRL::ComPtr<ID3D11Buffer> Buffer, UINT NumVertices) const
{
	BindPipeline(Pipeline);

	UINT Offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, Buffer.GetAddressOf(), &Pipeline->Stride, &Offset);
	DeviceContext->Draw(NumVertices, 0);
	++DrawCallCount;
}

void URenderer::RenderPrimitive(Microsoft::WRL::ComPtr<ID3D11Buffer> Buffer, UINT NumVertices, const FMatrix& Model) const
{
	FConstants Constants;
	Constants.Matrix = Model;
	Constants.Color = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	Constants.UVOffset = FVector2(0.0f, 0.0f);
	Constants.UseVertexColor = 1;
	Constants.HasTexture = 0;

	PrimitivePipeline->UpdateConstantBuffer(0, Constants);

	RenderPrimitive(PrimitivePipeline, Buffer, NumVertices);
}

void URenderer::RenderPrimitive(Microsoft::WRL::ComPtr<ID3D11Buffer> Buffer, UINT NumVertices, const FMatrix& Model, const FVector4& Color) const
{
	FConstants Constants;
	Constants.Matrix = Model;
	Constants.Color = Color;
	Constants.UVOffset = FVector2(0.0f, 0.0f);
	Constants.UseVertexColor = 1;
	Constants.HasTexture = 0;

	PrimitivePipeline->UpdateConstantBuffer(0, Constants);

	RenderPrimitive(PrimitivePipeline, Buffer, NumVertices);
}

void URenderer::RenderPrimitiveIndexed(const FRenderInfo& RenderInfo, uint32 StencilRef) const
{
	FConstants Constants;
	Constants.Matrix = RenderInfo.Model;
	Constants.Color = RenderInfo.Color;
	Constants.UVOffset = RenderInfo.UVOffset;
	Constants.UseVertexColor = RenderInfo.UseVertexColor;
	Constants.HasTexture = RenderInfo.Texture ? 1 : 0;

	PrimitivePipeline->UpdateConstantBuffer(0, Constants);

	RenderPrimitiveIndexed(PrimitivePipeline, RenderInfo, StencilRef);
}

void URenderer::RenderPrimitiveIndexed(const TSharedPtr<FRenderPipeline>& Pipeline, const FRenderInfo& RenderInfo, uint32 StencilRef) const
{
	BindPipeline(Pipeline, StencilRef);

	UINT Offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, RenderInfo.VertexBuffer.GetAddressOf(), &Pipeline->Stride, &Offset);

	if (RenderInfo.IndexBuffer)
	{
		DeviceContext->IASetIndexBuffer(RenderInfo.IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		DeviceContext->DrawIndexed(RenderInfo.IndexCount, RenderInfo.StartIndex, 0);
	}
	else
	{
		DeviceContext->Draw(RenderInfo.VertexCount, 0);
	}
	++DrawCallCount;
}

void URenderer::RenderQuad2D(const FRenderQuad2DInfo& Info) const
{
	Quad2DPipeline->SetShaderResource(0, Info.TextureSRV);

	DXGI_FORMAT TextureFormat = DXGI_FORMAT_UNKNOWN;
	if (Info.TextureSRV)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC Desc{};
		Info.TextureSRV->GetDesc(&Desc);

		TextureFormat = Desc.Format;
	}

	Quad2DPipeline->UpdateConstantBuffer(0, FQuad2DConstants{ Projection2D, Info.Color, Info.Position, Info.Size, Info.SubUV, Info.Rotation, Info.TextureSRV ? 1 : 0, TextureFormat == DXGI_FORMAT_R8_UNORM });

	BindPipeline(Quad2DPipeline);

	UINT Offset = 0;
	ID3D11Buffer* NullVB = nullptr;
	UINT Stride = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &NullVB, &Stride, &Offset);
	DeviceContext->Draw(6, 0);
}

void URenderer::RenderLine2D(const FVector2& Start, const FVector2& End, const FVector4& Color, float Thickness) const
{
	Line2DPipeline->UpdateConstantBuffer(0, FLine2DConstants{ Projection2D, Color, Start, End, Thickness });

	BindPipeline(Line2DPipeline);

	UINT Offset = 0;
	ID3D11Buffer* NullVB = nullptr;
	UINT Stride = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &NullVB, &Stride, &Offset);
	DeviceContext->Draw(6, 0);
	++DrawCallCount;
}

void URenderer::RenderCircle2D(const FVector2& Center, const FVector4& Color, float Radius) const
{
	Circle2DPipeline->UpdateConstantBuffer(0, FCircle2DConstants{ Projection2D, Color, Center, Radius });

	BindPipeline(Circle2DPipeline);

	UINT Offset = 0;
	ID3D11Buffer* NullVB = nullptr;
	UINT Stride = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &NullVB, &Stride, &Offset);
	DeviceContext->Draw(6, 0);
	DrawCallCount++;
}

void URenderer::RenderTriangle2D(const FVector2& Center, const FVector4& Color, float Size, float Rotation) const
{
	Triangle2DPipeline->UpdateConstantBuffer(0, FTriangle2DConstants{ Projection2D, Color, Center, Size, Rotation - PI * 0.5f });

	BindPipeline(Triangle2DPipeline);

	UINT Offset = 0;
	ID3D11Buffer* NullVB = nullptr;
	UINT Stride = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &NullVB, &Stride, &Offset);
	DeviceContext->Draw(3, 0);
	++DrawCallCount;
}

void URenderer::RenderWorldAxis(const FMatrix& View, const FMatrix& Projection, const FVector4& Color, const FVector& Axis, float Thickness) const
{
	// Use the scene viewport currently bound, which may differ from the window size.
	D3D11_VIEWPORT Viewport = {};
	UINT ViewportCount = 1;
	DeviceContext->RSGetViewports(&ViewportCount, &Viewport);
	WorldAxisPipeline->UpdateConstantBuffer(0, FWorldAxisConstants{
		View, Projection, Color, Axis, Thickness, FVector2(Viewport.Width, Viewport.Height) });

	BindPipeline(WorldAxisPipeline);

	UINT Offset = 0;
	ID3D11Buffer* NullVB = nullptr;
	UINT Stride = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &NullVB, &Stride, &Offset);
	DeviceContext->Draw(6, 0);
	++DrawCallCount;
}

void URenderer::RenderWorldGrid(const FMatrix& ViewProjection, const FVector& CameraLocation, float GridGap) const
{
	WorldGridPipeline->UpdateConstantBuffer(0, FWorldGridConstants{ ViewProjection, CameraLocation, GridGap });

	BindPipeline(WorldGridPipeline);

	UINT Offset = 0;
	ID3D11Buffer* NullVB = nullptr;
	UINT Stride = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &NullVB, &Stride, &Offset);
	DeviceContext->Draw(6, 0);
	++DrawCallCount;
}

void URenderer::ClearAllShaderResources() const
{
	ID3D11ShaderResourceView* nullSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};
	DeviceContext->VSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSRVs);
	DeviceContext->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSRVs);
}

//=============================================
void URenderer::CreateDepthStencilBuffer()
{
	D3D11_TEXTURE2D_DESC DepthTextureDesc = {};
	DepthTextureDesc.Width = Width;
	DepthTextureDesc.Height = Height;
	DepthTextureDesc.MipLevels = 1;
	DepthTextureDesc.ArraySize = 1;
	DepthTextureDesc.SampleDesc.Count = 1;
	DepthTextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DepthTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	Device->CreateTexture2D(&DepthTextureDesc, nullptr, &DepthStencilBuffer);

	D3D11_DEPTH_STENCIL_VIEW_DESC DsvDesc = {};
	DsvDesc.Format = DepthTextureDesc.Format;
	DsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

	Device->CreateDepthStencilView(DepthStencilBuffer, &DsvDesc, &DepthStencilView);
}

void URenderer::OnResize(UINT width, UINT height)
{
	if (!SwapChain || width == 0 || height == 0) return;

	DeviceContext->OMSetRenderTargets(0, 0, 0);

	FrameBuffer->Release();
	FrameBufferRTV->Release();
	DepthStencilBuffer->Release();
	DepthStencilView->Release();

	SwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

	Width = width;
	Height = height;
	ViewportInfo = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	Projection2D = FMatrix::Ortho(0.f, Width, Height, 0.f, 0.0f, 1.0f);

	CreateFrameBuffer();
	CreateDepthStencilBuffer();
}

