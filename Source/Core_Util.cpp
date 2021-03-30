#include "Core_Util.h"

uint32_t AlignUp(uint32_t size, uint32_t alignSize) {
  return size == 0 ? 0 : ((size - 1) / alignSize + 1) * alignSize;
}