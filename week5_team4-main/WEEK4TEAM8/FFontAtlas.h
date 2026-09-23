#pragma once

#include "Core.h"
#include "TMap.h"
#include "MathUtility.h"
#include "Vector.h"
#include <ft2build.h>
#include FT_FREETYPE_H

class FFontAtlas;

struct FFontGlyph
{
	FVector4 SubUV; // 아틀라스 기준 정규화

	uint32 Width;
	uint32 Height;
	int32 BearingX;
	int32 BearingY;
	int32 AdvanceX;
	int32 AdvanceY;
};

struct FFontGlyphBitmap
{
	uint32 Left;
	uint32 Top;
	uint32 Bottom;
	uint32 Right;
	int32 Pitch;
	const unsigned char* Buffer;
};

class FFontAtlasHandler
{
public:
	virtual ~FFontAtlasHandler() = default;

	virtual bool HandleAddGlyph(FFontAtlas& FontAtlas, const FFontGlyph& InGlyph, const FFontGlyphBitmap& InBitmap) = 0;
};

class FFontAtlas
{
public:
	FFontAtlas() = default;
	FFontAtlas(FT_Face InFace, uint32 InWidth, uint32 InHeight, uint32 InPaddingW, uint32 InPaddingH) 
		: Face(InFace)
		, Width(InWidth)
		, Height(InHeight)
		, PaddingW(InPaddingW)
		, PaddingH(InPaddingH)
	{
	}

	void SetAtlasHandler(FFontAtlasHandler& InAtlasHandler)
	{
		AtlasHandler = &InAtlasHandler;
	}

	void AddGlyph(uint32 CodePoint)
	{
		if (GlyphMap.Contains(CodePoint))
		{
			return;
		}

		if (PaddingH > Height || PaddingW > Width)
		{
			return;
		}

		FFontGlyph NewGlyph = {};
		if (FT_Load_Char(Face, CodePoint, FT_LOAD_RENDER))
		{
			// 글리프 로드 실패
			return;
		}
		else
		{
			FT_GlyphSlot glyph = Face->glyph;

			uint32 BackupX = CurrentX;
			uint32 BackupY = CurrentY;
			uint32 BackupRowHeight = CurrentRowHeight;

			if (glyph->bitmap.width > Width - PaddingW)
			{
				return;
			}

			if (CurrentX + glyph->bitmap.width + PaddingW > Width)
			{
				CurrentX = 0;
				CurrentY += CurrentRowHeight + PaddingH;
				CurrentRowHeight = 0;
			}

			if (CurrentY + glyph->bitmap.rows + PaddingH > Height)
			{
				// 아틀라스 공간 부족
				CurrentX = BackupX;
				CurrentY = BackupY;
				CurrentRowHeight = BackupRowHeight;
			}
			else
			{
				NewGlyph.SubUV = FVector4(CurrentX / (float)Width, CurrentY / (float)Height, glyph->bitmap.width / (float)Width, glyph->bitmap.rows / (float)Height);
				NewGlyph.Width = glyph->bitmap.width;
				NewGlyph.Height = glyph->bitmap.rows;
				NewGlyph.BearingX = glyph->bitmap_left;
				NewGlyph.BearingY = glyph->bitmap_top;
				NewGlyph.AdvanceX = (glyph->advance.x >> 6);
				NewGlyph.AdvanceY = (glyph->advance.y >> 6);

				FFontGlyphBitmap GlyphBitmap = {};
				GlyphBitmap.Left = CurrentX;
				GlyphBitmap.Top = CurrentY;
				GlyphBitmap.Right = CurrentX + glyph->bitmap.width;
				GlyphBitmap.Bottom = CurrentY + glyph->bitmap.rows;
				GlyphBitmap.Pitch = glyph->bitmap.pitch;
				GlyphBitmap.Buffer = glyph->bitmap.buffer;

				if (AtlasHandler && AtlasHandler->HandleAddGlyph(*this, NewGlyph, GlyphBitmap))
				{
					GlyphMap.Add(CodePoint, NewGlyph);
					CurrentX += glyph->bitmap.width + PaddingW;
					CurrentRowHeight = FPlatformMath::Max(CurrentRowHeight, glyph->bitmap.rows);
				}
				else
				{
					// 글리프 추가 실패
					CurrentX = BackupX;
					CurrentY = BackupY;
					CurrentRowHeight = BackupRowHeight;
				}
			}
		}
	}

	bool HasGlyph(uint32 CodePoint) const
	{
		return GlyphMap.Contains(CodePoint);
	}

	const FFontGlyph& GetGlyph(uint32 CodePoint) const
	{
		return GlyphMap[CodePoint];
	}

	inline float LineHeight() const
	{
		return Face->size->metrics.height / 64.f;
	}

	inline float Ascender() const
	{
		return Face->size->metrics.ascender / 64.f;
	}

	inline float Descender() const
	{
		return Face->size->metrics.descender / 64.f;
	}

	inline uint32 GetWidth() const { return Width; }
	inline uint32 GetHeight() const { return Height; }

private:
	FFontAtlasHandler* AtlasHandler = nullptr;

	FT_Face Face = nullptr;
	TMap<uint32, FFontGlyph> GlyphMap;

	uint32 Width = 1024;
	uint32 Height = 1024;
	uint32 PaddingW = 1;
	uint32 PaddingH = 1;

	uint32 CurrentX = 0;
	uint32 CurrentY = 0;
	uint32 CurrentRowHeight = 0;
};