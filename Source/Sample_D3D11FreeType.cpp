///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 FreeType
///////////////////////////////////////////////////////////////////////////////
// Let's draw some really nice-looking text...
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3D11Util.h"
#include "Core_D3DCompiler.h"
#include "Core_IImage.h"
#include "SampleResources.h"
#include <algorithm>
#include <d3d11.h>
#include <functional>
#include <memory>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11FreeType(std::shared_ptr<Direct3D11Device> device) {
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
  std::unique_ptr<IImage> atlasImage = std::unique_ptr<IImage>(
      CreateImage_AutoDelete(atlasWidth, atlasHeight, atlasWidth,
                             DXGI_FORMAT_R8_UNORM, atlasData));
  ////////////////////////////////////////////////////////////////////////////////
  // We're going to draw this texture to the screen using a compute shader.
  // If we were doing this properly we'd probably want to use textured
  // triangles and generate a vertex buffer with quads mapped out of an atlas.
  // But...we're lazy. So, no.
  ////////////////////////////////////////////////////////////////////////////////
  // Create a compute shader.
  CComPtr<ID3D11ComputeShader> shaderCompute;
  {
    CComPtr<ID3DBlob> blobCS = CompileShader("cs_5_0", "main", R"SHADER(
    RWTexture2D<float4> renderTarget;
    Texture2D<float4> userImage;

    [numthreads(1, 1, 1)]
    void main(uint3 dispatchThreadId : SV_DispatchThreadID)
    {
    float r = userImage.Load(int3(dispatchThreadId.xy, 0)).r;
    renderTarget[dispatchThreadId.xy] = float4(r, r, r, 1);
    })SHADER");
    TRYD3D(device->GetID3D11Device()->CreateComputeShader(
        blobCS->GetBufferPointer(), blobCS->GetBufferSize(), nullptr,
        &shaderCompute));
  }
  CComPtr<ID3D11ShaderResourceView> srvImage =
      D3D11_Create_SRV(device->GetID3D11DeviceContext(), atlasImage.get());
  return [=](const SampleResourcesD3D11 &sampleResources) {
    {
      D3D11_TEXTURE2D_DESC descBackbuffer = {};
      sampleResources.BackBufferTexture->GetDesc(&descBackbuffer);
      CComPtr<ID3D11UnorderedAccessView> uavBackbuffer =
          D3D11_Create_UAV_From_Texture2D(device->GetID3D11Device(),
                                          sampleResources.BackBufferTexture);
      device->GetID3D11DeviceContext()->ClearState();
      // Beginning of rendering.
      device->GetID3D11DeviceContext()->CSSetUnorderedAccessViews(
          0, 1, &uavBackbuffer.p, nullptr);
      device->GetID3D11DeviceContext()->CSSetShader(shaderCompute, nullptr, 0);
      device->GetID3D11DeviceContext()->CSSetShaderResources(0, 1, &srvImage.p);
      device->GetID3D11DeviceContext()->Dispatch(descBackbuffer.Width,
                                                 descBackbuffer.Height, 1);
      device->GetID3D11DeviceContext()->ClearState();
      device->GetID3D11DeviceContext()->Flush();
    }
    device->GetID3D11DeviceContext()->Flush();
  };
}