#pragma once

#include "Core_IImage.h"
#include <cstdint>
#include <memory>

void Image_Fill_Sample(void *bgra, uint32_t width, uint32_t height,
                       uint32_t stride);

std::unique_ptr<IImage> Image_BrickAlbedo(uint32_t width, uint32_t height);

std::unique_ptr<IImage> Image_BrickDepth(uint32_t width, uint32_t height);

std::unique_ptr<IImage> Image_BrickNormal(uint32_t width, uint32_t height);

std::unique_ptr<IImage> Image_Commodore64(uint32_t width, uint32_t height);

std::unique_ptr<IImage> Image_Sample(uint32_t width, uint32_t height);

std::unique_ptr<IImage> Image_SolidColor(uint32_t width, uint32_t height,
                                         uint32_t color);