cbuffer Constants : register(b0) // FConstants
{
	row_major matrix projection;
	float4 color;
    float2 position;
    float2 size;
	float4 sub_uv;
	float rotation;
    int has_texture;
    int grayscale_mode;
    int padding;
}

struct PS_INPUT
{
	float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
	float4 color : COLOR;
};

Texture2D main_texture : register(t0);
SamplerState default_sampler : register(s0);

// Vertex Shader
PS_INPUT mainVS(uint vertex_id : SV_VertexID)
{
    PS_INPUT output;
	
    float2 half_size = size * 0.5f;
	
    float2 local_positions[4] =
    {
        -half_size,
		float2(half_size.x, -half_size.y),
		half_size,
		float2(-half_size.x, half_size.y)
    };
	
    float2 uvs[4] =
    {
        float2(0.f, 0.f) * sub_uv.zw + sub_uv.xy, 
		float2(1.f, 0.f) * sub_uv.zw + sub_uv.xy,
		float2(1.f, 1.f) * sub_uv.zw + sub_uv.xy,
		float2(0.f, 1.f) * sub_uv.zw + sub_uv.xy
    };
	
    uint indices[6] =
    {
        0, 1, 2,
		0, 2, 3
    };
	
    float2 local_pos = local_positions[indices[vertex_id]];
    float2 uv = uvs[indices[vertex_id]];
	
    float2x2 rotation_matrix = float2x2(
		cos(rotation), sin(rotation), 
		-sin(rotation), cos(rotation)
	);
	
    float2 world_pos = mul(rotation_matrix, local_pos) + position;

	output.position = mul(float4(world_pos, 0.f, 1.f), projection);
    output.uv = uv;
	output.color = color;
	
	return output;
}

// Pixel Shader
float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float4 final_color = input.color;
    if (has_texture == 1)
    {
        float4 tex_color = main_texture.Sample(default_sampler, input.uv);
		
        if (grayscale_mode == 1)
        {
            final_color.a = tex_color.r;
        }
        else
        {
            final_color = tex_color * input.color;
        }
    }
	
    return final_color;
}
