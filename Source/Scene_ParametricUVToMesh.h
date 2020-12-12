#pragma once

#include "Core_Object.h"
#include "Scene_IMesh.h"
#include <memory>
#include <stdint.h>

class IParametricUV;

class ParametricUVToMesh : public Object, public IMesh {
public:
  ParametricUVToMesh(std::shared_ptr<IParametricUV> shape, uint32_t stepsInU,
                     uint32_t stepsInV);
  uint32_t getVertexCount() const override;
  uint32_t getIndexCount() const override;
  void copyVertices(void *to, uint32_t stride) const override;
  void copyNormals(void *to, uint32_t stride) const override;
  void copyIndices(void *to, uint32_t stride) const override;

private:
  std::shared_ptr<IParametricUV> m_shape;
  uint32_t m_stepsInU;
  uint32_t m_stepsInV;
};