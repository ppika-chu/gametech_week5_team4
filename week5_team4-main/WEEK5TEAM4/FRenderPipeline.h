#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include "Core.h"
#include "TArray.h"
#include "RenderInfo.h"
#include "enum.h"
#include <initializer_list>

class URenderer;
class FSamplerStatePool;
class FDepthStencilStatePool;
class FBlendStatePool;

class FRenderPipeline
{
public:
	FRenderPipeline(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext, FSamplerStatePool* InSamplerStatePool, FDepthStencilStatePool* InDepthStencilStatePool, FBlendStatePool* InBlendStatePool);
	~FRenderPipeline();

	void Release();

	// ViewModes에 나열한 뷰 모드마다 래스터라이저 상태를 하나씩 만든다.
	void SetRasterRizerState(D3D11_CULL_MODE CullMode, int32 DepthBias = 0, std::initializer_list<EViewModeIndex> ViewModes = { EViewModeIndex::VMI_Lit });
	ID3D11RasterizerState* GetRasterizerState(EViewModeIndex ViewMode) const;
	void SetDepthStencilState(bool bEnableDepthTest, bool bEnableDepthWrite);
	void SetDepthStencilState(bool bEnableDepthTest, bool bEnableDepthWrite, D3D11_COMPARISON_FUNC StencilFunc, D3D11_STENCIL_OP StencilPassOp);
	void SetBlendState(ERenderBlendMode BlendMode, bool bColorWriteEnable = true);
	void SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY Topology);
	void SetShader(const FString& ShaderPath);
	
	void SetShaderResource(uint32 Slot, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV);
	void ClearShaderResource();

	void SetSamplerState(uint32 Slot, D3D11_FILTER Filter, D3D11_TEXTURE_ADDRESS_MODE AddressU, D3D11_TEXTURE_ADDRESS_MODE AddressV);
	void ClearSamplerState();

	template <typename T>
	void AddConstantBuffer()
	{
		if (Device)
		{
			D3D11_BUFFER_DESC ConstantBufferDesc = {};
			ConstantBufferDesc.ByteWidth = sizeof(T) + 0xf & 0xfffffff0;
			ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			ID3D11Buffer* ConstantBuffer = nullptr;
			HRESULT Hr = Device->CreateBuffer(&ConstantBufferDesc, nullptr, &ConstantBuffer);
			if (SUCCEEDED(Hr))
			{
				ConstantBuffers.Add(ConstantBuffer);
			}
		}
	}

	template <typename T>
	void UpdateConstantBuffer(uint32 Index, const T& Data)
	{
		if (DeviceContext && Index < ConstantBuffers.Num())
		{
			ID3D11Buffer* ConstantBuffer = ConstantBuffers[Index];
			D3D11_MAPPED_SUBRESOURCE MappedResource;
			HRESULT Hr = DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
			if (SUCCEEDED(Hr))
			{
				memcpy(MappedResource.pData, &Data, sizeof(T));
				DeviceContext->Unmap(ConstantBuffer, 0);
			}
		}
	}

private:
	friend class URenderer;

	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	D3D11_PRIMITIVE_TOPOLOGY PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	FSamplerStatePool* SamplerStatePool = nullptr;
	FDepthStencilStatePool* DepthStencilStatePool = nullptr;
	FBlendStatePool* BlendStatePool = nullptr;
	static constexpr int32 ViewModeCount = static_cast<int32>(EViewModeIndex::VMI_Max);
	ID3D11RasterizerState* RasterizerStates[ViewModeCount] = {};
	ID3D11DepthStencilState* DepthStencilState = nullptr;
	ID3D11InputLayout* InputLayout = nullptr;
	ID3D11BlendState* BlendState = nullptr;
	uint32 Stride = 0;
	ID3D11VertexShader* VertexShader = nullptr;
	ID3D11PixelShader* PixelShader = nullptr;
	TArray<ID3D11Buffer*> ConstantBuffers;
	TArray<ID3D11ShaderResourceView*> ShaderResourceViews;
	TArray<ID3D11SamplerState*> SamplerStates;
};
