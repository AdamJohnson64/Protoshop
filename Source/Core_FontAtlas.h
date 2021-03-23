#pragma once

#include "Core_IImage.h"
#include <memory>
#include <stdint.h>

struct BitmapRegion {
  uint32_t X, Y, Width, Height;
};

class AtlasFontASCII {
public:
  std::unique_ptr<IImage> AtlasImage;
  BitmapRegion ASCIIToGlyphRegion[128];
};

std::unique_ptr<AtlasFontASCII> CreateFreeTypeFont();