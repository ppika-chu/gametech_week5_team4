cbuffer Constants : register(b0) // FConstants
{
	row_major matrix view_projection;
    float3 camera_location;
    float grid_gap;
}

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float3 world_position : TEXCOORD0;
};

// Vertex Shader
PS_INPUT mainVS(uint vertex_id : SV_VertexID)
{
	PS_INPUT output;
	
	float3 positions[6] =
	{
		float3(-1, 1, 0),
		float3(1, 1, 0),
		float3(1, -1, 0),
		float3(-1, 1, 0),
		float3(1, -1, 0),
		float3(-1, -1, 0)
	};

	// Half-size of the camera-centered square, in world units.
	// Keep the original minimum, expand with distance from Z=0, and cover
	// at least ten grid cells in each direction (before camera clipping).
	const float min_half_extent = 50.0f;
	const float height_scale = 4.0f;
	const float half_cell_count = 10.0f;
    float half_extent = max(min_half_extent,
		abs(camera_location.z) * height_scale);
    //,abs(grid_gap) * half_cell_count));
	
	float3 world_position = positions[vertex_id] * half_extent
		+ float3(camera_location.x, camera_location.y, 0.0f);
	
	output.world_position = world_position;
	output.position = mul(float4(world_position, 1.f), view_projection);
	
	return output;
}

// Pixel Shader
float4 mainPS(PS_INPUT input) : SV_TARGET
{
	const float cell_size = grid_gap;
	// World-space half-width: changing grid spacing must not scale the lines.
	const float half_line_width = 0.001f;
	
	float x = input.world_position.x;
	float y = input.world_position.y;
#if 0
	if (abs(x) < half_line_width)
	{
		return float4(0.f, 1.f, 0.f, 1.f); // Red for Y-axis
	}
	else if (abs(y) < half_line_width)
	{
		return float4(1.f, 0.f, 0.f, 1.f); // Green for X-axis
	}

	float x_mod = abs(fmod(x, cell_size));
	float y_mod = abs(fmod(y, cell_size));
	
	if (min(x_mod, cell_size - x_mod) > half_line_width && min(y_mod, cell_size - y_mod) > half_line_width)
	{
		discard;
	}
	
	return float4(0.4f, 0.4f, 0.4f, 1.f);
#else	
	float2 world_xy = float2(x, y);
	float2 grid_xy = world_xy / cell_size;
	float2 nearest_grid_world = round(grid_xy) * cell_size;
	// Measure distance and pixel footprint in the same world-space units.
	// Subtract the nearest line directly to preserve precision for large cells.
	float2 dist_to_nearest_grid = abs(world_xy - nearest_grid_world);
	float2 pixel_width = max(fwidth(world_xy), float2(0.00001f, 0.00001f));
	float2 line_width = (dist_to_nearest_grid - half_line_width) / pixel_width;
	float alpha = 1.f - smoothstep(0.f, 1.f, min(line_width.x, line_width.y));
	// Highlight multiples of 10 world units, independent of the cell spacing.
	uint2 nearest_grid_coordinate = (uint2)round(abs(nearest_grid_world));
	
	float3 color = float3(0.3f, 0.3f, 0.3f);
	if (abs(grid_xy.x) < 0.5f && line_width.x <= line_width.y)
	{
		color = float3(0.f, 1.f, 0.f); // Y축 (x = 0)
	}
	else if (abs(grid_xy.y) < 0.5f && line_width.y <= line_width.x)
	{
		color = float3(1.f, 0.f, 0.f); // X축 (y = 0)
	}
	else if ((nearest_grid_coordinate.x % 10u) == 0 && line_width.x <= line_width.y)
	{
		color = float3(1.0f, 1.0f, 1.0f);
	}
	else if ((nearest_grid_coordinate.y % 10u) == 0 && line_width.y <= line_width.x)
	{
		color = float3(1.0f, 1.0f, 1.0f);
	}

	// Empty grid cells must not write depth and hide later transparent quads.
	if (alpha < 0.01f)
	{
		discard;
	}
	return float4(color, alpha);
#endif
}
