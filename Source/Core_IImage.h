#pragma once

#include "Core_Object.h"
#include <cstdint>
#include <dxgi.h>
#include <memory>

class IImage {
public:
  virtual ~IImage() = default;
  virtual uint32_t GetWidth() const = 0;
  virtual uint32_t GetHeight() const = 0;
  virtual uint32_t GetStride() const = 0;
  virtual DXGI_FORMAT GetFormat() const = 0;
  virtual const void *GetData() const = 0;
};

// Create an image from pixel data which has been allocated with malloc/new.
// The provided image data will be automatically deleted along with this image.
// DO NOT USE! It's generally a bad idea to delete memory that was provided by
// something else since it's not guaranteed that memory was actually allocated
// in the manner this object assumes. Prefer a copy instead.
std::unique_ptr<IImage> CreateImage_AutoDelete(uint32_t width, uint32_t height,
                                               uint32_t stride,
                                               DXGI_FORMAT format, void *data);

// Create an image from pixel data and make a copy of the provided data.
// The image data will be automatically managed.
std::unique_ptr<IImage> CreateImage_CopyPixels(uint32_t width, uint32_t height,
                                               uint32_t stride,
                                               DXGI_FORMAT format,
                                               const void *data);

// Create an image from externally managed pixel data.
// CAUTION: It is assumed that the pixel data will remain static throughout the
// lifetime of this object. DO NOT DELETE THE PIXEL DATA AT THE MEMORY LOCATION
// PROVIDED HERE; this may (at best) cause non-deterministic behavior and will
// almost certainly crash the application.
std::unique_ptr<IImage> CreateImage_Unowned(uint32_t width, uint32_t height,
                                            uint32_t stride, DXGI_FORMAT format,
                                            const void *data);