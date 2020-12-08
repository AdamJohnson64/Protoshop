#include "Core_IImage.h"
#include <assert.h>

ImageBase::ImageBase()
    : m_Width(0), m_Height(0), m_Stride(0), m_Format(DXGI_FORMAT_UNKNOWN) {}

ImageBase::ImageBase(uint32_t width, uint32_t height, uint32_t stride,
                     DXGI_FORMAT format)
    : m_Width(width), m_Height(height), m_Stride(stride), m_Format(format) {}

uint32_t ImageBase::GetWidth() const { return m_Width; }

uint32_t ImageBase::GetHeight() const { return m_Height; }

uint32_t ImageBase::GetStride() const { return m_Stride; }

DXGI_FORMAT ImageBase::GetFormat() const { return m_Format; }

ImageUnowned::ImageUnowned() : m_Data(nullptr) {}

ImageUnowned::ImageUnowned(uint32_t width, uint32_t height, uint32_t stride,
                           DXGI_FORMAT format, void *data)
    : ImageBase(width, height, stride, format), m_Data(data) {
  assert(data != nullptr);
}

const void *ImageUnowned::GetData() const {
  // If this object has been moved then its data could be NULL.
  assert(m_Data != nullptr);
  return m_Data;
}

ImageOwned::ImageOwned(uint32_t width, uint32_t height, uint32_t stride,
                       DXGI_FORMAT format, void *data)
    : ImageUnowned(width, height, stride, format, data) {}

ImageOwned::ImageOwned(ImageOwned &&move) {
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

ImageOwned::~ImageOwned() {
  // If this object has been moved then its data could be NULL.
  if (m_Data != nullptr) {
    delete m_Data;
    m_Data = nullptr;
  }
}