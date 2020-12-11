#include "Scene_ParametricUVToMesh.h"
#include "Core_Math.h"
#include "Scene_ParametricUV.h"

ParametricUVToMesh::ParametricUVToMesh(std::shared_ptr<ParametricUV> shape,
                                       uint32_t stepsInU, uint32_t stepsInV)
    : m_shape(shape), m_stepsInU(stepsInU), m_stepsInV(stepsInV) {}

uint32_t ParametricUVToMesh::getVertexCount() const {
  return (m_stepsInU + 1) * (m_stepsInV + 1);
}

uint32_t ParametricUVToMesh::getIndexCount() const {
  return 3 * 2 * m_stepsInU * m_stepsInV;
}

void ParametricUVToMesh::copyVertices(void *to, uint32_t stride) const {
  for (int32_t v = 0; v <= m_stepsInV; ++v) {
    for (int32_t u = 0; u <= m_stepsInU; ++u) {
      *(reinterpret_cast<Vector3 *>(to)) = m_shape->getVertexPosition(
          {(float)u / m_stepsInU, (float)v / m_stepsInV});
      to =
          reinterpret_cast<Vector3 *>(reinterpret_cast<uint8_t *>(to) + stride);
    }
  }
}

void ParametricUVToMesh::copyNormals(void *to, uint32_t stride) const {
  for (int32_t v = 0; v <= m_stepsInV; ++v) {
    for (int32_t u = 0; u <= m_stepsInU; ++u) {
      *(reinterpret_cast<Vector3 *>(to)) = m_shape->getVertexNormal(
          {(float)u / m_stepsInU, (float)v / m_stepsInV});
      to =
          reinterpret_cast<Vector3 *>(reinterpret_cast<uint8_t *>(to) + stride);
    }
  }
}

void ParametricUVToMesh::copyIndices(void *to, uint32_t stride) const {
  for (int32_t v = 0; v < m_stepsInV; ++v) {
    for (int32_t u = 0; u < m_stepsInU; ++u) {
      *(reinterpret_cast<uint32_t *>(to)) =
          (u + 0) + (v + 0) * (m_stepsInU + 1);
      to = reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(to) +
                                        stride);
      *(reinterpret_cast<uint32_t *>(to)) =
          (u + 0) + (v + 1) * (m_stepsInU + 1);
      to = reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(to) +
                                        stride);
      *(reinterpret_cast<uint32_t *>(to)) =
          (u + 1) + (v + 1) * (m_stepsInU + 1);
      to = reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(to) +
                                        stride);
      *(reinterpret_cast<uint32_t *>(to)) =
          (u + 0) + (v + 0) * (m_stepsInU + 1);
      to = reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(to) +
                                        stride);
      *(reinterpret_cast<uint32_t *>(to)) =
          (u + 1) + (v + 1) * (m_stepsInU + 1);
      to = reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(to) +
                                        stride);
      *(reinterpret_cast<uint32_t *>(to)) =
          (u + 1) + (v + 0) * (m_stepsInU + 1);
      to = reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(to) +
                                        stride);
    }
  }
}