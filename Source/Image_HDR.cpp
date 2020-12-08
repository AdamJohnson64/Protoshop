#include "Image_HDR.h"
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

ImageOwned Load_HDR(const char *filename) {
  // It's not strictly necessary to define types here but we'll use these to
  // highlight the intermediate data storage.
  struct PixelR8G8B8E8 {
    uint8_t R, G, B, E;
  };
  struct PixelR32G32B32F {
    float R, G, B;
  };
  // This reader is deliberately impermissive - we'd rather fail on some unknown
  // input. Read and validate the file header.
  std::ifstream hdrfile(filename, std::ios::binary);
  uint32_t imageWidth = 768;
  uint32_t imageHeight = 1024;
  {
    std::string line;
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
    Expect(line, "-Y 1024 +X 768");
  }
  ////////////////////////////////////////////////////////////////////////////////
  // Define the output image as RGB float data and start filling it.
  std::unique_ptr<PixelR32G32B32F[]> finalImage(
      new PixelR32G32B32F[imageWidth * imageHeight]);
  {
    ////////////////////////////////////////////////////////////////////////////////
    // Move through each raster:
    // One raster contains R8G8B8E8 in planes (All Rs, then Gs, Bs, finally Es).
    // After we have a complete raster we will unpack this to float form.
    std::unique_ptr<PixelR8G8B8E8[]> rasterRGBE(new PixelR8G8B8E8[imageWidth]);
    for (int y = 0; y < imageHeight; ++y) {
      ////////////////////////////////////////////////////////////////////////////////
      // Validate the raster header:
      // Should start with 0x02, 0x02, and uint16_t length (big endian).
      ExpectData<uint8_t>(hdrfile, 2);
      ExpectData<uint8_t>(hdrfile, 2);
      uint16_t length = Swap(ReadType<uint16_t>(hdrfile));
      if (length != imageWidth)
        throw std::exception("HDR RLE raster has wrong size.");
      ////////////////////////////////////////////////////////////////////////////////
      // Read each of the RGBE components in planar layout.
      {
        uint8_t *rasterBytes = reinterpret_cast<uint8_t *>(rasterRGBE.get());
        for (int currentComponent = 0; currentComponent < 4;
             ++currentComponent) {
          int currentPixel = 0;
          // Keep reading until we reach the end of this raster.
          while (currentPixel < length) {
            uint8_t compressionModeAndSize = ReadType<uint8_t>(hdrfile);
            // A zero length span is meaningless.
            if (compressionModeAndSize == 0)
              throw std::exception("HDR RLE has zero length.");
            // There are two possible encodings for this span:
            // MSB set; the low 7-bits is a repeat count of the following byte.
            // MSB clear; the low 7-bits is the length of byte data to read.
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
                rasterBytes[4 * currentPixel + currentComponent] = valueRLE;
                ++currentPixel;
              }
            } else {
              // Read a span of bytes, one byte at a time.
              for (int readAllComponent = 0;
                   readAllComponent < compressionModeAndSize;
                   ++readAllComponent) {
                // If we ran off the end of the image then something is wrong.
                if (currentPixel > length)
                  throw std::exception("HDR RLE raster overrun.");
                rasterBytes[4 * currentPixel + currentComponent] =
                    ReadType<uint8_t>(hdrfile);
                ++currentPixel;
              }
            }
          }
          if (currentPixel != imageWidth)
            throw std::exception("HDR RLE raster has wrong size.");
        }
      }
      ////////////////////////////////////////////////////////////////////////////////
      // Unpack the raster of RGBE encoded pixels into RGBA32F format.
      PixelR32G32B32F *rasterFloat = &finalImage[imageWidth * y];
      for (int x = 0; x < imageWidth; ++x) {
        const PixelR8G8B8E8 &inRGBE = rasterRGBE[x];
        PixelR32G32B32F &outRGB = rasterFloat[x];
        outRGB.R = RGBEComponentToFloat(inRGBE.R, inRGBE.E);
        outRGB.G = RGBEComponentToFloat(inRGBE.G, inRGBE.E);
        outRGB.B = RGBEComponentToFloat(inRGBE.B, inRGBE.E);
      }
    }
  }
  return ImageOwned(imageWidth, imageHeight,
                    sizeof(PixelR32G32B32F) * imageWidth,
                    DXGI_FORMAT_R32G32B32_FLOAT, finalImage.release());
}