#include "Core_Util.h"

const int RENDERTARGET_WIDTH = 1024;
const int RENDERTARGET_HEIGHT = 1024;

uint32_t AlignUp(uint32_t size, uint32_t alignSize) {
  return size == 0 ? 0 : ((size - 1) / alignSize + 1) * alignSize;
}