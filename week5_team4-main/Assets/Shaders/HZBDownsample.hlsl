// HZB 1단계: 씬 깊이 버퍼를 4x4 블록마다 "가장 먼 깊이(max)"로 줄여서 기록한다.
// 가장 먼 값을 남겨야 "이 영역은 적어도 이 깊이까지 막혀 있다"는 보수적인 판정이 된다.
// (틈이 있으면 배경 깊이 1.0이 올라와서 가려짐으로 판정되지 않는다)

Texture2D<float> SceneDepth : register(t0);

// FRenderPipeline의 공용 입력 레이아웃과 맞추기 위해 입력을 선언만 한다 (정점 버퍼는 바인딩하지 않음)
struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float4 color : COLOR;
	float2 uv : TEXCOORD0;
	uint VertexId : SV_VertexID;
};

struct PS_INPUT
{
	float4 position : SV_POSITION;
};

// 정점 3개로 화면 전체를 덮는 삼각형
PS_INPUT mainVS(VS_INPUT input)
{
	PS_INPUT output;
	float2 uv = float2((input.VertexId << 1) & 2, input.VertexId & 2);
	output.position = float4(uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
	return output;
}

float mainPS(PS_INPUT input) : SV_Target
{
	uint SrcW, SrcH;
	SceneDepth.GetDimensions(SrcW, SrcH);

	int2 Base = int2(input.position.xy) * 4;
	int2 MaxCoord = int2(SrcW, SrcH) - 1;

	float MaxDepth = 0.0f;
	[unroll] for (int y = 0; y < 4; ++y)
	{
		[unroll] for (int x = 0; x < 4; ++x)
		{
			int2 P = min(Base + int2(x, y), MaxCoord);
			MaxDepth = max(MaxDepth, SceneDepth.Load(int3(P, 0)));
		}
	}
	return MaxDepth;
}
