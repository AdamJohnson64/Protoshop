#include "Core_FontAtlas.h"
#include <algorithm>
#include <functional>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H

struct AtlasGlyphExtract {
  std::unique_ptr<IImage> Image;
  GlyphMetadata Metadata;
};

std::unique_ptr<FontASCII> CreateFreeTypeFont() {
  ////////////////////////////////////////////////////////////////////////////////
  // Extract all glyphs from the font and store in IImages.
  // We're only dealing with ASCII here.
  std::vector<AtlasGlyphExtract> allGlyphs;
  uint32_t mapASCIIToGlyph[128] = {};
  for (int initmap = 0; initmap < 128; ++initmap) {
    mapASCIIToGlyph[initmap] = -1;
  }
  ////////////////////////////////////////////////////////////////////////////////
  // This is the final font we will emit.
  std::unique_ptr<FontASCII> result(new FontASCII());
  {
    ////////////////////////////////////////////////////////////////////////////////
    // Set up the FreeType library.
    FT_Library ftLibrary;
    FT_Init_FreeType(&ftLibrary);
    // Prepare our font for rendering (Roboto TTF).
    FT_Face ftFace;
    FT_New_Face(ftLibrary,
                "Submodules\\roboto\\src\\hinted\\Roboto-Regular.ttf", 0,
                &ftFace);
    FT_UInt fontSize = 14;
    FT_UInt fontDPI = 300;
    FT_Set_Char_Size(ftFace, 0, fontSize * 64, 300, 300);
    ////////////////////////////////////////////////////////////////////////////////
    // Line height calculation.
    // This is the pixel height to advance for one line.
    float fontPixelSize = fontSize * fontDPI / 72.0f;
    float fontGridSize = ftFace->height * fontPixelSize / ftFace->units_per_EM;
    result->OffsetLine = fontGridSize;
    ////////////////////////////////////////////////////////////////////////////////
    // Dump out every glyph from bog standard ASCII in this font.
    allGlyphs.resize(ftFace->num_glyphs);
    for (int asciiChar = 0; asciiChar < 128; ++asciiChar) {
      FT_Long glyphIndex = FT_Get_Char_Index(ftFace, asciiChar);
      // If we've already emitted this glyph then ignore it.
      if (glyphIndex < 0 || glyphIndex > ftFace->num_glyphs ||
          allGlyphs[glyphIndex].Image != nullptr)
        continue;
      // Map this character to its glyph.
      mapASCIIToGlyph[asciiChar] = glyphIndex;
      // Prepare the glyph.
      FT_Load_Glyph(ftFace, glyphIndex, FT_LOAD_DEFAULT);
      // Collect glyph metadata.
      allGlyphs[glyphIndex].Metadata.OffsetX = ftFace->glyph->bitmap_left;
      allGlyphs[glyphIndex].Metadata.OffsetY = ftFace->glyph->bitmap_top;
      allGlyphs[glyphIndex].Metadata.AdvanceX =
          ftFace->glyph->metrics.horiAdvance / 64;
      // Render the glyph bitmap.
      FT_Render_Glyph(ftFace->glyph, FT_RENDER_MODE_NORMAL);
      if (ftFace->glyph->bitmap.buffer == nullptr)
        continue;
      allGlyphs[glyphIndex].Image = CreateImage_CopyPixels(
          ftFace->glyph->bitmap.width, ftFace->glyph->bitmap.rows,
          ftFace->glyph->bitmap.pitch, DXGI_FORMAT_R8_UNORM,
          ftFace->glyph->bitmap.buffer);
    }
    FT_Done_FreeType(ftLibrary);
  }
  ////////////////////////////////////////////////////////////////////////////////
  // Perform the glyph packing.
  int atlasWidth = 64;
  int atlasHeight = 64;
  {
    ////////////////////////////////////////////////////////////////////////////////
    // Take a pointer copy of all the glyphs (we're going to pre-sort this).
    std::vector<AtlasGlyphExtract *> allSortedGlyphs;
    for (auto &iterGlyph : allGlyphs) {
      allSortedGlyphs.push_back(&iterGlyph);
    }
    ////////////////////////////////////////////////////////////////////////////////
    // Remove all null glyphs; we either didn't extract them or they don't have
    // any glyph data.
    allSortedGlyphs.erase(std::remove_if(allSortedGlyphs.begin(),
                                         allSortedGlyphs.end(),
                                         [=](const AtlasGlyphExtract *elem) {
                                           return elem->Image == nullptr;
                                         }),
                          allSortedGlyphs.end());
    ////////////////////////////////////////////////////////////////////////////////
    // Pre-sort the glyphs from smallest to largest height.
    // Glyphs of equal height (rare) sort by the smallest width.
    // By doing this we avoid situations where a line of glyphs contains one
    // tall glyph that wastes a lot of atlas height.
    std::function<bool(AtlasGlyphExtract *, AtlasGlyphExtract *)> lessFunction =
        [&](const AtlasGlyphExtract *lhs, const AtlasGlyphExtract *rhs) {
          return lhs->Image->GetHeight() < rhs->Image->GetHeight() ||
                 (lhs->Image->GetHeight() == rhs->Image->GetHeight() &&
                  lhs->Image->GetWidth() < rhs->Image->GetWidth());
        };
    std::sort(allSortedGlyphs.begin(), allSortedGlyphs.end(), lessFunction);
    // Now flip it around so we have the largest first (exits sooner).
    std::reverse(allSortedGlyphs.begin(), allSortedGlyphs.end());
    ////////////////////////////////////////////////////////////////////////////////
    // We're going to attempt the packing without generating a bitmap first and
    // try to find a good size. If we were trying to be performant we could take
    // into account the total pixel coverage and start from a sensible square
    // size instead of trying something that might be far too small to start.
    // We're also not performing clever bin packing here since the problem is
    // actually NP-hard; we just want something "good enough".
  TRYAGAIN : {
    int cursorX = 0;
    int cursorY = 0;
    int lineHeight = 0;
    for (auto &iterGlyph : allSortedGlyphs) {
      // If the next glyph will spill over the bitmap width then start a new
      // line.
      if (cursorX + iterGlyph->Image->GetWidth() > atlasWidth) {
        cursorX = 0;
        cursorY = cursorY + lineHeight;
        lineHeight = 0;
      }
      // If the next glyph will spill over the bitmap size then resize the
      // bitmap slightly.
      if (cursorX + iterGlyph->Image->GetWidth() > atlasWidth ||
          cursorY + iterGlyph->Image->GetHeight() > atlasHeight) {
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
      // Record the position and size of this glyph.
      iterGlyph->Metadata.Image.X = cursorX;
      iterGlyph->Metadata.Image.Y = cursorY;
      iterGlyph->Metadata.Image.Width = iterGlyph->Image->GetWidth();
      iterGlyph->Metadata.Image.Height = iterGlyph->Image->GetHeight();
      iterGlyph->Metadata.OffsetX = iterGlyph->Metadata.OffsetX;
      iterGlyph->Metadata.OffsetY = iterGlyph->Metadata.OffsetY;
      iterGlyph->Metadata.AdvanceX = iterGlyph->Metadata.AdvanceX;
      // Update our cursor, move right, and expand the line if necessary.
      cursorX += iterGlyph->Image->GetWidth();
      lineHeight = max(lineHeight, iterGlyph->Image->GetHeight());
    }
  }
  }
  ////////////////////////////////////////////////////////////////////////////////
  // Now use the recorded locations to pack the image.
  uint8_t *atlasBitmap = new uint8_t[atlasWidth * atlasHeight];
  memset(atlasBitmap, 0, atlasWidth * atlasHeight);
  for (const auto &iterGlyph : allGlyphs) {
    if (iterGlyph.Image == nullptr)
      continue;
    // Copy this glyph into the atlas.
    for (int y = 0; y < iterGlyph.Image->GetHeight(); ++y) {
      const uint8_t *copyFromRaster =
          static_cast<const uint8_t *>(iterGlyph.Image->GetData()) +
          iterGlyph.Image->GetStride() * y;
      uint8_t *copyToRaster =
          atlasBitmap + atlasWidth * (iterGlyph.Metadata.Image.Y + y);
      memcpy(copyToRaster + iterGlyph.Metadata.Image.X, copyFromRaster,
             iterGlyph.Image->GetWidth());
    }
  }
  ////////////////////////////////////////////////////////////////////////////////
  // Record all the glyph locations for each ASCII character.
  for (int indexASCII = 0; indexASCII < 128; ++indexASCII) {
    int glyphIndex = mapASCIIToGlyph[indexASCII];
    if (glyphIndex >= 0 && glyphIndex < allGlyphs.size()) {
      result->ASCIIToGlyph[indexASCII] = allGlyphs[glyphIndex].Metadata;
    } else {
      result->ASCIIToGlyph[indexASCII].Image.X = 0;
      result->ASCIIToGlyph[indexASCII].Image.Y = 0;
      result->ASCIIToGlyph[indexASCII].Image.Width = 0;
      result->ASCIIToGlyph[indexASCII].Image.Height = 0;
    }
  }
  result->Atlas = CreateImage_AutoDelete(atlasWidth, atlasHeight, atlasWidth,
                                         DXGI_FORMAT_R8_UNORM, atlasBitmap);
  return result;
}

void EmitTextQuads(const FontASCII &font, std::vector<VertexTex> &vertices,
                   const Vector2 &location, const std::string_view &text) {
  Vector2 cursor = {};
  cursor.X = location.X;
  cursor.Y = location.Y;
  for (unsigned char c : text)
    if (c == 0x0A) {
      // Carriage Return + Line Feed.
      cursor.X = location.X;
      cursor.Y += font.OffsetLine;
    } else if (c == 0x0D) {
      // Carriage Return Only.
      cursor.X = location.X;
    } else {
      const GlyphMetadata &r = font.ASCIIToGlyph[c];
      float minU = r.Image.X;
      float maxU = r.Image.X + r.Image.Width;
      float minV = r.Image.Y;
      float maxV = r.Image.Y + r.Image.Height;
      float minX = cursor.X + r.OffsetX;
      float maxX = minX + r.Image.Width;
      float minY = cursor.Y - r.OffsetY;
      float maxY = minY + r.Image.Height;
      vertices.push_back({{minX, minY}, {minU, minV}});
      vertices.push_back({{maxX, minY}, {maxU, minV}});
      vertices.push_back({{maxX, maxY}, {maxU, maxV}});
      vertices.push_back({{maxX, maxY}, {maxU, maxV}});
      vertices.push_back({{minX, maxY}, {minU, maxV}});
      vertices.push_back({{minX, minY}, {minU, minV}});
      cursor.X += r.AdvanceX;
    }
}