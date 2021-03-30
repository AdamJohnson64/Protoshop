#pragma once

#include "Core_D3D.h"
#include "Core_IImage.h"
#include "Core_Math.h"
#include <memory>
#include <stdint.h>
#include <string_view>
#include <vector>

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
  GlyphMetadata ASCIIToGlyph[256];
};

std::unique_ptr<FontASCII> CreateFreeTypeFont();

void EmitTextQuads(const FontASCII &font, std::vector<VertexTex> &vertices,
                   const Vector2 &location, const std::string_view &text);