#include "Core_D3D11Util.h"
#include "Core_D3D.h"
#include "Core_DXGIUtil.h"

D3D11_SAMPLER_DESC Make_D3D11_SAMPLER_DESC_DefaultWrap() {
  D3D11_SAMPLER_DESC desc = {};
  desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
  desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
  desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
  return desc;
}

D3D11_VIEWPORT Make_D3D11_VIEWPORT(UINT width, UINT height) {
  D3D11_VIEWPORT desc = {};
  desc.Width = width;
  desc.Height = height;
  desc.MaxDepth = 1.0f;
  return desc;
}

D3D11_SHADER_RESOURCE_VIEW_DESC
Make_D3D11_SHADER_RESOURCE_VIEW_DESC_Texture2D(DXGI_FORMAT format) {
  D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
  desc.Format = format;
  desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  desc.Texture2D.MipLevels = 1;
  return desc;
}

CComPtr<ID3D11Buffer> D3D11_Create_Buffer(ID3D11Device *device, UINT bindflags,
                                          int size) {
  D3D11_BUFFER_DESC desc = {};
  desc.ByteWidth = size;
  desc.BindFlags = bindflags;
  desc.StructureByteStride = size;
  CComPtr<ID3D11Buffer> resource;
  TRYD3D(device->CreateBuffer(&desc, nullptr, &resource.p));
  return resource;
}

CComPtr<ID3D11Buffer> D3D11_Create_Buffer(ID3D11Device *device, UINT bindflags,
                                          int size, const void *data) {
  D3D11_BUFFER_DESC desc = {};
  desc.ByteWidth = size;
  desc.Usage = D3D11_USAGE_IMMUTABLE;
  desc.BindFlags = bindflags;
  desc.StructureByteStride = size;
  D3D11_SUBRESOURCE_DATA filldata = {};
  filldata.pSysMem = data;
  CComPtr<ID3D11Buffer> resource;
  TRYD3D(device->CreateBuffer(&desc, &filldata, &resource.p));
  return resource;
}

CComPtr<ID3D11Texture2D> D3D11_Create_Texture2D(ID3D11Device *device,
                                                DXGI_FORMAT format, int width,
                                                int height, const void *data) {
  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width = width;
  desc.Height = height;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.Format = format;
  desc.SampleDesc.Count = 1;
  desc.Usage = D3D11_USAGE_IMMUTABLE;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  D3D11_SUBRESOURCE_DATA filldata = {};
  filldata.pSysMem = data;
  filldata.SysMemPitch = DXGI_FORMAT_Size(format) * width;
  filldata.SysMemSlicePitch = filldata.SysMemPitch * height;
  CComPtr<ID3D11Texture2D> resource;
  TRYD3D(device->CreateTexture2D(&desc, &filldata, &resource.p));
  return resource;
}

CComPtr<ID3D11RenderTargetView>
D3D11_Create_RTV_From_Texture2D(ID3D11Device *device,
                                ID3D11Texture2D *texture) {
  D3D11_RENDER_TARGET_VIEW_DESC desc = {};
  desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
  CComPtr<ID3D11RenderTargetView> rtv;
  TRYD3D(device->CreateRenderTargetView(texture, &desc, &rtv));
  return rtv;
}

CComPtr<ID3D11UnorderedAccessView>
D3D11_Create_UAV_From_Texture2D(ID3D11Device *device,
                                ID3D11Texture2D *texture) {
  D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
  desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
  CComPtr<ID3D11UnorderedAccessView> uav;
  TRYD3D(device->CreateUnorderedAccessView(texture, &desc, &uav));
  return uav;
}

////////////////////////////////////////////////////////////////////////////////
// HDR Loader
// A very junky HDR loader that barely supports the standard.
////////////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <string>

static void Expect(const std::string &line, const std::string &expect) {
  if (line != expect)
    throw std::string("Expected '" + expect + "'.");
}

template <class T>
static void ExpectData(std::ifstream &stream, const T &data) {
  T read;
  stream.read(reinterpret_cast<char *>(&read), sizeof(read));
  if (read != data)
    throw std::string("Expected data did not match.");
}

template <class T> static T ReadType(std::ifstream &stream) {
  T read;
  stream.read(reinterpret_cast<char *>(&read), sizeof(read));
  return read;
}

static uint16_t Swap(uint16_t value) {
  uint16_t result = 0;
  result |= (value & 0xFF) << 8;
  result |= (value & 0xFF00) >> 8;
  return result;
}

static float RGBEComponentToFloat(int mantissa, int exponent) {
  float v = mantissa / 256.0f;
  float d = powf(2, exponent - 128);
  return v * d;
}

CComPtr<ID3D11Texture2D> D3D11_Load_HDR(ID3D11Device *deviceD3D11,
                                        const char *filename) {
  std::ifstream hdrfile(filename, std::ios::binary);
  std::string line;
  // This reader is deliberately impermissive - we'd rather fail on some unknown
  // input. Read and validate the file header.
  std::getline(hdrfile, line);
  Expect(line, "#?RADIANCE");
  std::getline(hdrfile, line);
  Expect(line, "# Made with 100% pure HDR Shop");
  std::getline(hdrfile, line);
  Expect(line, "FORMAT=32-bit_rle_rgbe");
  std::getline(hdrfile, line);
  Expect(line, "EXPOSURE=          1.0000000000000");
  std::getline(hdrfile, line);
  Expect(line, "");
  std::getline(hdrfile, line);
  // The great thing about TDD is I don't need this to work with everything.
  uint32_t bytesPerPixel = sizeof(float) * 3;
  uint32_t imageWidth = 768;
  uint32_t imageHeight = 1024;
  Expect(line, "-Y 1024 +X 768");
  // Start reading the HDR image data.
  std::unique_ptr<uint8_t[]> imageData(
      new uint8_t[bytesPerPixel * imageWidth * imageHeight]);
  for (int y = 0; y < imageHeight; ++y) {
    std::unique_ptr<uint8_t[]> rasterBuffer(new uint8_t[4 * imageWidth]);
    ExpectData<uint8_t>(hdrfile, 2);
    ExpectData<uint8_t>(hdrfile, 2);
    uint16_t length = Swap(ReadType<uint16_t>(hdrfile));
    if (length != imageWidth)
      throw std::exception("HDR RLE raster has wrong size.");
    // Read each of the RGBE components in planar layout.
    for (int currentComponent = 0; currentComponent < 4; ++currentComponent) {
      int currentPixel = 0;
      // Keep reading until we reach the end of this raster.
      while (currentPixel < length) {
        uint8_t compressionModeAndSize = ReadType<uint8_t>(hdrfile);
        // A zero length span is meaningless.
        if (compressionModeAndSize == 0)
          throw std::exception("HDR RLE has zero length.");
        // There are two possible encodings for this span:
        if (compressionModeAndSize > 128) {
          // Read a replicated run-length encoded span.
          compressionModeAndSize &= 127;
          uint8_t valueRLE = ReadType<uint8_t>(hdrfile);
          for (int replicateComponentRLE = 0;
               replicateComponentRLE < compressionModeAndSize;
               ++replicateComponentRLE) {
            // If we ran off the end of the image then something is wrong.
            if (currentPixel > length)
              throw std::exception("HDR RLE raster overrun.");
            rasterBuffer[4 * currentPixel + currentComponent] = valueRLE;
            ++currentPixel;
          }
        } else {
          // Read a span of bytes, one byte at a time.
          for (int readAllComponent = 0;
               readAllComponent < compressionModeAndSize; ++readAllComponent) {
            // If we ran off the end of the image then something is wrong.
            if (currentPixel > length)
              throw std::exception("HDR RLE raster overrun.");
            uint8_t pixel = ReadType<uint8_t>(hdrfile);
            rasterBuffer[4 * currentPixel + currentComponent] = pixel;
            ++currentPixel;
          }
        }
      }
    }
    // Unpack the RGBE encoded pixels into RGBA32 format.
    uint8_t *rasterByte = &imageData[bytesPerPixel * imageWidth * y];
    float *rasterFloat = (float *)rasterByte;
    for (int x = 0; x < imageWidth; ++x) {
      byte unpackR = rasterBuffer[x * 4 + 0];
      byte unpackG = rasterBuffer[x * 4 + 1];
      byte unpackB = rasterBuffer[x * 4 + 2];
      byte unpackE = rasterBuffer[x * 4 + 3];
      rasterFloat[3 * x + 0] = RGBEComponentToFloat(unpackR, unpackE);
      rasterFloat[3 * x + 1] = RGBEComponentToFloat(unpackG, unpackE);
      rasterFloat[3 * x + 2] = RGBEComponentToFloat(unpackB, unpackE);
    }
  }
  CComPtr<ID3D11Texture2D> texture;
  {
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = imageWidth;
    desc.Height = imageHeight;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = imageData.get();
    data.SysMemPitch = sizeof(float) * 3 * imageWidth;
    data.SysMemSlicePitch = data.SysMemPitch * imageHeight;
    TRYD3D(deviceD3D11->CreateTexture2D(&desc, &data, &texture.p));
  }
  return texture;
}