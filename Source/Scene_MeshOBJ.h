#pragma once

#include "Core_Math.h"
#include "Core_Object.h"
#include "Scene_Mesh.h"
#include <memory>

////////////////////////////////////////////////////////////////////////////////
// NOTE: This is an INCREDIBLY limited OBJ loader.
// There's just enough here to read Sponza but that's about it.
////////////////////////////////////////////////////////////////////////////////
class MeshOBJ : public Object, public Mesh {
public:
  MeshOBJ(const char *filename);
  uint32_t getVertexCount() override;
  uint32_t getIndexCount() override;
  void copyVertices(void *to, uint32_t stride) override;
  void copyNormals(void *to, uint32_t stride) override;
  void copyIndices(void *to, uint32_t stride) override;

private:
  int m_vertexCount;
  int m_faceCount;
  std::unique_ptr<TVector3<float>[]> m_vertices;
  std::unique_ptr<TVector3<float>[]> m_normals;
  std::unique_ptr<TVector3<int>[]> m_faces;
};