#include "Core_Util.h"

const int RENDERTARGET_WIDTH = 640;
const int RENDERTARGET_HEIGHT = 480;

uint32_t AlignUp(uint32_t size, uint32_t alignSize) {
  return size == 0 ? 0 : ((size - 1) / alignSize + 1) * alignSize;
}