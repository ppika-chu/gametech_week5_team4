#pragma once

#include "Core.h"
#include "FFontAtlas.h"
#include "enum.h"

class FTextBuilder
{
public:
	FTextBuilder(const TSharedPtr<FFontAtlas>& fontAtlas, float unitFactor = 1.0f)
		: mFontAtlas(fontAtlas)
		, mUnitFactor(unitFactor)
	{
	}

	inline void SetCoordinateSpace(ECoordinateSpace coordinateSpace)
	{
		mCoordinateSpace = coordinateSpace;
	}

	void CalculateSize(const std::wstring& Text, float& OutWidth, float& OutHeight)
	{
		const float LineHeight = mFontAtlas->LineHeight() * mUnitFactor;
		const float Ascender = mFontAtlas->Ascender() * mUnitFactor;
		const float Descender = mFontAtlas->Descender() * mUnitFactor;

		OutWidth = 0.0f;
		OutHeight = 0.0f;
		uint32 LineCount = 1;

		float CurrentLineWidth = 0.0f;
		for (wchar_t C : Text)
		{
			if (C == L'\n')
			{
				OutWidth = FPlatformMath::Max(OutWidth, CurrentLineWidth);
				LineCount++;
				CurrentLineWidth = 0.0f;
				continue;
			}

			if (!mFontAtlas->HasGlyph(C))
			{
				mFontAtlas->AddGlyph(C);
			}

			const FFontGlyph& Glyph = mFontAtlas->GetGlyph(C);

			CurrentLineWidth += Glyph.AdvanceX * mUnitFactor;
		}

		OutWidth = FPlatformMath::Max(OutWidth, CurrentLineWidth);
		OutHeight = (Ascender - Descender) + (LineCount - 1) * LineHeight;
	}

	template <typename Func>
	void Build(const std::wstring& Text, float Width, float Height, Func&& Callback)
	{
		const float LineHeight = mFontAtlas->LineHeight() * mUnitFactor;
		const float Ascender = mFontAtlas->Ascender() * mUnitFactor;
		const float Descender = mFontAtlas->Descender() * mUnitFactor;

		FVector2 TextLocation = FVector2(-Width * 0.5f, Height * 0.5f - Ascender);
		for (wchar_t C : Text)
		{
			if (C == L'\n')
			{
				TextLocation.X = -Width * 0.5f;
				TextLocation.Y -= LineHeight;
				continue;
			}

			if (!mFontAtlas->HasGlyph(C))
			{
				continue;
			}

			const FFontGlyph& Glyph = mFontAtlas->GetGlyph(C);

			float GlyphWidth = Glyph.Width * mUnitFactor;
			float GlyphHeight = Glyph.Height * mUnitFactor;
			float AdvanceX = Glyph.AdvanceX * mUnitFactor;
			float BearingX = Glyph.BearingX * mUnitFactor;
			float BearingY = Glyph.BearingY * mUnitFactor;

			FVector2 GlyphCenter(TextLocation.X + BearingX + GlyphWidth * 0.5f, TextLocation.Y + BearingY - GlyphHeight * 0.5f);

			FRect TextRect;
			TextRect.X = GlyphCenter.X;
			TextRect.Y = mCoordinateSpace == ECoordinateSpace::World ? GlyphCenter.Y : -GlyphCenter.Y;
			TextRect.Width = GlyphWidth;
			TextRect.Height = GlyphHeight;

			FRect SubUVRect;
			SubUVRect.X = Glyph.SubUV.x;
			SubUVRect.Y = Glyph.SubUV.y;
			SubUVRect.Width = Glyph.SubUV.z;
			SubUVRect.Height = Glyph.SubUV.w;

			Callback(TextRect, SubUVRect);

			TextLocation.X += AdvanceX;
		}
	}

private:
	ECoordinateSpace mCoordinateSpace = ECoordinateSpace::World;
	TSharedPtr<FFontAtlas> mFontAtlas;
	float mUnitFactor = 1.0f;
};

#if 0 // NOTE: 스크린에 텍스트를 렌더링하는 예제입니다. (월드는 UText3DComponent를 참조하세요.)
TSharedPtr<FFontAtlasAsset> FontAtlasAsset = FAssetManager::Get().GetAssetAs<FFontAtlasAsset>(FName("TestFontAtlas"), true);
FTextBuilder TextBuilder(FontAtlasAsset->GetFontAtlas());
TextBuilder.SetCoordinateSpace(ECoordinateSpace::Screen);

float Width = 0.f;
float Height = 0.f;
TextBuilder.CalculateSize(L"Hello, World!\nThis is a test.", Width, Height);

FVector2 TextLocation = FVector2(CurrentViewport->Window->Rect.Width * 0.5f, CurrentViewport->Window->Rect.Height * 0.5f);
TextBuilder.Build(L"Hello, World!\nThis is a test.", Width, Height, [&](const FRect& TextRect, const FRect& SubUVRect) {
	if (TextRect.Width <= 0.f || TextRect.Height <= 0.f)
	{
		return;
	}

	FRenderQuad2DInfo Quad2DInfo;
	Quad2DInfo.Position = FVector2(TextRect.X, TextRect.Y) + TextLocation;
	Quad2DInfo.Size = { TextRect.Width, TextRect.Height };
	Quad2DInfo.Color = { 1.f, 0.f, 1.f, 1.f };
	Quad2DInfo.TextureSRV = FontAtlasAsset->GetSRV();
	Quad2DInfo.SubUV = { SubUVRect.X, SubUVRect.Y, SubUVRect.Width, SubUVRect.Height };
	Quad2DInfo.BlendMode = ERenderBlendMode::Transparent;

	RenderCollector.AddQuad2DInfo(Quad2DInfo);
});
#endif