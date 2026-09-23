#include "FRenderPipeline.h"
#include <windows.h>
#include <d3dcompiler.h>
#include "Renderer.h"
#include "FMeshDescription.h"
#include "TMap.h"

FRenderPipeline::FRenderPipeline(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext, FSamplerStatePool* InSamplerStatePool, FDepthStencilStatePool* InDepthStencilStatePool, FBlendStatePool* InBlendStatePool)
	: Device(InDevice)
	, DeviceContext(InDeviceContext)
	, SamplerStatePool(InSamplerStatePool)
	, DepthStencilStatePool(InDepthStencilStatePool)
	, BlendStatePool(InBlendStatePool)
{
	FBlendStateKey Key{ ERenderBlendMode::Opaque, true };
	BlendState = InBlendStatePool->GetOrCreateBlendState(Device, Key);
}

FRenderPipeline::~FRenderPipeline()
{
	Release();
}

void FRenderPipeline::Release()
{
	for (int32 Index = 0; Index < ViewModeCount; ++Index)
	{
		if (RasterizerStates[Index])
		{
			RasterizerStates[Index]->Release();
			RasterizerStates[Index] = nullptr;
		}
	}

	if (VertexShader)
	{
		VertexShader->Release();
		VertexShader = nullptr;
	}

	if (PixelShader)
	{
		PixelShader->Release();
		PixelShader = nullptr;
	}

	if (InputLayout)
	{
		InputLayout->Release();
		InputLayout = nullptr;
	}

	for (int32 Index = 0; Index < ConstantBuffers.Num(); Index++)
	{
		if (ConstantBuffers[Index])
		{
			ConstantBuffers[Index]->Release();
			ConstantBuffers[Index] = nullptr;
		}
	}
}

// 뷰 모드가 요구하는 FillMode. 모드를 추가하면 여기만 고치면 된다.
static D3D11_FILL_MODE GetFillModeForViewMode(EViewModeIndex ViewMode)
{
	switch (ViewMode)
	{
	case EViewModeIndex::VMI_Wireframe:	return D3D11_FILL_WIREFRAME;
	case EViewModeIndex::VMI_Lit:
	case EViewModeIndex::VMI_Unlit:
	default:							return D3D11_FILL_SOLID;
	}
}

void FRenderPipeline::SetRasterRizerState(D3D11_CULL_MODE CullMode, int32 DepthBias,
	std::initializer_list<EViewModeIndex> ViewModes)
{
	for (int32 Index = 0; Index < ViewModeCount; ++Index)
	{
		if (RasterizerStates[Index])
		{
			RasterizerStates[Index]->Release();
			RasterizerStates[Index] = nullptr;
		}
	}

	D3D11_RASTERIZER_DESC RasterizerDesc = {};
	RasterizerDesc.CullMode = CullMode;
	// Clip geometry outside the near/far planes in every projection mode.
	RasterizerDesc.DepthClipEnable = TRUE;
	RasterizerDesc.DepthBias = static_cast<INT>(DepthBias);
	RasterizerDesc.SlopeScaledDepthBias = DepthBias != 0 ? 1.0f : 0.0f;
	RasterizerDesc.DepthClipEnable = TRUE;

	for (EViewModeIndex ViewMode : ViewModes)
	{
		const int32 Index = static_cast<int32>(ViewMode);
		if (Index < 0 || Index >= ViewModeCount)
		{
			continue;   // VMI_Max 같은 센티넬이 들어온 경우
		}

		RasterizerDesc.FillMode = GetFillModeForViewMode(ViewMode);
		Device->CreateRasterizerState(&RasterizerDesc, &RasterizerStates[Index]);
	}

	// Lit은 폴백 대상이라 항상 있어야 한다. 나열에 빠졌으면 솔리드로 채운다.
	const int32 LitIndex = static_cast<int32>(EViewModeIndex::VMI_Lit);
	if (RasterizerStates[LitIndex] == nullptr)
	{
		RasterizerDesc.FillMode = D3D11_FILL_SOLID;
		Device->CreateRasterizerState(&RasterizerDesc, &RasterizerStates[LitIndex]);
	}
}

ID3D11RasterizerState* FRenderPipeline::GetRasterizerState(EViewModeIndex ViewMode) const
{
	const int32 Index = static_cast<int32>(ViewMode);
	if (Index >= 0 && Index < ViewModeCount && RasterizerStates[Index] != nullptr)
	{
		return RasterizerStates[Index];
	}

	// 이 파이프라인이 지원하지 않는 모드다. Lit(솔리드)로 그린다.
	return RasterizerStates[static_cast<int32>(EViewModeIndex::VMI_Lit)];
}

void FRenderPipeline::SetDepthStencilState(bool bEnableDepthTest, bool bEnableDepthWrite)
{
	FDepthStencilStateKey Key{ bEnableDepthTest, bEnableDepthWrite, false, D3D11_COMPARISON_ALWAYS, D3D11_STENCIL_OP_KEEP };
	DepthStencilState = DepthStencilStatePool->GetOrCreateDepthStencilState(Device, Key);
}

void FRenderPipeline::SetDepthStencilState(bool bEnableDepthTest, bool bEnableDepthWrite, D3D11_COMPARISON_FUNC StencilFunc, D3D11_STENCIL_OP StencilPassOp)
{
	FDepthStencilStateKey Key{ bEnableDepthTest, bEnableDepthWrite, true, StencilFunc, StencilPassOp };
	DepthStencilState = DepthStencilStatePool->GetOrCreateDepthStencilState(Device, Key);
}

void FRenderPipeline::SetBlendState(ERenderBlendMode BlendMode, bool bColorWriteEnable)
{
	FBlendStateKey Key{ BlendMode, bColorWriteEnable };
	BlendState = BlendStatePool->GetOrCreateBlendState(Device, Key);
}

void FRenderPipeline::SetShader(const FString& ShaderPath)
{
	std::wstring WShaderPath = Utf2Wide(ShaderPath);

	ID3DBlob* VertexShaderCSO;
	ID3DBlob* PixelShaderCSO;

	D3DCompileFromFile(WShaderPath.c_str(), nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &VertexShaderCSO, nullptr);
	Device->CreateVertexShader(VertexShaderCSO->GetBufferPointer(), VertexShaderCSO->GetBufferSize(), nullptr, &VertexShader);

	D3DCompileFromFile(WShaderPath.c_str(), nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &PixelShaderCSO, nullptr);
	Device->CreatePixelShader(PixelShaderCSO->GetBufferPointer(), PixelShaderCSO->GetBufferSize(), nullptr, &PixelShader);

	D3D11_INPUT_ELEMENT_DESC Layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	Device->CreateInputLayout(Layout, ARRAYSIZE(Layout), VertexShaderCSO->GetBufferPointer(), VertexShaderCSO->GetBufferSize(), &InputLayout);
	Stride = sizeof(FVertex);

	VertexShaderCSO->Release();
	PixelShaderCSO->Release();
}

void FRenderPipeline::SetShaderResource(uint32 Slot, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV)
{
	if (Slot >= ShaderResourceViews.Num())
	{
		ShaderResourceViews.SetNum(Slot + 1);
	}
	ShaderResourceViews[Slot] = SRV.Get();
}

void FRenderPipeline::ClearShaderResource()
{
	ShaderResourceViews.Empty();
}

void FRenderPipeline::SetSamplerState(uint32 Slot, D3D11_FILTER Filter, D3D11_TEXTURE_ADDRESS_MODE AddressU, D3D11_TEXTURE_ADDRESS_MODE AddressV)
{
	if (Slot >= SamplerStates.Num())
	{
		SamplerStates.SetNum(Slot + 1);
	}

	FSamplerStateKey Key{ Filter, AddressU, AddressV };
	ID3D11SamplerState* SamplerState = SamplerStatePool->GetOrCreateSamplerState(Device, Key);

	SamplerStates[Slot] = SamplerState;
}

void FRenderPipeline::ClearSamplerState()
{
	SamplerStates.Empty();
}
