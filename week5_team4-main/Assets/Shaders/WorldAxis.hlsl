cbuffer Constants : register(b0) // FConstants
{
	row_major matrix view;
	row_major matrix projection;
	float4 color;
	float3 axis;
	float thickness; // Full width in world units, matching WorldGrid.hlsl.
	float2 viewport_size;
	float2 padding;
}

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
	noperspective float line_distance : TEXCOORD0;
	noperspective float half_width_pixels : TEXCOORD1;
};

// Clip the center line before dividing by W; either end can be behind the camera.
bool ClipAxisPlane(float start_distance, float end_distance, inout float first, inout float last)
{
	if (start_distance < 0.0f && end_distance < 0.0f)
		return false;
	if (start_distance < 0.0f)
		first = max(first, start_distance / (start_distance - end_distance));
	else if (end_distance < 0.0f)
		last = min(last, start_distance / (start_distance - end_distance));
	return first <= last;
}

// Vertex Shader
PS_INPUT mainVS(uint vertex_id : SV_VertexID)
{
	PS_INPUT output = (PS_INPUT)0;
	output.position = float4(0.0f, 0.0f, -1.0f, 1.0f);

	const float half_length = 10000.0f;
	float4 start = mul(mul(float4(-axis * half_length, 1.0f), view), projection);
	float4 end = mul(mul(float4(axis * half_length, 1.0f), view), projection);
	float first = 0.0f;
	float last = 1.0f;
	if (!ClipAxisPlane(start.z, end.z, first, last))
		return output;
	if (!ClipAxisPlane(start.w - start.z, end.w - end.z, first, last))
		return output;
	if (!ClipAxisPlane(start.w - 0.00001f, end.w - 0.00001f, first, last))
		return output;

	float4 clipped_start = lerp(start, end, first);
	float4 clipped_end = lerp(start, end, last);
	float2 safe_viewport = max(viewport_size, float2(1.0f, 1.0f));
	float2 pixel_delta = (clipped_end.xy / clipped_end.w - clipped_start.xy / clipped_start.w)
		* safe_viewport * 0.5f;
	float pixel_length = length(pixel_delta);
	if (pixel_length < 0.0001f)
		return output; // An axis viewed end-on has no screen-space line direction.
	float2 normal = float2(-pixel_delta.y, pixel_delta.x) / pixel_length;

	// Convert the world-space core width to pixels at each endpoint.
	float2 projection_scale = max(abs(float2(projection[0][0], projection[1][1]))
		* safe_viewport * 0.5f, float2(0.00001f, 0.00001f));
	float pixel_scale = 1.0f / length(normal / projection_scale);
	float half_world_width = max(thickness, 0.0f) * 0.5f;
	float aa_width = abs(normal.x) + abs(normal.y);

	const uint indices[6] = { 0, 1, 2, 0, 2, 3 };
	uint corner = indices[vertex_id];
	float4 center = (corner == 0 || corner == 3) ? clipped_start : clipped_end;
	float half_width_pixels = half_world_width * pixel_scale / center.w;
	float side = corner < 2 ? 1.0f : -1.0f;
	float distance_pixels = side * (half_width_pixels + aa_width);
	center.xy += normal * distance_pixels * 2.0f / safe_viewport * center.w;

	output.position = center;
	output.color = color;
	output.line_distance = distance_pixels;
	output.half_width_pixels = half_width_pixels;
	return output;
}

// Pixel Shader
float4 mainPS(PS_INPUT input) : SV_TARGET
{
	// Same world-width + one derivative footprint falloff as the grid axes.
	float pixel_width = max(fwidth(input.line_distance), 0.00001f);
	float edge = (abs(input.line_distance) - input.half_width_pixels) / pixel_width;
	float alpha = 1.0f - smoothstep(0.0f, 1.0f, edge);
	if (alpha < 0.01f)
		discard;
	return float4(input.color.rgb, input.color.a * alpha);
}
