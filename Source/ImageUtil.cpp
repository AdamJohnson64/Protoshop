#include "ImageUtil.h"
#include "Core_Math.h"
#include <cstdlib>

struct PixelBGRA {
  uint8_t B, G, R, A;
};

struct ImageBGRA {
  void *data;
  uint32_t width, height, stride;
};

static int Random2DMix(int x, int y) { return x + y * 57; }

static int RandomExtend(int n) { return (n << 13) ^ n; }

static float RandomFloat(int n) {
  return 1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) /
                    1073741824.0;
};

static float Random2D(int x, int y) {
  return RandomFloat(RandomExtend(Random2DMix(x, y)));
}

static float Noise2D(float x, float y) { return Random2D((int)x, (int)y); }

static float SmoothNoiseFn(float x, float y) {
  return (Noise2D(x - 1, y - 1) + Noise2D(x + 1, y - 1) +
          Noise2D(x - 1, y + 1) + Noise2D(x + 1, y + 1)) /
             16 +
         (Noise2D(x - 1, y) + Noise2D(x + 1, y) + Noise2D(x, y - 1) +
          Noise2D(x, y + 1)) /
             8 +
         (Noise2D(x, y)) / 4;
}

static float Lerp(float a, float b, float x) { return a + (b - a) * x; }

static float InterpolatedNoiseFn(float x, float y) {
  return Lerp(Lerp(SmoothNoiseFn((int)x, (int)y),
                   SmoothNoiseFn((int)x + 1, (int)y), x - (int)x),
              Lerp(SmoothNoiseFn((int)x, (int)y + 1),
                   SmoothNoiseFn((int)x + 1, (int)y + 1), x - (int)x),
              y - (int)y);
}

static float PerlinNoiseFn(float x, float y) {
  return InterpolatedNoiseFn(x * 1, y * 1) * 1 +
         InterpolatedNoiseFn(x * 2, y * 2) * 0.5 +
         InterpolatedNoiseFn(x * 4, y * 4) * 0.25 +
         InterpolatedNoiseFn(x * 8, y * 8) * 0.125;
}

static const Vector3 BRICK_COLOR = {0.5f, 0.2f, 0};
static const float BRICK_DEPTH = 1.0f;
static const Vector3 MORTAR_COLOR = {0.4f, 0.4f, 0.4f};
static const float MORTAR_DEPTH = 0.8f;
static const float MORTAR_HALF_HEIGHT = 0.05f;

enum BrickTexelType {
  BRICK,
  MORTAR,
};

static BrickTexelType BrickType(float x, float y) {
  if (y < MORTAR_HALF_HEIGHT) {
    return MORTAR;
  } else if (y < 0.5f - MORTAR_HALF_HEIGHT) {
    if (x < MORTAR_HALF_HEIGHT) {
      return MORTAR;
    } else if (x < 1 - MORTAR_HALF_HEIGHT) {
      return BRICK;
    } else {
      return MORTAR;
    }
    return BRICK;
  } else if (y < 0.5f + MORTAR_HALF_HEIGHT) {
    return MORTAR;
  } else if (y < 1 - MORTAR_HALF_HEIGHT) {
    if (x < 0.5f - MORTAR_HALF_HEIGHT) {
      return BRICK;
    } else if (x < 0.5f + MORTAR_HALF_HEIGHT) {
      return MORTAR;
    } else {
      return BRICK;
    }
  } else {
    return MORTAR;
  }
}

static Vector3 BrickColor(float x, float y) {
  switch (BrickType(x, y)) {
  case BRICK:
    return BRICK_COLOR + Vector3 { 0.0f, 0.1f, 0.0f } * PerlinNoiseFn(x * 128, y * 128);
  default:
    return MORTAR_COLOR;
  }
}

static float BrickDepth(float x, float y) {
  switch (BrickType(x, y)) {
  case BRICK:
    return BRICK_DEPTH + PerlinNoiseFn(x * 128, y * 128) * 0.002f;
  default:
    return MORTAR_DEPTH + PerlinNoiseFn(x * 32, y * 32) * 0.005f;
  }
}

static Vector3 BrickNormal(float x, float y) {
  float eta = 0.001f;
  float dx = BrickDepth(x + eta, y) - BrickDepth(x - eta, y);
  float dy = BrickDepth(x, y + eta) - BrickDepth(x, y - eta);
  return Normalize(Vector3{-dx, dy, 2 * eta});
}

static void Image_Fill_BrickAlbedo(const ImageBGRA &image) {
  uint8_t *bytes = reinterpret_cast<uint8_t *>(image.data);
  for (int y = 0; y < image.height; ++y) {
    for (int x = 0; x < image.width; ++x) {
      Vector3 c = BrickColor((float)x / image.width, (float)y / image.height);
      PixelBGRA pixel = {(uint8_t)(c.Z * 255), (uint8_t)(c.Y * 255),
                         (uint8_t)(c.X * 255), 255};
      *reinterpret_cast<PixelBGRA *>(bytes + sizeof(PixelBGRA) * x +
                                     image.stride * y) = pixel;
    }
  }
}

void Image_Fill_BrickAlbedo(void *data, uint32_t width, uint32_t height,
                            uint32_t stride) {
  Image_Fill_BrickAlbedo(ImageBGRA{data, width, height, stride});
}

static void Image_Fill_BrickDepth(const ImageBGRA &image) {
  uint8_t *bytes = reinterpret_cast<uint8_t *>(image.data);
  for (int y = 0; y < image.height; ++y) {
    for (int x = 0; x < image.width; ++x) {
      float d = BrickDepth((float)x / image.width, (float)y / image.height);
      PixelBGRA pixel = {(uint8_t)(d * 255), (uint8_t)(d * 255),
                         (uint8_t)(d * 255), 255};
      *reinterpret_cast<PixelBGRA *>(bytes + sizeof(PixelBGRA) * x +
                                     image.stride * y) = pixel;
    }
  }
}

void Image_Fill_BrickDepth(void *data, uint32_t width, uint32_t height,
                            uint32_t stride) {
  Image_Fill_BrickDepth(ImageBGRA{data, width, height, stride});
}

static void Image_Fill_BrickNormal(const ImageBGRA &image) {
  uint8_t *bytes = reinterpret_cast<uint8_t *>(image.data);
  for (int y = 0; y < image.height; ++y) {
    for (int x = 0; x < image.width; ++x) {
      Vector3 n = BrickNormal((float)x / image.width, (float)y / image.height);
      Vector3 c = Vector3{n.X + 1, n.Y + 1, n.Z + 1} * 0.5f;
      PixelBGRA pixel = {(uint8_t)(c.Z * 255), (uint8_t)(c.Y * 255),
                         (uint8_t)(c.X * 255), 255};
      *reinterpret_cast<PixelBGRA *>(bytes + sizeof(PixelBGRA) * x +
                                     image.stride * y) = pixel;
    }
  }
}

void Image_Fill_BrickNormal(void *data, uint32_t width, uint32_t height,
                            uint32_t stride) {
  Image_Fill_BrickNormal(ImageBGRA{data, width, height, stride});
}

////////////////////////////////////////////////////////////////////////////////
// I grew up with a Commodore 64 so I should probably include a copy of the
// C64 character generator ROM to use as a font.
//
// You'll find this 4096 byte ROM mapped at 0xD000. The more you know...
////////////////////////////////////////////////////////////////////////////////
uint8_t Commodore64CharacterGeneratorROM[] = {
    // clang-format off
    0x3c,0x66,0x6e,0x6e,0x60,0x62,0x3c,0x00,0x18,0x3c,0x66,0x7e,0x66,0x66,0x66,0x00, // @A
    0x7c,0x66,0x66,0x7c,0x66,0x66,0x7c,0x00,0x3c,0x66,0x60,0x60,0x60,0x66,0x3c,0x00, // BC
    0x78,0x6c,0x66,0x66,0x66,0x6c,0x78,0x00,0x7e,0x60,0x60,0x78,0x60,0x60,0x7e,0x00, // DE
    0x7e,0x60,0x60,0x78,0x60,0x60,0x60,0x00,0x3c,0x66,0x60,0x6e,0x66,0x66,0x3c,0x00, // FG
    0x66,0x66,0x66,0x7e,0x66,0x66,0x66,0x00,0x3c,0x18,0x18,0x18,0x18,0x18,0x3c,0x00, // HI
    0x1e,0x0c,0x0c,0x0c,0x0c,0x6c,0x38,0x00,0x66,0x6c,0x78,0x70,0x78,0x6c,0x66,0x00, // JK
    0x60,0x60,0x60,0x60,0x60,0x60,0x7e,0x00,0x63,0x77,0x7f,0x6b,0x63,0x63,0x63,0x00, // LM
    0x66,0x76,0x7e,0x7e,0x6e,0x66,0x66,0x00,0x3c,0x66,0x66,0x66,0x66,0x66,0x3c,0x00, // NO
    0x7c,0x66,0x66,0x7c,0x60,0x60,0x60,0x00,0x3c,0x66,0x66,0x66,0x66,0x3c,0x0e,0x00, // PQ
    0x7c,0x66,0x66,0x7c,0x78,0x6c,0x66,0x00,0x3c,0x66,0x60,0x3c,0x06,0x66,0x3c,0x00, // RS
    0x7e,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x66,0x66,0x66,0x66,0x66,0x66,0x3c,0x00, // TU
    0x66,0x66,0x66,0x66,0x66,0x3c,0x18,0x00,0x63,0x63,0x63,0x6b,0x7f,0x77,0x63,0x00, // VW
    0x66,0x66,0x3c,0x18,0x3c,0x66,0x66,0x00,0x66,0x66,0x66,0x3c,0x18,0x18,0x18,0x00, // XY
    0x7e,0x06,0x0c,0x18,0x30,0x60,0x7e,0x00,0x3c,0x30,0x30,0x30,0x30,0x30,0x3c,0x00, // Z[
    0x0c,0x12,0x30,0x7c,0x30,0x62,0xfc,0x00,0x3c,0x0c,0x0c,0x0c,0x0c,0x0c,0x3c,0x00,
    0x00,0x18,0x3c,0x7e,0x18,0x18,0x18,0x18,0x00,0x10,0x30,0x7f,0x7f,0x30,0x10,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x18,0x18,0x00,0x00,0x18,0x00,
    0x66,0x66,0x66,0x00,0x00,0x00,0x00,0x00,0x66,0x66,0xff,0x66,0xff,0x66,0x66,0x00,
    0x18,0x3e,0x60,0x3c,0x06,0x7c,0x18,0x00,0x62,0x66,0x0c,0x18,0x30,0x66,0x46,0x00,
    0x3c,0x66,0x3c,0x38,0x67,0x66,0x3f,0x00,0x06,0x0c,0x18,0x00,0x00,0x00,0x00,0x00,
    0x0c,0x18,0x30,0x30,0x30,0x18,0x0c,0x00,0x30,0x18,0x0c,0x0c,0x0c,0x18,0x30,0x00,
    0x00,0x66,0x3c,0xff,0x3c,0x66,0x00,0x00,0x00,0x18,0x18,0x7e,0x18,0x18,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30,0x00,0x00,0x00,0x7e,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x03,0x06,0x0c,0x18,0x30,0x60,0x00,
    0x3c,0x66,0x6e,0x76,0x66,0x66,0x3c,0x00,0x18,0x18,0x38,0x18,0x18,0x18,0x7e,0x00,
    0x3c,0x66,0x06,0x0c,0x30,0x60,0x7e,0x00,0x3c,0x66,0x06,0x1c,0x06,0x66,0x3c,0x00,
    0x06,0x0e,0x1e,0x66,0x7f,0x06,0x06,0x00,0x7e,0x60,0x7c,0x06,0x06,0x66,0x3c,0x00,
    0x3c,0x66,0x60,0x7c,0x66,0x66,0x3c,0x00,0x7e,0x66,0x0c,0x18,0x18,0x18,0x18,0x00,
    0x3c,0x66,0x66,0x3c,0x66,0x66,0x3c,0x00,0x3c,0x66,0x66,0x3e,0x06,0x66,0x3c,0x00,
    0x00,0x00,0x18,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x18,0x00,0x00,0x18,0x18,0x30,
    0x0e,0x18,0x30,0x60,0x30,0x18,0x0e,0x00,0x00,0x00,0x7e,0x00,0x7e,0x00,0x00,0x00,
    0x70,0x18,0x0c,0x06,0x0c,0x18,0x70,0x00,0x3c,0x66,0x06,0x0c,0x18,0x00,0x18,0x00,
    0x00,0x00,0x00,0xff,0xff,0x00,0x00,0x00,0x08,0x1c,0x3e,0x7f,0x7f,0x1c,0x3e,0x00,
    0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0xff,0xff,0x00,0x00,0x00,
    0x00,0x00,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0xff,0xff,0x00,0x00,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
    0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x00,0x00,0x00,0xe0,0xf0,0x38,0x18,0x18,
    0x18,0x18,0x1c,0x0f,0x07,0x00,0x00,0x00,0x18,0x18,0x38,0xf0,0xe0,0x00,0x00,0x00,
    0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xff,0xff,0xc0,0xe0,0x70,0x38,0x1c,0x0e,0x07,0x03,
    0x03,0x07,0x0e,0x1c,0x38,0x70,0xe0,0xc0,0xff,0xff,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,
    0xff,0xff,0x03,0x03,0x03,0x03,0x03,0x03,0x00,0x3c,0x7e,0x7e,0x7e,0x7e,0x3c,0x00,
    0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x00,0x36,0x7f,0x7f,0x7f,0x3e,0x1c,0x08,0x00,
    0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x00,0x00,0x00,0x07,0x0f,0x1c,0x18,0x18,
    0xc3,0xe7,0x7e,0x3c,0x3c,0x7e,0xe7,0xc3,0x00,0x3c,0x7e,0x66,0x66,0x7e,0x3c,0x00,
    0x18,0x18,0x66,0x66,0x18,0x18,0x3c,0x00,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,
    0x08,0x1c,0x3e,0x7f,0x3e,0x1c,0x08,0x00,0x18,0x18,0x18,0xff,0xff,0x18,0x18,0x18,
    0xc0,0xc0,0x30,0x30,0xc0,0xc0,0x30,0x30,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
    0x00,0x00,0x03,0x3e,0x76,0x36,0x36,0x00,0xff,0x7f,0x3f,0x1f,0x0f,0x07,0x03,0x01,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
    0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,
    0xcc,0xcc,0x33,0x33,0xcc,0xcc,0x33,0x33,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,
    0x00,0x00,0x00,0x00,0xcc,0xcc,0x33,0x33,0xff,0xfe,0xfc,0xf8,0xf0,0xe0,0xc0,0x80,
    0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x18,0x18,0x18,0x1f,0x1f,0x18,0x18,0x18,
    0x00,0x00,0x00,0x00,0x0f,0x0f,0x0f,0x0f,0x18,0x18,0x18,0x1f,0x1f,0x00,0x00,0x00,
    0x00,0x00,0x00,0xf8,0xf8,0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,
    0x00,0x00,0x00,0x1f,0x1f,0x18,0x18,0x18,0x18,0x18,0x18,0xff,0xff,0x00,0x00,0x00,
    0x00,0x00,0x00,0xff,0xff,0x18,0x18,0x18,0x18,0x18,0x18,0xf8,0xf8,0x18,0x18,0x18,
    0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xe0,0xe0,0xe0,0xe0,0xe0,0xe0,0xe0,0xe0,
    0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xff,
    0x03,0x03,0x03,0x03,0x03,0x03,0xff,0xff,0x00,0x00,0x00,0x00,0xf0,0xf0,0xf0,0xf0,
    0x0f,0x0f,0x0f,0x0f,0x00,0x00,0x00,0x00,0x18,0x18,0x18,0xf8,0xf8,0x00,0x00,0x00,
    0xf0,0xf0,0xf0,0xf0,0x00,0x00,0x00,0x00,0xf0,0xf0,0xf0,0xf0,0x0f,0x0f,0x0f,0x0f,
    0xc3,0x99,0x91,0x91,0x9f,0x99,0xc3,0xff,0xe7,0xc3,0x99,0x81,0x99,0x99,0x99,0xff,
    0x83,0x99,0x99,0x83,0x99,0x99,0x83,0xff,0xc3,0x99,0x9f,0x9f,0x9f,0x99,0xc3,0xff,
    0x87,0x93,0x99,0x99,0x99,0x93,0x87,0xff,0x81,0x9f,0x9f,0x87,0x9f,0x9f,0x81,0xff,
    0x81,0x9f,0x9f,0x87,0x9f,0x9f,0x9f,0xff,0xc3,0x99,0x9f,0x91,0x99,0x99,0xc3,0xff,
    0x99,0x99,0x99,0x81,0x99,0x99,0x99,0xff,0xc3,0xe7,0xe7,0xe7,0xe7,0xe7,0xc3,0xff,
    0xe1,0xf3,0xf3,0xf3,0xf3,0x93,0xc7,0xff,0x99,0x93,0x87,0x8f,0x87,0x93,0x99,0xff,
    0x9f,0x9f,0x9f,0x9f,0x9f,0x9f,0x81,0xff,0x9c,0x88,0x80,0x94,0x9c,0x9c,0x9c,0xff,
    0x99,0x89,0x81,0x81,0x91,0x99,0x99,0xff,0xc3,0x99,0x99,0x99,0x99,0x99,0xc3,0xff,
    0x83,0x99,0x99,0x83,0x9f,0x9f,0x9f,0xff,0xc3,0x99,0x99,0x99,0x99,0xc3,0xf1,0xff,
    0x83,0x99,0x99,0x83,0x87,0x93,0x99,0xff,0xc3,0x99,0x9f,0xc3,0xf9,0x99,0xc3,0xff,
    0x81,0xe7,0xe7,0xe7,0xe7,0xe7,0xe7,0xff,0x99,0x99,0x99,0x99,0x99,0x99,0xc3,0xff,
    0x99,0x99,0x99,0x99,0x99,0xc3,0xe7,0xff,0x9c,0x9c,0x9c,0x94,0x80,0x88,0x9c,0xff,
    0x99,0x99,0xc3,0xe7,0xc3,0x99,0x99,0xff,0x99,0x99,0x99,0xc3,0xe7,0xe7,0xe7,0xff,
    0x81,0xf9,0xf3,0xe7,0xcf,0x9f,0x81,0xff,0xc3,0xcf,0xcf,0xcf,0xcf,0xcf,0xc3,0xff,
    0xf3,0xed,0xcf,0x83,0xcf,0x9d,0x03,0xff,0xc3,0xf3,0xf3,0xf3,0xf3,0xf3,0xc3,0xff,
    0xff,0xe7,0xc3,0x81,0xe7,0xe7,0xe7,0xe7,0xff,0xef,0xcf,0x80,0x80,0xcf,0xef,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xe7,0xe7,0xe7,0xe7,0xff,0xff,0xe7,0xff,
    0x99,0x99,0x99,0xff,0xff,0xff,0xff,0xff,0x99,0x99,0x00,0x99,0x00,0x99,0x99,0xff,
    0xe7,0xc1,0x9f,0xc3,0xf9,0x83,0xe7,0xff,0x9d,0x99,0xf3,0xe7,0xcf,0x99,0xb9,0xff,
    0xc3,0x99,0xc3,0xc7,0x98,0x99,0xc0,0xff,0xf9,0xf3,0xe7,0xff,0xff,0xff,0xff,0xff,
    0xf3,0xe7,0xcf,0xcf,0xcf,0xe7,0xf3,0xff,0xcf,0xe7,0xf3,0xf3,0xf3,0xe7,0xcf,0xff,
    0xff,0x99,0xc3,0x00,0xc3,0x99,0xff,0xff,0xff,0xe7,0xe7,0x81,0xe7,0xe7,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xe7,0xe7,0xcf,0xff,0xff,0xff,0x81,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xe7,0xe7,0xff,0xff,0xfc,0xf9,0xf3,0xe7,0xcf,0x9f,0xff,
    0xc3,0x99,0x91,0x89,0x99,0x99,0xc3,0xff,0xe7,0xe7,0xc7,0xe7,0xe7,0xe7,0x81,0xff,
    0xc3,0x99,0xf9,0xf3,0xcf,0x9f,0x81,0xff,0xc3,0x99,0xf9,0xe3,0xf9,0x99,0xc3,0xff,
    0xf9,0xf1,0xe1,0x99,0x80,0xf9,0xf9,0xff,0x81,0x9f,0x83,0xf9,0xf9,0x99,0xc3,0xff,
    0xc3,0x99,0x9f,0x83,0x99,0x99,0xc3,0xff,0x81,0x99,0xf3,0xe7,0xe7,0xe7,0xe7,0xff,
    0xc3,0x99,0x99,0xc3,0x99,0x99,0xc3,0xff,0xc3,0x99,0x99,0xc1,0xf9,0x99,0xc3,0xff,
    0xff,0xff,0xe7,0xff,0xff,0xe7,0xff,0xff,0xff,0xff,0xe7,0xff,0xff,0xe7,0xe7,0xcf,
    0xf1,0xe7,0xcf,0x9f,0xcf,0xe7,0xf1,0xff,0xff,0xff,0x81,0xff,0x81,0xff,0xff,0xff,
    0x8f,0xe7,0xf3,0xf9,0xf3,0xe7,0x8f,0xff,0xc3,0x99,0xf9,0xf3,0xe7,0xff,0xe7,0xff,
    0xff,0xff,0xff,0x00,0x00,0xff,0xff,0xff,0xf7,0xe3,0xc1,0x80,0x80,0xe3,0xc1,0xff,
    0xe7,0xe7,0xe7,0xe7,0xe7,0xe7,0xe7,0xe7,0xff,0xff,0xff,0x00,0x00,0xff,0xff,0xff,
    0xff,0xff,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0x00,0x00,0xff,0xff,0xcf,0xcf,0xcf,0xcf,0xcf,0xcf,0xcf,0xcf,
    0xf3,0xf3,0xf3,0xf3,0xf3,0xf3,0xf3,0xf3,0xff,0xff,0xff,0x1f,0x0f,0xc7,0xe7,0xe7,
    0xe7,0xe7,0xe3,0xf0,0xf8,0xff,0xff,0xff,0xe7,0xe7,0xc7,0x0f,0x1f,0xff,0xff,0xff,
    0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x00,0x00,0x3f,0x1f,0x8f,0xc7,0xe3,0xf1,0xf8,0xfc,
    0xfc,0xf8,0xf1,0xe3,0xc7,0x8f,0x1f,0x3f,0x00,0x00,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,
    0x00,0x00,0xfc,0xfc,0xfc,0xfc,0xfc,0xfc,0xff,0xc3,0x81,0x81,0x81,0x81,0xc3,0xff,
    0xff,0xff,0xff,0xff,0xff,0x00,0x00,0xff,0xc9,0x80,0x80,0x80,0xc1,0xe3,0xf7,0xff,
    0x9f,0x9f,0x9f,0x9f,0x9f,0x9f,0x9f,0x9f,0xff,0xff,0xff,0xf8,0xf0,0xe3,0xe7,0xe7,
    0x3c,0x18,0x81,0xc3,0xc3,0x81,0x18,0x3c,0xff,0xc3,0x81,0x99,0x99,0x81,0xc3,0xff,
    0xe7,0xe7,0x99,0x99,0xe7,0xe7,0xc3,0xff,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,
    0xf7,0xe3,0xc1,0x80,0xc1,0xe3,0xf7,0xff,0xe7,0xe7,0xe7,0x00,0x00,0xe7,0xe7,0xe7,
    0x3f,0x3f,0xcf,0xcf,0x3f,0x3f,0xcf,0xcf,0xe7,0xe7,0xe7,0xe7,0xe7,0xe7,0xe7,0xe7,
    0xff,0xff,0xfc,0xc1,0x89,0xc9,0xc9,0xff,0x00,0x80,0xc0,0xe0,0xf0,0xf8,0xfc,0xfe,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,
    0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,
    0x33,0x33,0xcc,0xcc,0x33,0x33,0xcc,0xcc,0xfc,0xfc,0xfc,0xfc,0xfc,0xfc,0xfc,0xfc,
    0xff,0xff,0xff,0xff,0x33,0x33,0xcc,0xcc,0x00,0x01,0x03,0x07,0x0f,0x1f,0x3f,0x7f,
    0xfc,0xfc,0xfc,0xfc,0xfc,0xfc,0xfc,0xfc,0xe7,0xe7,0xe7,0xe0,0xe0,0xe7,0xe7,0xe7,
    0xff,0xff,0xff,0xff,0xf0,0xf0,0xf0,0xf0,0xe7,0xe7,0xe7,0xe0,0xe0,0xff,0xff,0xff,
    0xff,0xff,0xff,0x07,0x07,0xe7,0xe7,0xe7,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,
    0xff,0xff,0xff,0xe0,0xe0,0xe7,0xe7,0xe7,0xe7,0xe7,0xe7,0x00,0x00,0xff,0xff,0xff,
    0xff,0xff,0xff,0x00,0x00,0xe7,0xe7,0xe7,0xe7,0xe7,0xe7,0x07,0x07,0xe7,0xe7,0xe7,
    0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,
    0xf8,0xf8,0xf8,0xf8,0xf8,0xf8,0xf8,0xf8,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,
    0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,
    0xfc,0xfc,0xfc,0xfc,0xfc,0xfc,0x00,0x00,0xff,0xff,0xff,0xff,0x0f,0x0f,0x0f,0x0f,
    0xf0,0xf0,0xf0,0xf0,0xff,0xff,0xff,0xff,0xe7,0xe7,0xe7,0x07,0x07,0xff,0xff,0xff,
    0x0f,0x0f,0x0f,0x0f,0xff,0xff,0xff,0xff,0x0f,0x0f,0x0f,0x0f,0xf0,0xf0,0xf0,0xf0,
    0x3c,0x66,0x6e,0x6e,0x60,0x62,0x3c,0x00,0x00,0x00,0x3c,0x06,0x3e,0x66,0x3e,0x00,
    0x00,0x60,0x60,0x7c,0x66,0x66,0x7c,0x00,0x00,0x00,0x3c,0x60,0x60,0x60,0x3c,0x00,
    0x00,0x06,0x06,0x3e,0x66,0x66,0x3e,0x00,0x00,0x00,0x3c,0x66,0x7e,0x60,0x3c,0x00,
    0x00,0x0e,0x18,0x3e,0x18,0x18,0x18,0x00,0x00,0x00,0x3e,0x66,0x66,0x3e,0x06,0x7c,
    0x00,0x60,0x60,0x7c,0x66,0x66,0x66,0x00,0x00,0x18,0x00,0x38,0x18,0x18,0x3c,0x00,
    0x00,0x06,0x00,0x06,0x06,0x06,0x06,0x3c,0x00,0x60,0x60,0x6c,0x78,0x6c,0x66,0x00,
    0x00,0x38,0x18,0x18,0x18,0x18,0x3c,0x00,0x00,0x00,0x66,0x7f,0x7f,0x6b,0x63,0x00,
    0x00,0x00,0x7c,0x66,0x66,0x66,0x66,0x00,0x00,0x00,0x3c,0x66,0x66,0x66,0x3c,0x00,
    0x00,0x00,0x7c,0x66,0x66,0x7c,0x60,0x60,0x00,0x00,0x3e,0x66,0x66,0x3e,0x06,0x06,
    0x00,0x00,0x7c,0x66,0x60,0x60,0x60,0x00,0x00,0x00,0x3e,0x60,0x3c,0x06,0x7c,0x00,
    0x00,0x18,0x7e,0x18,0x18,0x18,0x0e,0x00,0x00,0x00,0x66,0x66,0x66,0x66,0x3e,0x00,
    0x00,0x00,0x66,0x66,0x66,0x3c,0x18,0x00,0x00,0x00,0x63,0x6b,0x7f,0x3e,0x36,0x00,
    0x00,0x00,0x66,0x3c,0x18,0x3c,0x66,0x00,0x00,0x00,0x66,0x66,0x66,0x3e,0x0c,0x78,
    0x00,0x00,0x7e,0x0c,0x18,0x30,0x7e,0x00,0x3c,0x30,0x30,0x30,0x30,0x30,0x3c,0x00,
    0x0c,0x12,0x30,0x7c,0x30,0x62,0xfc,0x00,0x3c,0x0c,0x0c,0x0c,0x0c,0x0c,0x3c,0x00,
    0x00,0x18,0x3c,0x7e,0x18,0x18,0x18,0x18,0x00,0x10,0x30,0x7f,0x7f,0x30,0x10,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x18,0x18,0x00,0x00,0x18,0x00,
    0x66,0x66,0x66,0x00,0x00,0x00,0x00,0x00,0x66,0x66,0xff,0x66,0xff,0x66,0x66,0x00,
    0x18,0x3e,0x60,0x3c,0x06,0x7c,0x18,0x00,0x62,0x66,0x0c,0x18,0x30,0x66,0x46,0x00,
    0x3c,0x66,0x3c,0x38,0x67,0x66,0x3f,0x00,0x06,0x0c,0x18,0x00,0x00,0x00,0x00,0x00,
    0x0c,0x18,0x30,0x30,0x30,0x18,0x0c,0x00,0x30,0x18,0x0c,0x0c,0x0c,0x18,0x30,0x00,
    0x00,0x66,0x3c,0xff,0x3c,0x66,0x00,0x00,0x00,0x18,0x18,0x7e,0x18,0x18,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30,0x00,0x00,0x00,0x7e,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x03,0x06,0x0c,0x18,0x30,0x60,0x00,
    0x3c,0x66,0x6e,0x76,0x66,0x66,0x3c,0x00,0x18,0x18,0x38,0x18,0x18,0x18,0x7e,0x00,
    0x3c,0x66,0x06,0x0c,0x30,0x60,0x7e,0x00,0x3c,0x66,0x06,0x1c,0x06,0x66,0x3c,0x00,
    0x06,0x0e,0x1e,0x66,0x7f,0x06,0x06,0x00,0x7e,0x60,0x7c,0x06,0x06,0x66,0x3c,0x00,
    0x3c,0x66,0x60,0x7c,0x66,0x66,0x3c,0x00,0x7e,0x66,0x0c,0x18,0x18,0x18,0x18,0x00,
    0x3c,0x66,0x66,0x3c,0x66,0x66,0x3c,0x00,0x3c,0x66,0x66,0x3e,0x06,0x66,0x3c,0x00,
    0x00,0x00,0x18,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x18,0x00,0x00,0x18,0x18,0x30,
    0x0e,0x18,0x30,0x60,0x30,0x18,0x0e,0x00,0x00,0x00,0x7e,0x00,0x7e,0x00,0x00,0x00,
    0x70,0x18,0x0c,0x06,0x0c,0x18,0x70,0x00,0x3c,0x66,0x06,0x0c,0x18,0x00,0x18,0x00,
    0x00,0x00,0x00,0xff,0xff,0x00,0x00,0x00,0x18,0x3c,0x66,0x7e,0x66,0x66,0x66,0x00,
    0x7c,0x66,0x66,0x7c,0x66,0x66,0x7c,0x00,0x3c,0x66,0x60,0x60,0x60,0x66,0x3c,0x00,
    0x78,0x6c,0x66,0x66,0x66,0x6c,0x78,0x00,0x7e,0x60,0x60,0x78,0x60,0x60,0x7e,0x00,
    0x7e,0x60,0x60,0x78,0x60,0x60,0x60,0x00,0x3c,0x66,0x60,0x6e,0x66,0x66,0x3c,0x00,
    0x66,0x66,0x66,0x7e,0x66,0x66,0x66,0x00,0x3c,0x18,0x18,0x18,0x18,0x18,0x3c,0x00,
    0x1e,0x0c,0x0c,0x0c,0x0c,0x6c,0x38,0x00,0x66,0x6c,0x78,0x70,0x78,0x6c,0x66,0x00,
    0x60,0x60,0x60,0x60,0x60,0x60,0x7e,0x00,0x63,0x77,0x7f,0x6b,0x63,0x63,0x63,0x00,
    0x66,0x76,0x7e,0x7e,0x6e,0x66,0x66,0x00,0x3c,0x66,0x66,0x66,0x66,0x66,0x3c,0x00,
    0x7c,0x66,0x66,0x7c,0x60,0x60,0x60,0x00,0x3c,0x66,0x66,0x66,0x66,0x3c,0x0e,0x00,
    0x7c,0x66,0x66,0x7c,0x78,0x6c,0x66,0x00,0x3c,0x66,0x60,0x3c,0x06,0x66,0x3c,0x00,
    0x7e,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x66,0x66,0x66,0x66,0x66,0x66,0x3c,0x00,
    0x66,0x66,0x66,0x66,0x66,0x3c,0x18,0x00,0x63,0x63,0x63,0x6b,0x7f,0x77,0x63,0x00,
    0x66,0x66,0x3c,0x18,0x3c,0x66,0x66,0x00,0x66,0x66,0x66,0x3c,0x18,0x18,0x18,0x00,
    0x7e,0x06,0x0c,0x18,0x30,0x60,0x7e,0x00,0x18,0x18,0x18,0xff,0xff,0x18,0x18,0x18,
    0xc0,0xc0,0x30,0x30,0xc0,0xc0,0x30,0x30,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
    0x33,0x33,0xcc,0xcc,0x33,0x33,0xcc,0xcc,0x33,0x99,0xcc,0x66,0x33,0x99,0xcc,0x66,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
    0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,
    0xcc,0xcc,0x33,0x33,0xcc,0xcc,0x33,0x33,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,
    0x00,0x00,0x00,0x00,0xcc,0xcc,0x33,0x33,0xcc,0x99,0x33,0x66,0xcc,0x99,0x33,0x66,
    0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x18,0x18,0x18,0x1f,0x1f,0x18,0x18,0x18,
    0x00,0x00,0x00,0x00,0x0f,0x0f,0x0f,0x0f,0x18,0x18,0x18,0x1f,0x1f,0x00,0x00,0x00,
    0x00,0x00,0x00,0xf8,0xf8,0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,
    0x00,0x00,0x00,0x1f,0x1f,0x18,0x18,0x18,0x18,0x18,0x18,0xff,0xff,0x00,0x00,0x00,
    0x00,0x00,0x00,0xff,0xff,0x18,0x18,0x18,0x18,0x18,0x18,0xf8,0xf8,0x18,0x18,0x18,
    0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xe0,0xe0,0xe0,0xe0,0xe0,0xe0,0xe0,0xe0,
    0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xff,
    0x01,0x03,0x06,0x6c,0x78,0x70,0x60,0x00,0x00,0x00,0x00,0x00,0xf0,0xf0,0xf0,0xf0,
    0x0f,0x0f,0x0f,0x0f,0x00,0x00,0x00,0x00,0x18,0x18,0x18,0xf8,0xf8,0x00,0x00,0x00,
    0xf0,0xf0,0xf0,0xf0,0x00,0x00,0x00,0x00,0xf0,0xf0,0xf0,0xf0,0x0f,0x0f,0x0f,0x0f,
    0xc3,0x99,0x91,0x91,0x9f,0x99,0xc3,0xff,0xff,0xff,0xc3,0xf9,0xc1,0x99,0xc1,0xff,
    0xff,0x9f,0x9f,0x83,0x99,0x99,0x83,0xff,0xff,0xff,0xc3,0x9f,0x9f,0x9f,0xc3,0xff,
    0xff,0xf9,0xf9,0xc1,0x99,0x99,0xc1,0xff,0xff,0xff,0xc3,0x99,0x81,0x9f,0xc3,0xff,
    0xff,0xf1,0xe7,0xc1,0xe7,0xe7,0xe7,0xff,0xff,0xff,0xc1,0x99,0x99,0xc1,0xf9,0x83,
    0xff,0x9f,0x9f,0x83,0x99,0x99,0x99,0xff,0xff,0xe7,0xff,0xc7,0xe7,0xe7,0xc3,0xff,
    0xff,0xf9,0xff,0xf9,0xf9,0xf9,0xf9,0xc3,0xff,0x9f,0x9f,0x93,0x87,0x93,0x99,0xff,
    0xff,0xc7,0xe7,0xe7,0xe7,0xe7,0xc3,0xff,0xff,0xff,0x99,0x80,0x80,0x94,0x9c,0xff,
    0xff,0xff,0x83,0x99,0x99,0x99,0x99,0xff,0xff,0xff,0xc3,0x99,0x99,0x99,0xc3,0xff,
    0xff,0xff,0x83,0x99,0x99,0x83,0x9f,0x9f,0xff,0xff,0xc1,0x99,0x99,0xc1,0xf9,0xf9,
    0xff,0xff,0x83,0x99,0x9f,0x9f,0x9f,0xff,0xff,0xff,0xc1,0x9f,0xc3,0xf9,0x83,0xff,
    0xff,0xe7,0x81,0xe7,0xe7,0xe7,0xf1,0xff,0xff,0xff,0x99,0x99,0x99,0x99,0xc1,0xff,
    0xff,0xff,0x99,0x99,0x99,0xc3,0xe7,0xff,0xff,0xff,0x9c,0x94,0x80,0xc1,0xc9,0xff,
    0xff,0xff,0x99,0xc3,0xe7,0xc3,0x99,0xff,0xff,0xff,0x99,0x99,0x99,0xc1,0xf3,0x87,
    0xff,0xff,0x81,0xf3,0xe7,0xcf,0x81,0xff,0xc3,0xcf,0xcf,0xcf,0xcf,0xcf,0xc3,0xff,
    0xf3,0xed,0xcf,0x83,0xcf,0x9d,0x03,0xff,0xc3,0xf3,0xf3,0xf3,0xf3,0xf3,0xc3,0xff,
    0xff,0xe7,0xc3,0x81,0xe7,0xe7,0xe7,0xe7,0xff,0xef,0xcf,0x80,0x80,0xcf,0xef,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xe7,0xe7,0xe7,0xe7,0xff,0xff,0xe7,0xff,
    0x99,0x99,0x99,0xff,0xff,0xff,0xff,0xff,0x99,0x99,0x00,0x99,0x00,0x99,0x99,0xff,
    0xe7,0xc1,0x9f,0xc3,0xf9,0x83,0xe7,0xff,0x9d,0x99,0xf3,0xe7,0xcf,0x99,0xb9,0xff,
    0xc3,0x99,0xc3,0xc7,0x98,0x99,0xc0,0xff,0xf9,0xf3,0xe7,0xff,0xff,0xff,0xff,0xff,
    0xf3,0xe7,0xcf,0xcf,0xcf,0xe7,0xf3,0xff,0xcf,0xe7,0xf3,0xf3,0xf3,0xe7,0xcf,0xff,
    0xff,0x99,0xc3,0x00,0xc3,0x99,0xff,0xff,0xff,0xe7,0xe7,0x81,0xe7,0xe7,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xe7,0xe7,0xcf,0xff,0xff,0xff,0x81,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xe7,0xe7,0xff,0xff,0xfc,0xf9,0xf3,0xe7,0xcf,0x9f,0xff,
    0xc3,0x99,0x91,0x89,0x99,0x99,0xc3,0xff,0xe7,0xe7,0xc7,0xe7,0xe7,0xe7,0x81,0xff,
    0xc3,0x99,0xf9,0xf3,0xcf,0x9f,0x81,0xff,0xc3,0x99,0xf9,0xe3,0xf9,0x99,0xc3,0xff,
    0xf9,0xf1,0xe1,0x99,0x80,0xf9,0xf9,0xff,0x81,0x9f,0x83,0xf9,0xf9,0x99,0xc3,0xff,
    0xc3,0x99,0x9f,0x83,0x99,0x99,0xc3,0xff,0x81,0x99,0xf3,0xe7,0xe7,0xe7,0xe7,0xff,
    0xc3,0x99,0x99,0xc3,0x99,0x99,0xc3,0xff,0xc3,0x99,0x99,0xc1,0xf9,0x99,0xc3,0xff,
    0xff,0xff,0xe7,0xff,0xff,0xe7,0xff,0xff,0xff,0xff,0xe7,0xff,0xff,0xe7,0xe7,0xcf,
    0xf1,0xe7,0xcf,0x9f,0xcf,0xe7,0xf1,0xff,0xff,0xff,0x81,0xff,0x81,0xff,0xff,0xff,
    0x8f,0xe7,0xf3,0xf9,0xf3,0xe7,0x8f,0xff,0xc3,0x99,0xf9,0xf3,0xe7,0xff,0xe7,0xff,
    0xff,0xff,0xff,0x00,0x00,0xff,0xff,0xff,0xe7,0xc3,0x99,0x81,0x99,0x99,0x99,0xff,
    0x83,0x99,0x99,0x83,0x99,0x99,0x83,0xff,0xc3,0x99,0x9f,0x9f,0x9f,0x99,0xc3,0xff,
    0x87,0x93,0x99,0x99,0x99,0x93,0x87,0xff,0x81,0x9f,0x9f,0x87,0x9f,0x9f,0x81,0xff,
    0x81,0x9f,0x9f,0x87,0x9f,0x9f,0x9f,0xff,0xc3,0x99,0x9f,0x91,0x99,0x99,0xc3,0xff,
    0x99,0x99,0x99,0x81,0x99,0x99,0x99,0xff,0xc3,0xe7,0xe7,0xe7,0xe7,0xe7,0xc3,0xff,
    0xe1,0xf3,0xf3,0xf3,0xf3,0x93,0xc7,0xff,0x99,0x93,0x87,0x8f,0x87,0x93,0x99,0xff,
    0x9f,0x9f,0x9f,0x9f,0x9f,0x9f,0x81,0xff,0x9c,0x88,0x80,0x94,0x9c,0x9c,0x9c,0xff,
    0x99,0x89,0x81,0x81,0x91,0x99,0x99,0xff,0xc3,0x99,0x99,0x99,0x99,0x99,0xc3,0xff,
    0x83,0x99,0x99,0x83,0x9f,0x9f,0x9f,0xff,0xc3,0x99,0x99,0x99,0x99,0xc3,0xf1,0xff,
    0x83,0x99,0x99,0x83,0x87,0x93,0x99,0xff,0xc3,0x99,0x9f,0xc3,0xf9,0x99,0xc3,0xff,
    0x81,0xe7,0xe7,0xe7,0xe7,0xe7,0xe7,0xff,0x99,0x99,0x99,0x99,0x99,0x99,0xc3,0xff,
    0x99,0x99,0x99,0x99,0x99,0xc3,0xe7,0xff,0x9c,0x9c,0x9c,0x94,0x80,0x88,0x9c,0xff,
    0x99,0x99,0xc3,0xe7,0xc3,0x99,0x99,0xff,0x99,0x99,0x99,0xc3,0xe7,0xe7,0xe7,0xff,
    0x81,0xf9,0xf3,0xe7,0xcf,0x9f,0x81,0xff,0xe7,0xe7,0xe7,0x00,0x00,0xe7,0xe7,0xe7,
    0x3f,0x3f,0xcf,0xcf,0x3f,0x3f,0xcf,0xcf,0xe7,0xe7,0xe7,0xe7,0xe7,0xe7,0xe7,0xe7,
    0xcc,0xcc,0x33,0x33,0xcc,0xcc,0x33,0x33,0xcc,0x66,0x33,0x99,0xcc,0x66,0x33,0x99,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,
    0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,
    0x33,0x33,0xcc,0xcc,0x33,0x33,0xcc,0xcc,0xfc,0xfc,0xfc,0xfc,0xfc,0xfc,0xfc,0xfc,
    0xff,0xff,0xff,0xff,0x33,0x33,0xcc,0xcc,0x33,0x66,0xcc,0x99,0x33,0x66,0xcc,0x99,
    0xfc,0xfc,0xfc,0xfc,0xfc,0xfc,0xfc,0xfc,0xe7,0xe7,0xe7,0xe0,0xe0,0xe7,0xe7,0xe7,
    0xff,0xff,0xff,0xff,0xf0,0xf0,0xf0,0xf0,0xe7,0xe7,0xe7,0xe0,0xe0,0xff,0xff,0xff,
    0xff,0xff,0xff,0x07,0x07,0xe7,0xe7,0xe7,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,
    0xff,0xff,0xff,0xe0,0xe0,0xe7,0xe7,0xe7,0xe7,0xe7,0xe7,0x00,0x00,0xff,0xff,0xff,
    0xff,0xff,0xff,0x00,0x00,0xe7,0xe7,0xe7,0xe7,0xe7,0xe7,0x07,0x07,0xe7,0xe7,0xe7,
    0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,
    0xf8,0xf8,0xf8,0xf8,0xf8,0xf8,0xf8,0xf8,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,
    0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,
    0xfe,0xfc,0xf9,0x93,0x87,0x8f,0x9f,0xff,0xff,0xff,0xff,0xff,0x0f,0x0f,0x0f,0x0f,
    0xf0,0xf0,0xf0,0xf0,0xff,0xff,0xff,0xff,0xe7,0xe7,0xe7,0x07,0x07,0xff,0xff,0xff,
    0x0f,0x0f,0x0f,0x0f,0xff,0xff,0xff,0xff,0x0f,0x0f,0x0f,0x0f,0xf0,0xf0,0xf0,0xf0,
    // clang-format on
};

////////////////////////////////////////////////////////////////////////////////
// The C64 was PETSCII based, not ASCII based so we'll have to remap our ASCII
// codes into this character map.
//
// The great thing about standards is that there's so many to choose from!
////////////////////////////////////////////////////////////////////////////////
uint8_t Commodore64ASCIIRemap[256] = {
    // clang-format off
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 0
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 16
    0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F, // 32
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x00,0x00,0x3D,0x00,0x00, // 48
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F, // 64
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x00,0x00,0x00,0x00,0x00, // 80
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 96
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 112
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 128 (Extended ASCII)
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 144
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 160
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 176
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 192
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 208
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 224
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 240
    // clang-format on
};

////////////////////////////////////////////////////////////////////////////////
// And here's the C64 balloon sprite from page 71 of the user manual.
// I include this here because reasons.
////////////////////////////////////////////////////////////////////////////////
uint8_t Commodore64BalloonSprite[] = {
    0,   127, 0,   1,   255, 192, 3,   255, 224, 3,   231, 224, 7,
    217, 240, 7,   223, 240, 7,   217, 240, 3,   231, 224, 3,   255,
    224, 3,   255, 224, 2,   255, 160, 1,   127, 64,  1,   62,  64,
    0,   156, 128, 0,   156, 128, 0,   73,  0,   0,   73,  0,   0,
    62,  0,   0,   62,  0,   0,   62,  0,   0,   28,  0};

void Image_Plot_Pixel(const ImageBGRA &image, int x, int y, PixelBGRA &color) {
  if (x >= 0 && x < image.width && y >= 0 && y < image.height) {
    *reinterpret_cast<PixelBGRA *>(reinterpret_cast<uint8_t *>(image.data) +
                                   sizeof(PixelBGRA) * x + image.stride * y) =
        color;
  }
}

void Image_Plot_C64_Sprite(const ImageBGRA &image, int x, int y,
                           PixelBGRA color) {
  uint8_t *bytes = reinterpret_cast<uint8_t *>(image.data);
  for (int j = 0; j < 21; ++j) {
    for (int c = 0; c < 3; ++c) {
      uint8_t bitmap = Commodore64BalloonSprite[c + 3 * j];
      for (int i = 0; i < 8; ++i) {
        if (bitmap & 0x80) {
          Image_Plot_Pixel(image, x + 8 * c + i, y + j, color);
        }
        bitmap = bitmap << 1;
      }
    }
  }
}

void Image_Plot_Char_PETSCII(const ImageBGRA &image, int x, int y,
                             PixelBGRA color, uint8_t character) {
  uint8_t *bytes = reinterpret_cast<uint8_t *>(image.data);
  for (int j = 0; j < 8; ++j) {
    uint8_t bitmap = Commodore64CharacterGeneratorROM[8 * character + j];
    for (int i = 0; i < 8; ++i) {
      if (bitmap & 0x80) {
        Image_Plot_Pixel(image, x + i, y + j, color);
      }
      bitmap = bitmap << 1;
    }
  }
}

void Image_Plot_Char_ASCII(const ImageBGRA &image, int x, int y,
                           PixelBGRA color, uint8_t character) {
  Image_Plot_Char_PETSCII(image, x, y, color, Commodore64ASCIIRemap[character]);
}

void Image_Plot_String(const ImageBGRA &image, int x, int y, PixelBGRA color,
                       const char *string) {
  while (*string != 0) {
    Image_Plot_Char_ASCII(image, x, y, color, static_cast<uint8_t>(*string));
    x += 8;
    ++string;
  }
}

void Image_Fill_Color(ImageBGRA &image, PixelBGRA color) {
  uint8_t *bytes = reinterpret_cast<uint8_t *>(image.data);
  for (int y = 0; y < image.height; ++y) {
    for (int x = 0; x < image.width; ++x) {
      *reinterpret_cast<PixelBGRA *>(bytes + sizeof(PixelBGRA) * x +
                                     image.stride * y) = color;
    }
  }
}

void Image_Fill_Fun(ImageBGRA &image) {
  uint8_t *bytes = reinterpret_cast<uint8_t *>(image.data);
  for (int y = 0; y < image.height; ++y) {
    for (int x = 0; x < image.width; ++x) {
      PixelBGRA &pixel = *reinterpret_cast<PixelBGRA *>(
          bytes + sizeof(PixelBGRA) * x + image.stride * y);
      pixel.B = (x * 2 + y * 4);
      pixel.G = (x * 4 + y * 2);
      pixel.R = (x * 4 + y * 4);
      pixel.A = 0xFF;
    }
  }
}

void Image_Fill_Commodore64(ImageBGRA &image) {
  PixelBGRA db{0xFF, 0x00, 0x00, 0xFF}; // Dark Blue
  PixelBGRA lb{0xFF, 0x80, 0x80, 0xFF}; // Light Blue
  Image_Fill_Color(image, db);
  Image_Plot_String(image, 8 * 4, 8 * 1, lb, "**** COMMODORE 64 BASIC V2 ****");
  Image_Plot_String(image, 8 * 1, 8 * 3, lb,
                    "64K RAM SYSTEM  38911 BASIC BYTES FREE");
  Image_Plot_String(image, 8 * 0, 8 * 5, lb, "READY.");
  Image_Plot_String(image, 8 * 0, 8 * 6, lb, "1 REM UP, UP, AND AWAY!");
  Image_Plot_String(image, 8 * 0, 8 * 7, lb, "5 PRINT \"(CLR/HOME)\"");
  Image_Plot_String(image, 8 * 0, 8 * 8, lb,
                    "10 V=53248 : REM START OF  DISPLAY CHIP");
  Image_Plot_String(image, 8 * 0, 8 * 9, lb,
                    "11 POKE V+21,4 : REM ENABLE SPRITE 2");
  Image_Plot_String(image, 8 * 0, 8 * 10, lb,
                    "12 POKE 2042,13 : REM SPRITE 2 DATA FROM");
  Image_Plot_String(image, 8 * 0, 8 * 11, lb, "13TH BLK");
  Image_Plot_String(image, 8 * 0, 8 * 12, lb,
                    "20 FOR N = 0 TO 62: READ Q : POKE 932+N,");
  Image_Plot_String(image, 8 * 0, 8 * 13, lb, "Q: NEXT");
  Image_Plot_String(image, 8 * 0, 8 * 14, lb, "30 FOR X = 0 TO 200");
  Image_Plot_String(image, 8 * 0, 8 * 15, lb,
                    "40 POKE V+4,X: REM UPDATE X COORDINATES");
  Image_Plot_String(image, 8 * 0, 8 * 16, lb,
                    "50 POKE V+5,X: REM UPDATE Y COORDINATES");
  Image_Plot_String(image, 8 * 0, 8 * 17, lb, "60 NEXT X");
  Image_Plot_String(image, 8 * 0, 8 * 18, lb, "70 GOTO 30");
  Image_Plot_String(image, 8 * 0, 8 * 19, lb,
                    "200 DATA 0,127,0,1,255,192,3,255,224,3,2");
  Image_Plot_String(image, 8 * 0, 8 * 20, lb, "31,224");
  Image_Plot_String(image, 8 * 0, 8 * 21, lb,
                    "210 DATA 7,217,240,7,223,240,7,217,240,3");
  Image_Plot_String(image, 8 * 0, 8 * 22, lb, ",231,224");
  Image_Plot_String(image, 8 * 0, 8 * 23, lb,
                    "220 DATA 3,255,224,3,255,224,2,255,160,1");
  Image_Plot_String(image, 8 * 0, 8 * 24, lb, ",127,64");
  Image_Plot_C64_Sprite(image, 128, 128, PixelBGRA{0xFF, 0xFF, 0xFF, 0xFF});
}

void Image_Fill_Sample(ImageBGRA &image) {
  Image_Fill_Fun(image);
  for (int y = 0; y < 16; ++y) {
    for (int x = 0; x < 32; ++x) {
      Image_Plot_Char_PETSCII(image, 8 + 9 * x + 1, 8 + 9 * y + 1,
                              PixelBGRA{0x00, 0x00, 0x00, 0xFF}, x + 32 * y);
    }
  }
  for (int y = 0; y < 16; ++y) {
    for (int x = 0; x < 32; ++x) {
      Image_Plot_Char_PETSCII(image, 8 + 9 * x, 8 + 9 * y,
                              PixelBGRA{0xFF, 0xFF, 0xFF, 0xFF}, x + 32 * y);
    }
  }
}

void Image_Fill_Commodore64(void *data, uint32_t width, uint32_t height,
                            uint32_t stride) {
  Image_Fill_Commodore64(ImageBGRA{data, width, height, stride});
}

void Image_Fill_Sample(void *data, uint32_t width, uint32_t height,
                       uint32_t stride) {
  Image_Fill_Sample(ImageBGRA{data, width, height, stride});
}