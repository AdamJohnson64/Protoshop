#pragma once

#include "Core_Object.h"
#include <cstdint>
#include <dxgi.h>

class IImage {
public:
  virtual uint32_t GetWidth() const = 0;
  virtual uint32_t GetHeight() const = 0;
  virtual uint32_t GetStride() const = 0;
  virtual DXGI_FORMAT GetFormat() const = 0;
  virtual const void *GetData() const = 0;
};

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
               DXGI_FORMAT format, void *owneddata);
  const void *GetData() const override;

protected:
  void *m_Data;
};

////////////////////////////////////////////////////////////////////////////////
// An owned image controls the lifetime of its internal image data.
// This object will delete its image data on destruction.
//
// Use this object when returning images from utility functions.
class ImageOwned : public ImageUnowned {
public:
  ImageOwned(uint32_t width, uint32_t height, uint32_t stride,
             DXGI_FORMAT format, void *owneddata);
  ImageOwned(const ImageOwned &copy) = delete;
  ImageOwned(ImageOwned &&move);
  ~ImageOwned();
};