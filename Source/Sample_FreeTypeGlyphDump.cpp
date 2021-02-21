///////////////////////////////////////////////////////////////////////////////
// Sample - FreeType Glyph Dump
///////////////////////////////////////////////////////////////////////////////
// We cannot assume nor rely on any operating system to provide text display
// support in 3D. This sample demonstrates a trivial method to dump all the
// glyphs in a TrueType font file using FreeType. This may be used later in a
// debug text rendering API.
///////////////////////////////////////////////////////////////////////////////

#include "Core_Object.h"
#include <memory>
#include <Windows.h>

#include <ft2build.h>
#include FT_FREETYPE_H

class Sample_FreeTypeGlyphDump : public Object {
public:
  Sample_FreeTypeGlyphDump() {
    // Common buffer for string output.
    char debug[4096];
    // Set up the FreeType library.
    FT_Library library;
    FT_Init_FreeType(&library);
    // Prepare our font for rendering (Roboto TTF).
    FT_Face face;
    FT_New_Face(library, "Submodules\\roboto\\src\\hinted\\Roboto-Regular.ttf", 0, &face);
    FT_Set_Char_Size(face, 0, 4 * 64, 300, 300);
    // Dump out every glyph in this font.
    for (FT_Long allglyph = 0; allglyph < face->num_glyphs; ++allglyph) {
      // Prepare the glyph bitmap.
      FT_Load_Glyph(face, allglyph, FT_LOAD_DEFAULT);
      FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
      // Emit glyph metrics and data.
      sprintf_s(debug, "glyph:%d width:%d\n", allglyph, face->glyph->bitmap.width);
      OutputDebugStringA(debug);
      // Emit glyph visualization.
      for (int y = 0; y < face->glyph->bitmap.rows; ++y) {
        for (int x = 0; x < face->glyph->bitmap.width; ++x) {
          unsigned char pixel = face->glyph->bitmap.buffer[face->glyph->bitmap.width * y + x];
          debug[x] = pixel > 128 ? '#' : ' ';
          debug[face->glyph->bitmap.width] = 0;
        }
        OutputDebugStringA(debug);
        OutputDebugStringA("\n");
      }
    }
    // Some miscellaneous functions.
    FT_UInt glyph = FT_Get_Char_Index(face, 'A');
    sprintf_s(debug, "Character 'A' is glyph %d\n", glyph);
    OutputDebugStringA(debug);
  }
};

std::shared_ptr<Object>
CreateSample_FreeTypeGlyphDump() {
  return std::shared_ptr<Object>(new Sample_FreeTypeGlyphDump());
}