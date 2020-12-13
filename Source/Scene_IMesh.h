#pragma once

#include <stdint.h>

class IMesh {
public:
  virtual uint32_t getVertexCount() const = 0;
  virtual uint32_t getIndexCount() const = 0;
  virtual void copyVertices(void *to, uint32_t stride) const = 0;
  virtual void copyNormals(void *to, uint32_t stride) const = 0;
  virtual void copyTexcoords(void *to, uint32_t stride) const = 0;
  virtual void copyIndices(void *to, uint32_t stride) const = 0;
};