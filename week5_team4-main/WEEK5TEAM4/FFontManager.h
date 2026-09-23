#pragma once

#include "Core.h"
#include "FFontAtlas.h"
#include <ft2build.h>
#include FT_FREETYPE_H

class FFontManager
{
public:
	FFontManager()
	{
		if (Library != nullptr)
		{
			throw std::runtime_error("FreeType library is already initialized.");
		}

		FT_Error error = FT_Init_FreeType(&Library);
		if (error)
		{
			throw std::runtime_error("Failed to initialize FreeType library.");
		}
	}

	~FFontManager()
	{
		FT_Done_FreeType(Library);
		Library = nullptr;
	}

	inline FT_Library GetLibrary() const { return Library; }

private:
	FT_Library Library = nullptr;
};