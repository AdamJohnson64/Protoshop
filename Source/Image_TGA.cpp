#include "Image_TGA.h"
#include <cstdint>
#include <dxgi1_6.h>
#include <fstream>

template <class T> T Read(std::ifstream &stream) {
  T data;
  stream.read(reinterpret_cast<char *>(&data), sizeof(T));
  return data;
}

template <class T> void Expect(std::ifstream &stream, T expect) {
  if (Read<T>(stream) != expect)
    throw std::exception("Badness.");
}

ImageOwned Load_TGA(const char *filename) {
  if (filename == nullptr) {
    return ImageOwned(1, 1, sizeof(uint32_t), DXGI_FORMAT_B8G8R8A8_UNORM,
                      new uint32_t[1]{0xFFFF0000});
  }
  std::ifstream file(filename, std::ios_base::binary);
  if (!file.is_open()) {
    return ImageOwned(1, 1, sizeof(uint32_t), DXGI_FORMAT_B8G8R8A8_UNORM,
                      new uint32_t[1]{0xFFFF0000});
  }
  Expect<uint8_t>(file, 0);
  Expect<uint8_t>(file, 0);
  Expect<uint8_t>(file, 2);
  Expect<uint16_t>(file, 0);
  Expect<uint16_t>(file, 0);
  Expect<uint8_t>(file, 0);
  uint16_t xorigin = Read<uint16_t>(file);
  uint16_t yorigin = Read<uint16_t>(file);
  uint16_t width = Read<uint16_t>(file);
  uint16_t height = Read<uint16_t>(file);
  uint8_t bitdepth = Read<uint8_t>(file);
  uint8_t imagedescriptor = Read<uint8_t>(file);
  if (bitdepth == 24) {
    if (imagedescriptor != 0)
      throw std::exception("The image descriptor should be zero for 24-bit "
                           "images, this are teh badness.");
    std::unique_ptr<uint8_t[]> data(new uint8_t[4 * width * height]);
    for (int y = height - 1; y >= 0; --y) {
      for (int x = 0; x < width; ++x) {
        data[0 + 4 * x + 4 * width * y] = Read<uint8_t>(file);
        data[1 + 4 * x + 4 * width * y] = Read<uint8_t>(file);
        data[2 + 4 * x + 4 * width * y] = Read<uint8_t>(file);
        data[3 + 4 * x + 4 * width * y] = 255;
      }
    }
    return ImageOwned(width, height, 4 * width, DXGI_FORMAT_B8G8R8A8_UNORM,
                      data.release());
  } else if (bitdepth == 32) {
    if (imagedescriptor != 8)
      throw std::exception("The image descriptor should be 8 for 32-bit "
                           "images, this am teh argh.");
    std::unique_ptr<uint8_t[]> data(new uint8_t[4 * width * height]);
    for (int y = height - 1; y >= 0; --y) {
      for (int x = 0; x < width; ++x) {
        data[0 + 4 * x + 4 * width * y] = Read<uint8_t>(file);
        data[1 + 4 * x + 4 * width * y] = Read<uint8_t>(file);
        data[2 + 4 * x + 4 * width * y] = Read<uint8_t>(file);
        data[3 + 4 * x + 4 * width * y] = Read<uint8_t>(file);
      }
    }
    return ImageOwned(width, height, 4 * width, DXGI_FORMAT_B8G8R8A8_UNORM,
                      data.release());
  }
  return ImageOwned(1, 1, sizeof(uint32_t), DXGI_FORMAT_B8G8R8A8_UNORM,
                    new uint32_t[1]{0xFFFF0000});
}