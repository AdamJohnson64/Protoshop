#pragma once

#include "Core_Math.h"
#include "Core_Object.h"
#include "Scene_Mesh.h"

#include <string>

////////////////////////////////////////////////////////////////////////////////
// NOTE: This is an INCREDIBLY limited PLY loader.
// There's just enough here to read the Stanford Bunny, but that's about it.
////////////////////////////////////////////////////////////////////////////////
class MeshPLY : public Object, public Mesh {
public:
  MeshPLY(const std::string &filename);
  uint32_t getVertexCount() override;
  uint32_t getIndexCount() override;
  void copyVertices(void *to, uint32_t stride) override;
  void copyIndices(void *to, uint32_t stride) override;

private:
  int m_vertexCount;
  int m_faceCount;
  std::unique_ptr<TVector3<float>[]> m_vertices;
  std::unique_ptr<TVector3<int>[]> m_faces;
};