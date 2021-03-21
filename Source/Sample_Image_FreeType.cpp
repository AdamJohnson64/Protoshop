///////////////////////////////////////////////////////////////////////////////
// Sample - FreeType Atlas
///////////////////////////////////////////////////////////////////////////////
// Pack a font onto a texture atlas using FreeType as the backend rasterizer.
///////////////////////////////////////////////////////////////////////////////

#include "Core_IImage.h"
#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H

std::unique_ptr<IImage> CreateSample_Image_FreeTypeAtlas() {
  ////////////////////////////////////////////////////////////////////////////////
  // Extract all glyphs from the font and store in IImages.
  std::vector<std::unique_ptr<IImage>> Glyphs;
  {
    // Set up the FreeType library.
    FT_Library ftLibrary;
    FT_Init_FreeType(&ftLibrary);
    // Prepare our font for rendering (Roboto TTF).
    FT_Face ftFace;
    FT_New_Face(ftLibrary,
                "Submodules\\roboto\\src\\hinted\\Roboto-Regular.ttf", 0,
                &ftFace);
    FT_Set_Char_Size(ftFace, 0, 36 * 64, 300, 300);
    // Dump out every glyph from bog standard ASCII in this font.
    std::vector<FT_Long> alreadyHave;
    for (int asciiChar = 0; asciiChar < 128; ++asciiChar) {
      FT_Long glyphIndex = FT_Get_Char_Index(ftFace, asciiChar);
      // If we've already emitted this glyph then we just reuse the existing
      // glyph for this character.
      if (std::find(alreadyHave.begin(), alreadyHave.end(), glyphIndex) !=
          alreadyHave.end())
        continue;
      // Make a note that we now have this glyph.
      alreadyHave.push_back(glyphIndex);
      // Prepare the glyph bitmap.
      FT_Load_Glyph(ftFace, glyphIndex, FT_LOAD_DEFAULT);
      FT_Render_Glyph(ftFace->glyph, FT_RENDER_MODE_NORMAL);
      if (ftFace->glyph->bitmap.buffer == nullptr)
        continue;
      Glyphs.push_back(CreateImage_CopyPixels(
          ftFace->glyph->bitmap.width, ftFace->glyph->bitmap.rows,
          ftFace->glyph->bitmap.pitch, DXGI_FORMAT_R8_UNORM,
          ftFace->glyph->bitmap.buffer));
    }
    FT_Done_FreeType(ftLibrary);
  }
  ////////////////////////////////////////////////////////////////////////////////
  // Pre-sort the glyphs from smallest to largest height.
  // Glyphs of equal height (rare) sort by the smallest width.
  // By doing this we avoid situations where a line of glyphs contains one tall
  // glyph that wastes a lot of atlas height.
  std::function<bool(std::unique_ptr<IImage> &, std::unique_ptr<IImage> &)>
      lessFunction =
          [&](std::unique_ptr<IImage> &lhs, std::unique_ptr<IImage> &rhs) {
            return lhs->GetHeight() < rhs->GetHeight() ||
                   (lhs->GetHeight() == rhs->GetHeight() &&
                    lhs->GetWidth() < rhs->GetWidth());
          };
  std::sort(Glyphs.begin(), Glyphs.end(), lessFunction);
  // Now flip it around so we have the largest first (exits sooner).
  std::reverse(Glyphs.begin(), Glyphs.end());
  // Pack all these glyphs into a square atlas using a naive packing.
  // We're going to use the dumb method of trying a packing and then resizing up
  // if we run out of space.
  int atlasWidth = 64;
  int atlasHeight = 64;
  // We're going to attempt the packing without generating a bitmap first and
  // try to find a good size. If we were trying to be performant we could take
  // into account the total pixel coverage and start from a sensible square size
  // instead of trying something that might be far too small to start. We're
  // also not performing clever bin packing here since the problem is actually
  // NP-hard; we just want something "good enough".
TRYAGAIN : {
  int cursorX = 0;
  int cursorY = 0;
  int lineHeight = 0;
  for (auto &glyph : Glyphs) {
    // If the next glyph will spill over the bitmap width then start a new
    // line.
    if (cursorX + glyph->GetWidth() > atlasWidth) {
      cursorX = 0;
      cursorY = cursorY + lineHeight;
      lineHeight = 0;
    }
    // If the next glyph will spill over the bitmap size then resize the
    // bitmap slightly.
    if (cursorX + glyph->GetWidth() > atlasWidth ||
        cursorY + glyph->GetHeight() > atlasHeight) {
      atlasWidth += 1;
      atlasHeight += 1;
      // atlasWidth *= 2;
      // atlasHeight *= 2;
      // If we're getting up to 4K textures then we'll stop the stupid here.
      if (atlasWidth > 4096 || atlasHeight > 4096)
        throw std::exception("Atlas will be unreasonably large.");
      // Otherwise we'll try this size instead.
      goto TRYAGAIN;
    }
    // Update our cursor, move right, and expand the line if necessary.
    cursorX += glyph->GetWidth();
    lineHeight = max(lineHeight, glyph->GetHeight());
  }
}
  // Now perform this packing again but build the bitmap as we go.
  uint8_t *atlasData = new uint8_t[atlasWidth * atlasHeight];
  memset(atlasData, 0, atlasWidth * atlasHeight);
  int cursorX = 0;
  int cursorY = 0;
  int lineHeight = 0;
  for (auto &glyph : Glyphs) {
    // If the next glyph will spill over the bitmap width then start a new line.
    if (cursorX + glyph->GetWidth() > atlasWidth) {
      cursorX = 0;
      cursorY = cursorY + lineHeight;
      lineHeight = 0;
    }
    // If the next glyph will spill over the bitmap size then resize the bitmap
    // slightly.
    if (cursorX + glyph->GetWidth() > atlasWidth ||
        cursorY + glyph->GetHeight() > atlasHeight) {
      throw std::exception("Glyph does not fit; how can this be possible?");
    }
    // Copy this glyph into the atlas.
    for (int y = 0; y < glyph->GetHeight(); ++y) {
      const uint8_t *copyFromRaster =
          static_cast<const uint8_t *>(glyph->GetData()) +
          glyph->GetStride() * y;
      uint8_t *copyToRaster = atlasData + atlasWidth * (cursorY + y);
      memcpy(copyToRaster + cursorX, copyFromRaster, glyph->GetWidth());
    }
    // Update our cursor, move right, and expand the line if necessary.
    cursorX += glyph->GetWidth();
    lineHeight = max(lineHeight, glyph->GetHeight());
  }
  return CreateImage_AutoDelete(atlasWidth, atlasHeight, atlasWidth,
                                DXGI_FORMAT_R8_UNORM, atlasData);
}