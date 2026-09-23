cbuffer Constants : register(b0)
{
    float4 outline_color; // RGBA color for the outline
    int2 texture_size;
    int outline_radius;
    int padding;
};

struct PS_INPUT
{
	float4 position : SV_POSITION;
};

Texture2D<uint2> stencil_texture : register(t0);

// Vertex Shader
PS_INPUT mainVS(uint vertex_id : SV_VertexID)
{
	PS_INPUT output;
	
    float2 full_screen_quad[4] =
    {
        float2(-1.0, 1.0),
		float2(1.0, 1.0),
		float2(1.0, -1.0),
		float2(-1.0, -1.0)
    };
    
    uint indices[] = { 0, 1, 2, 0, 2, 3 }; // Two triangles to form a quad
	
    output.position = float4(full_screen_quad[indices[vertex_id]], 0.0, 1.0);

	return output;
}

int ReadStencil(int2 p)
{
    if (any(p < 0) || any(p >= texture_size))
    {
        return 0;
    }
    
    return stencil_texture.Load(int3(p, 0)).g;
}

// Pixel Shader
float4 mainPS(PS_INPUT input) : SV_TARGET
{
    int2 pixel = int2(input.position.xy);
    float origin = ReadStencil(pixel);
    
    if (origin != 0)
    {
        discard;
    }
    
    for (int y = -outline_radius; y <= outline_radius; ++y)
    {
        for (int x = -outline_radius; x <= outline_radius; ++x)
        {
            if (x == 0 && y == 0)
            {
                // Skip origin
                continue;
            }
            
            if (x * x + y * y > outline_radius * outline_radius)
            {
                // Skip pixels outside the outline radius
                continue;
            }
            
            int2 neighbor_pixel = pixel + int2(x, y);
            int neighbor_value = ReadStencil(neighbor_pixel);
            
            if (neighbor_value == 1)
            {
                return outline_color; // Return the outline color if a neighbor is part of the stencil
            }
        }
    }
    
    discard;
   
    return float4(0, 0, 0, 0);
}
