#include "Scene_MeshPLY.h"
#include <fstream>

static void consumelineandexpect(std::ifstream &stream,
                                 const std::string &expect) {
  std::string line;
  std::getline(stream, line);
  if (line != expect)
    throw std::exception(std::string("Expected '" + expect + "'.").c_str());
}

MeshPLY::MeshPLY(const std::string &filename) {
  std::ifstream readit(filename);
  std::string line;
  consumelineandexpect(readit, "ply");
  consumelineandexpect(readit, "format ascii 1.0");
  std::getline(readit, line);
  if (line.substr(0, 7) != "comment")
    throw std::string("Expected 'comment ...'.");
  std::getline(readit, line);
  // Get vertex count.
  if (line.substr(0, 15) != "element vertex ")
    throw std::exception("Expected 'element vertex <nnn>'.");
  m_vertexCount = std::stoi(line.substr(15));
  // Read vertex component schema.
  int componentcount = 0;
  int componentindexofx = -1;
  int componentindexofy = -1;
  int componentindexofz = -1;
ANOTHERPROPERTY:
  std::getline(readit, line);
  if (line.substr(0, 8) == "property") {
    if (line.substr(9, 5) != "float")
      throw std::string("Expected 'float' vertex data.");
    std::string component = line.substr(15);
    if (component == "x") {
      componentindexofx = componentcount;
    }
    if (component == "y") {
      componentindexofy = componentcount;
    }
    if (component == "z") {
      componentindexofz = componentcount;
    }
    ++componentcount;
    goto ANOTHERPROPERTY;
  }
  if (componentindexofx == -1)
    throw std::exception("Expected an 'x' vertex component.");
  if (componentindexofy == -1)
    throw std::exception("Expected an 'x' vertex component.");
  if (componentindexofz == -1)
    throw std::exception("Expected an 'x' vertex component.");
  // Get face count.
  if (line.substr(0, 13) != "element face ")
    throw std::exception("Expected 'element face <nnn>'.");
  m_faceCount = std::stoi(line.substr(13));
  consumelineandexpect(readit, "property list uchar int vertex_indices");
  consumelineandexpect(readit, "end_header");
  m_vertices.reset(new TVector3<float>[m_vertexCount]);
  for (int i = 0; i < m_vertexCount; ++i) {
    float x, y, z;
    for (int c = 0; c < componentcount; ++c) {
      if (c != componentcount - 1) {
        std::getline(readit, line, ' ');
      } else {
        std::getline(readit, line);
      }
      if (c == componentindexofx)
        x = std::stof(line);
      if (c == componentindexofy)
        y = std::stof(line);
      if (c == componentindexofz)
        z = std::stof(line);
    }
    m_vertices[i] = {x, y, z};
  }
  m_faces.reset(new TVector3<int>[m_faceCount]);
  for (int i = 0; i < m_faceCount; ++i) {
    std::getline(readit, line, ' ');
    int indexcount = std::stoi(line);
    if (indexcount != 3)
      throw std::exception("Input must be triangles only.");
    std::getline(readit, line, ' ');
    int a = std::stoi(line);
    std::getline(readit, line, ' ');
    int b = std::stoi(line);
    std::getline(readit, line);
    int c = std::stoi(line);
    m_faces[i] = {a, b, c};
  }
}

uint32_t MeshPLY::getVertexCount() { return m_vertexCount; }

uint32_t MeshPLY::getIndexCount() { return 3 * m_faceCount; }

void MeshPLY::copyVertices(void *to, uint32_t stride) {
  void *begin = to;
  for (int i = 0; i < m_vertexCount; ++i) {
    *reinterpret_cast<TVector3<float> *>(to) = m_vertices[i];
    to = reinterpret_cast<uint8_t *>(to) + stride;
  }
  int test = 0;
}

void MeshPLY::copyIndices(void *to, uint32_t stride) {
  uint32_t *from = reinterpret_cast<uint32_t *>(&m_faces[0]);
  for (int i = 0; i < m_faceCount * 3; ++i) {
    *reinterpret_cast<uint32_t *>(to) = *from;
    ++from;
    to = reinterpret_cast<uint8_t *>(to) + stride;
  }
}