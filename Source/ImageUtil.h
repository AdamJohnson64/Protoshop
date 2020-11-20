#pragma once

#include <cstdint>

void Image_Fill_Commodore64(void* bgra, uint32_t width, uint32_t height, uint32_t stride);
void Image_Fill_Sample(void* bgra, uint32_t width, uint32_t height, uint32_t stride);