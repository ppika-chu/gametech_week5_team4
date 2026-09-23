cbuffer ModelConstants : register(b0) // FConstants
{
	row_major matrix Model;
	float4 Color;
    float2 uv_offset;
	int UseVertexColor;
    int HasTexture;
}

cbuffer ViewConstants : register(b1) // FConstants
{
	row_major matrix View;
}

struct VS_INPUT
{
	float4 position : POSITION;
    float3 normal : NORMAL;
	float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

struct PS_INPUT
{
	float4 position : SV_POSITION;
    float3 normal : NORMAL;
	float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

Texture2D main_texture : register(t0);
SamplerState default_sampler : register(s0);

// Vertex Shader
PS_INPUT mainVS(VS_INPUT input)
{
	PS_INPUT output;
    
	output.position = mul(mul(input.position, Model), View);
    output.normal = input.normal;
	
	if (UseVertexColor != 0)
	{
		output.color = input.color;
	}
	else
	{
		output.color = Color;
	}
	
    output.uv = input.uv + uv_offset;
	
	return output;
}

// Pixel Shader
float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float4 final_color = input.color;
    if (HasTexture != 0)
    {
        final_color *= main_texture.Sample(default_sampler, input.uv);
    }
	
	return final_color;
}
