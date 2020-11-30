#pragma once

#include <stdint.h>

class Mesh {
public:
  virtual uint32_t getVertexCount() = 0;
  virtual uint32_t getIndexCount() = 0;
  virtual void copyVertices(void *to, uint32_t stride) = 0;
  virtual void copyNormals(void *to, uint32_t stride) = 0;
  virtual void copyIndices(void *to, uint32_t stride) = 0;
};