#include "Scene_MeshOBJ.h"
#include "Core_Math.h"
#include <fstream>
#include <regex>
#include <string>
#include <string_view>

std::vector<std::string_view> split(const std::string_view &input,
                                    char delim = ' ') {
  std::vector<std::string_view> o;
  int start = 0;
  while (true) {
    int end = input.find_first_of(delim, start);
    if (end == -1) {
      if (start < input.size()) {
        o.push_back(input.substr(start));
      }
      break;
    } else if (end != start) {
      o.push_back(input.substr(start, end - start));
    }
    start = end + 1;
  }
  return o;
}

MeshOBJ::MeshOBJ(const char *filename) {
  std::vector<Vector3> vertices, normals, uvs;
  std::vector<TVector3<int>> facesVertex, facesNormal, facesUV;
  std::ifstream model(filename);
  std::string line;
  while (std::getline(model, line)) {
    if (false) {
    } else if (line.size() == 0 || line[0] == '#') {
      // Ignore comment.
    } else if (line.substr(0, 7) == "mtllib ") {
      // Filename of the material definition file (mtllib)
      std::string materialName = line.substr(7);
    } else if (line.substr(0, 2) == "v ") {
      // Vertex Position (v)
      auto bits = split(line);
      if (bits.size() != 4) {
        throw std::exception("Wrong number of components in vertex position.");
      }
      float x = std::stof(std::string(bits[1]));
      float y = std::stof(std::string(bits[2]));
      float z = std::stof(std::string(bits[3]));
      vertices.push_back({x, y, z});
    } else if (line.substr(0, 3) == "vn ") {
      // Vertex Normals (vn)
      auto bits = split(line);
      if (bits.size() != 4) {
        throw std::exception("Wrong number of components in vertex normal.");
      }
      float x = std::stof(std::string(bits[1]));
      float y = std::stof(std::string(bits[2]));
      float z = std::stof(std::string(bits[3]));
      normals.push_back({x, y, z});
    } else if (line.substr(0, 3) == "vt ") {
      // Vertex Texture Coordinates (vt)
      auto bits = split(line);
      if (bits.size() != 4) {
        throw std::exception("Wrong number of components in vertex UV.");
      }
      float x = std::stof(std::string(bits[1]));
      float y = std::stof(std::string(bits[2]));
      float z = std::stof(std::string(bits[3]));
      uvs.push_back({x, y, z});
    } else if (line.substr(0, 2) == "g ") {
      // Groups (g?)
      std::string groupName = line.substr(2);
    } else if (line.substr(0, 7) == "usemtl ") {
      std::string materialName = line.substr(7);
    } else if (line.substr(0, 2) == "s ") {

    } else if (line.substr(0, 2) == "f ") {
      auto bits = split(line);
      if (bits.size() < 4) {
        throw std::exception("Insufficient components in face.");
      }
      for (int poly = 0; poly < bits.size() - 3; ++poly) {
        auto facebits1 = split(bits[1], '/');
        if (facebits1.size() != 3) {
          throw std::exception("Wrong number of indices in face.");
        }
        auto facebits2 = split(bits[2 + poly], '/');
        if (facebits2.size() != 3) {
          throw std::exception("Wrong number of indices in face.");
        }
        auto facebits3 = split(bits[3 + poly], '/');
        if (facebits3.size() != 3) {
          throw std::exception("Wrong number of indices in face.");
        }
        int v0 = std::stoi(std::string(facebits1[0])) - 1;
        int v1 = std::stoi(std::string(facebits2[0])) - 1;
        int v2 = std::stoi(std::string(facebits3[0])) - 1;
        int vt0 = std::stoi(std::string(facebits1[1])) - 1;
        int vt1 = std::stoi(std::string(facebits2[1])) - 1;
        int vt2 = std::stoi(std::string(facebits3[1])) - 1;
        int vn0 = std::stoi(std::string(facebits1[2])) - 1;
        int vn1 = std::stoi(std::string(facebits2[2])) - 1;
        int vn2 = std::stoi(std::string(facebits3[2])) - 1;
        if (v0 >= vertices.size() || v1 >= vertices.size() ||
            v2 >= vertices.size()) {
          throw std::exception("Vertex position index out of range.");
        }
        if (vn0 >= normals.size() || vn1 >= normals.size() ||
            vn2 >= normals.size()) {
          throw std::exception("Vertex normal index out of range.");
        }
        if (vt0 >= uvs.size() || vt1 >= uvs.size() || vt2 >= uvs.size()) {
          throw std::exception("Vertex UV index out of range.");
        }
        facesVertex.push_back({v0, v1, v2});
        facesNormal.push_back({vn0, vn1, vn2});
        facesUV.push_back({vt0, vt1, vt2});
      }
    } else {
      throw std::exception(("Unreadable OBJ file at '" + line + ".").c_str());
    }
  }
  m_faceCount = facesVertex.size();
  m_vertexCount = 3 * m_faceCount;
  // Now we're going to be horribly inefficient and fracture all the face
  // vertices.
  m_vertices.reset(new Vector3[m_vertexCount]);
  m_normals.reset(new Vector3[m_vertexCount]);
  m_faces.reset(new TVector3<int>[m_faceCount]);
  for (int f = 0; f < m_faceCount; ++f) {
    m_faces[f] = {3 * f + 0, 3 * f + 1, 3 * f + 2};
    m_vertices[m_faces[f].X] = vertices[facesVertex[f].X] * 0.01f;
    m_vertices[m_faces[f].Y] = vertices[facesVertex[f].Y] * 0.01f;
    m_vertices[m_faces[f].Z] = vertices[facesVertex[f].Z] * 0.01f;
    m_normals[m_faces[f].X] = normals[facesNormal[f].X];
    m_normals[m_faces[f].Y] = normals[facesNormal[f].Y];
    m_normals[m_faces[f].Z] = normals[facesNormal[f].Z];
  }
  int test = 0;
}

uint32_t MeshOBJ::getVertexCount() { return m_vertexCount; }

uint32_t MeshOBJ::getIndexCount() { return 3 * m_faceCount; }

void MeshOBJ::copyVertices(void *to, uint32_t stride) {
  void *begin = to;
  for (int i = 0; i < m_vertexCount; ++i) {
    *reinterpret_cast<TVector3<float> *>(to) = m_vertices[i];
    to = reinterpret_cast<uint8_t *>(to) + stride;
  }
  int test = 0;
}

void MeshOBJ::copyNormals(void *to, uint32_t stride) {
  void *begin = to;
  for (int i = 0; i < m_vertexCount; ++i) {
    *reinterpret_cast<TVector3<float> *>(to) = m_normals[i];
    to = reinterpret_cast<uint8_t *>(to) + stride;
  }
  int test = 0;
}

void MeshOBJ::copyIndices(void *to, uint32_t stride) {
  uint32_t *from = reinterpret_cast<uint32_t *>(&m_faces[0]);
  for (int i = 0; i < m_faceCount * 3; ++i) {
    *reinterpret_cast<uint32_t *>(to) = *from;
    ++from;
    to = reinterpret_cast<uint8_t *>(to) + stride;
  }
}