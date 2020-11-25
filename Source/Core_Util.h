#pragma once

#include <stdint.h>

// Global render target dimensions for simplicity.
#define RENDERTARGET_WIDTH 640
#define RENDERTARGET_HEIGHT 480

// Align a value up to the next multiple of a designated size.
// e.g. AlignUp(5, 256) == 256
uint32_t AlignUp(uint32_t size, uint32_t alignSize);