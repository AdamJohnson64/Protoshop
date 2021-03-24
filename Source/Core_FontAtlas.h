#pragma once

#include "Core_IImage.h"
#include <memory>
#include <stdint.h>

struct BitmapRegion {
  int32_t X, Y;
  int32_t Width, Height;
};

struct GlyphMetadata {
  BitmapRegion Image;
  int32_t OffsetX, OffsetY;
  int32_t AdvanceX;
};

class FontASCII {
public:
  std::unique_ptr<IImage> Atlas;
  int32_t OffsetLine;
  GlyphMetadata ASCIIToGlyph[128];
};

std::unique_ptr<FontASCII> CreateFreeTypeFont();