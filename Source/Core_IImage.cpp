#include "Core_IImage.h"
#include <assert.h>

////////////////////////////////////////////////////////////////////////////////
// DO NOT USE - The base image is not constructible.
class ImageBase : public Object, public IImage {
public:
  uint32_t GetWidth() const override;
  uint32_t GetHeight() const override;
  uint32_t GetStride() const override;
  DXGI_FORMAT GetFormat() const override;

protected:
  ImageBase();
  ImageBase(uint32_t width, uint32_t height, uint32_t stride,
            DXGI_FORMAT format);
  uint32_t m_Width;
  uint32_t m_Height;
  uint32_t m_Stride;
  DXGI_FORMAT m_Format;
};

ImageBase::ImageBase()
    : m_Width(0), m_Height(0), m_Stride(0), m_Format(DXGI_FORMAT_UNKNOWN) {}

ImageBase::ImageBase(uint32_t width, uint32_t height, uint32_t stride,
                     DXGI_FORMAT format)
    : m_Width(width), m_Height(height), m_Stride(stride), m_Format(format) {}

uint32_t ImageBase::GetWidth() const { return m_Width; }

uint32_t ImageBase::GetHeight() const { return m_Height; }

uint32_t ImageBase::GetStride() const { return m_Stride; }

DXGI_FORMAT ImageBase::GetFormat() const { return m_Format; }

////////////////////////////////////////////////////////////////////////////////
// An unowned image is not assumed to "own" its image data.
// The data will not be deleted upon destruction.
//
// Use this object to pass around temporary stubs for images to declare their
// format and dimensions where the data itself is maintained somewhere else.
class ImageUnowned : public ImageBase {
public:
  ImageUnowned();
  ImageUnowned(uint32_t width, uint32_t height, uint32_t stride,
               DXGI_FORMAT format, const void *owneddata);
  const void *GetData() const override;

protected:
  const void *m_Data;
};

ImageUnowned::ImageUnowned() : m_Data(nullptr) {}

ImageUnowned::ImageUnowned(uint32_t width, uint32_t height, uint32_t stride,
                           DXGI_FORMAT format, const void *data)
    : ImageBase(width, height, stride, format), m_Data(data) {
  assert(data != nullptr);
}

const void *ImageUnowned::GetData() const {
  // If this object has been moved then its data could be NULL.
  assert(m_Data != nullptr);
  return m_Data;
}

std::unique_ptr<IImage> CreateImage_Unowned(uint32_t width, uint32_t height,
                                            uint32_t stride, DXGI_FORMAT format,
                                            const void *data) {
  return std::unique_ptr<IImage>(
      new ImageUnowned(width, height, stride, format, data));
}

////////////////////////////////////////////////////////////////////////////////
// An owned image controls the lifetime of its internal image data.
// This object will delete its image data on destruction.
//
// Use this object when returning images from utility functions.
class ImageAutoDelete : public ImageUnowned {
public:
  ImageAutoDelete(uint32_t width, uint32_t height, uint32_t stride,
                  DXGI_FORMAT format, void *owneddata);
  ImageAutoDelete(const ImageAutoDelete &copy) = delete;
  ImageAutoDelete(ImageAutoDelete &&move);
  ~ImageAutoDelete();
};

ImageAutoDelete::ImageAutoDelete(uint32_t width, uint32_t height,
                                 uint32_t stride, DXGI_FORMAT format,
                                 void *data)
    : ImageUnowned(width, height, stride, format, data) {}

ImageAutoDelete::ImageAutoDelete(ImageAutoDelete &&move) {
  m_Width = move.m_Width;
  m_Height = move.m_Height;
  m_Stride = move.m_Stride;
  m_Format = move.m_Format;
  m_Data = move.m_Data;
  move.m_Width = 0;
  move.m_Height = 0;
  move.m_Stride = 0;
  move.m_Format = DXGI_FORMAT_UNKNOWN;
  // The moved object is now invalid.
  move.m_Data = nullptr;
}

ImageAutoDelete::~ImageAutoDelete() {
  // If this object has been moved then its data could be NULL.
  if (m_Data != nullptr) {
    delete m_Data;
    m_Data = nullptr;
  }
}

std::unique_ptr<IImage> CreateImage_AutoDelete(uint32_t width, uint32_t height,
                                               uint32_t stride,
                                               DXGI_FORMAT format, void *data) {
  return std::unique_ptr<IImage>(
      new ImageAutoDelete(width, height, stride, format, data));
}

std::unique_ptr<IImage> CreateImage_CopyPixels(uint32_t width, uint32_t height,
                                               uint32_t stride,
                                               DXGI_FORMAT format,
                                               const void *data) {
  void *copydata = new uint8_t[stride * height];
  memcpy(copydata, data, stride * height);
  return CreateImage_AutoDelete(width, height, stride, format, copydata);
}