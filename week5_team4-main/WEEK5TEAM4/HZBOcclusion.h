#pragma once

#include <vector>
#include <cmath>
#include <cfloat>
#include <cstring>
#include <algorithm>
#include "Core.h"
#include "Matrix.h"
#include "FAABB.h"

// HZB(Hierarchical Z-Buffer) 오클루전 판정 (CPU 쪽)
//
// 1) GPU가 씬 깊이를 4x4 블록 max로 줄인 텍스처(Level 0)를 만든다 (HZBDownsample.hlsl).
// 2) 그 결과를 1~2프레임 뒤에 CPU로 읽어 와서, 2x2 max로 계속 줄여 피라미드를 만든다.
// 3) 오브젝트 AABB를 "그 깊이를 만들 때 쓴 ViewProjection"으로 투영해서,
//    화면 사각형이 텍셀 2x2 정도에 들어오는 밉에서 가장 먼 깊이와 비교한다.
//    박스의 가장 가까운 깊이가 그보다 뒤에 있으면 가려진 것이다.
//
// 이전 프레임 깊이를 쓰므로, 가려져 있다가 갑자기 드러나는 물체는 1~2프레임 늦게 나타날 수 있다.
struct FHZB
{
	bool bValid = false;
	FMatrix ViewProjection = FMatrix::Identity;   // 이 깊이를 그릴 때 쓴 ViewProjection

	std::vector<std::vector<float>> Mips;
	std::vector<int32> MipWidth;
	std::vector<int32> MipHeight;

	void Invalidate() { bValid = false; }

	// Level0: GPU에서 읽어 온 4x4 max 깊이. RowPitchInFloats는 스테이징 텍스처의 행 간격(float 단위).
	void Build(const float* Level0, int32 Width, int32 Height, int32 RowPitchInFloats, const FMatrix& InViewProjection)
	{
		ViewProjection = InViewProjection;
		Mips.clear(); MipWidth.clear(); MipHeight.clear();

		std::vector<float> Base(size_t(Width) * Height);
		for (int32 y = 0; y < Height; ++y)
		{
			memcpy(&Base[size_t(y) * Width], Level0 + size_t(y) * RowPitchInFloats, sizeof(float) * Width);
		}
		Mips.push_back(std::move(Base)); MipWidth.push_back(Width); MipHeight.push_back(Height);

		while (MipWidth.back() > 1 || MipHeight.back() > 1)
		{
			const int32 PW = MipWidth.back(), PH = MipHeight.back();
			const int32 W = (std::max)(1, (PW + 1) / 2), H = (std::max)(1, (PH + 1) / 2);
			const std::vector<float>& Prev = Mips.back();
			std::vector<float> Next(size_t(W) * H);
			for (int32 y = 0; y < H; ++y)
			{
				const int32 y0 = y * 2, y1 = (std::min)(y * 2 + 1, PH - 1);
				for (int32 x = 0; x < W; ++x)
				{
					const int32 x0 = x * 2, x1 = (std::min)(x * 2 + 1, PW - 1);
					Next[size_t(y) * W + x] = (std::max)((std::max)(Prev[size_t(y0) * PW + x0], Prev[size_t(y0) * PW + x1]),
					                                     (std::max)(Prev[size_t(y1) * PW + x0], Prev[size_t(y1) * PW + x1]));
				}
			}
			Mips.push_back(std::move(Next)); MipWidth.push_back(W); MipHeight.push_back(H);
		}
		bValid = true;
	}

	// 확실히 가려졌을 때만 true. 판단할 수 없으면(카메라 뒤에 걸침, 화면 밖 등) false = 그린다.
	bool IsOccluded(const FAABB& Box) const
	{
		if (!bValid)
		{
			return false;
		}

		float MinX = FLT_MAX, MinY = FLT_MAX, MaxX = -FLT_MAX, MaxY = -FLT_MAX, MinZ = FLT_MAX;
		for (int32 Corner = 0; Corner < 8; ++Corner)
		{
			const FVector4 P((Corner & 1) ? Box.Max.x : Box.Min.x,
			                 (Corner & 2) ? Box.Max.y : Box.Min.y,
			                 (Corner & 4) ? Box.Max.z : Box.Min.z, 1.0f);
			const FVector4 Clip = P * ViewProjection;
			if (Clip.w <= 1e-4f)
			{
				return false;   // 카메라 뒤로 걸친 박스는 판정하지 않는다
			}
			const float InvW = 1.0f / Clip.w;
			const float X = Clip.x * InvW, Y = Clip.y * InvW, Z = Clip.z * InvW;
			MinX = (std::min)(MinX, X); MaxX = (std::max)(MaxX, X);
			MinY = (std::min)(MinY, Y); MaxY = (std::max)(MaxY, Y);
			MinZ = (std::min)(MinZ, Z);
		}

		if (MinZ <= 0.0f)
		{
			return false;   // near 평면 앞쪽에 걸침
		}

		const int32 W0 = MipWidth[0], H0 = MipHeight[0];
		// NDC -> Level0 텍셀 좌표 (y는 위아래 반전)
		float U0 = (MinX * 0.5f + 0.5f) * W0;
		float U1 = (MaxX * 0.5f + 0.5f) * W0;
		float V0 = (0.5f - MaxY * 0.5f) * H0;
		float V1 = (0.5f - MinY * 0.5f) * H0;
		if (U1 < 0.0f || V1 < 0.0f || U0 >= float(W0) || V0 >= float(H0))
		{
			return false;   // 화면 밖은 프러스텀 컬링이 처리한다
		}
		U0 = (std::max)(U0, 0.0f); V0 = (std::max)(V0, 0.0f);
		U1 = (std::min)(U1, float(W0 - 1)); V1 = (std::min)(V1, float(H0 - 1));

		// 사각형이 텍셀 2개 정도에 들어오는 밉 선택
		const float Size = (std::max)(U1 - U0, V1 - V0);
		int32 Level = 0;
		if (Size > 1.0f)
		{
			Level = int32(std::ceil(std::log2(Size))) - 1;
		}
		Level = (std::clamp)(Level, 0, int32(Mips.size()) - 1);

		const int32 W = MipWidth[Level];
		const int32 X0 = int32(U0) >> Level, X1 = int32(U1) >> Level;
		const int32 Y0 = int32(V0) >> Level, Y1 = int32(V1) >> Level;
		const std::vector<float>& Mip = Mips[Level];

		float FarthestOccluder = 0.0f;
		for (int32 y = Y0; y <= Y1; ++y)
		{
			for (int32 x = X0; x <= X1; ++x)
			{
				FarthestOccluder = (std::max)(FarthestOccluder, Mip[size_t(y) * W + x]);
			}
		}

		// 박스에서 가장 가까운 점조차 그 영역을 막고 있는 가장 먼 물체보다 뒤에 있으면 가려짐
		return MinZ > FarthestOccluder;
	}
};
