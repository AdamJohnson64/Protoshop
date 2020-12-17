#include "Scene_MeshOBJ.h"
#include "Core_Math.h"
#include "Scene_IMaterial.h"
#include "Scene_IMesh.h"
#include "Scene_InstanceTable.h"
#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

class MeshFromOBJ : public Object, public IMesh {
public:
  uint32_t getVertexCount() const override;
  uint32_t getIndexCount() const override;
  void copyVertices(void *to, uint32_t stride) const override;
  void copyNormals(void *to, uint32_t stride) const override;
  void copyTexcoords(void *to, uint32_t stride) const override;
  void copyTangents(void *to, uint32_t stride) const override;
  void copyBitangents(void *to, uint32_t stride) const override;
  void copyIndices(void *to, uint32_t stride) const override;
  int m_vertexCount;
  int m_indexCount;
  std::unique_ptr<TVector3<float>[]> m_vertices;
  std::unique_ptr<TVector3<float>[]> m_normals;
  std::unique_ptr<TVector2<float>[]> m_texcoords;
  std::unique_ptr<uint32_t[]> m_indices;
};

uint32_t MeshFromOBJ::getVertexCount() const { return m_vertexCount; }

uint32_t MeshFromOBJ::getIndexCount() const { return m_indexCount; }

void MeshFromOBJ::copyVertices(void *to, uint32_t stride) const {
  void *begin = to;
  for (int i = 0; i < m_vertexCount; ++i) {
    *reinterpret_cast<TVector3<float> *>(to) = m_vertices[i];
    to = reinterpret_cast<uint8_t *>(to) + stride;
  }
}

void MeshFromOBJ::copyNormals(void *to, uint32_t stride) const {
  void *begin = to;
  for (int i = 0; i < m_vertexCount; ++i) {
    *reinterpret_cast<TVector3<float> *>(to) = m_normals[i];
    to = reinterpret_cast<uint8_t *>(to) + stride;
  }
}

void MeshFromOBJ::copyTexcoords(void *to, uint32_t stride) const {
  void *begin = to;
  for (int i = 0; i < m_vertexCount; ++i) {
    *reinterpret_cast<TVector2<float> *>(to) = m_texcoords[i];
    to = reinterpret_cast<uint8_t *>(to) + stride;
  }
}

void MeshFromOBJ::copyTangents(void *to, uint32_t stride) const {
  // TODO: Tangents
}

void MeshFromOBJ::copyBitangents(void *to, uint32_t stride) const {
  // TODO: Bitangents
}

void MeshFromOBJ::copyIndices(void *to, uint32_t stride) const {
  void *begin = to;
  for (int i = 0; i < m_indexCount; ++i) {
    *reinterpret_cast<uint32_t *>(to) = m_indices[i];
    to = reinterpret_cast<uint8_t *>(to) + stride;
  }
}

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

std::map<std::string, std::shared_ptr<IMaterial>>
LoadMTL(const char *filename) {
  const std::string pathPrefix =
      "Submodules\\RenderToyAssets\\Models\\Sponza\\";
  std::map<std::string, std::shared_ptr<IMaterial>> mapNameToMaterial;
  std::map<std::string, std::shared_ptr<TextureImage>> mapPathToTexture;
  {
    ////////////////////////////////////////////////////////////////////////////////
    // Build up a material definition along with its name identity.
    std::string currentMaterialName;
    std::shared_ptr<OBJMaterial> currentMaterialDefinition;
    ////////////////////////////////////////////////////////////////////////////////
    // Take the currently accumulated material definition and emit a material.
    std::function<void()> FLUSHMATERIAL = [&]() {
      if (currentMaterialDefinition == nullptr)
        return;
      mapNameToMaterial[currentMaterialName] = currentMaterialDefinition;
      currentMaterialName = "";
      currentMaterialDefinition = nullptr;
    };
    ////////////////////////////////////////////////////////////////////////////////
    // Parse the material file.
    std::ifstream mtl(pathPrefix + filename);
    std::string line;
    while (std::getline(mtl, line)) {
      if (line.size() == 0 || line[0] == '#') {
      } else if (line.substr(0, 7) == "newmtl ") {
        // When we find a new material we must flush the old one before
        // restarting.
        FLUSHMATERIAL();
        currentMaterialName = line.substr(7);
        currentMaterialDefinition.reset(new OBJMaterial());
      } else if (line.substr(0, 8) == "\tmap_Kd ") {
        if (currentMaterialName == "")
          throw std::exception("No material name specified.");
        std::string textureFilename = line.substr(8);
        if (mapPathToTexture.find(textureFilename) == mapPathToTexture.end()) {
          mapPathToTexture[textureFilename].reset(new TextureImage());
          mapPathToTexture[textureFilename]->Filename =
              pathPrefix + textureFilename;
        }
        currentMaterialDefinition->DiffuseMap =
            mapPathToTexture[textureFilename];
      } else if (line.substr(0, 7) == "\tmap_d ") {
        if (currentMaterialName == "")
          throw std::exception("No material name specified.");
        std::string textureFilename = line.substr(7);
        if (mapPathToTexture.find(textureFilename) == mapPathToTexture.end()) {
          mapPathToTexture[textureFilename].reset(new TextureImage());
          mapPathToTexture[textureFilename]->Filename =
              pathPrefix + textureFilename;
        }
        currentMaterialDefinition->DissolveMap =
            mapPathToTexture[textureFilename];
      }
    }
    ////////////////////////////////////////////////////////////////////////////////
    // Whatever material remains should be flushed.
    FLUSHMATERIAL();
  }
  return mapNameToMaterial;
}

std::vector<Instance> LoadOBJ(const char *filename) {
  ////////////////////////////////////////////////////////////////////////////////
  // Accumulated instances so far.
  std::vector<Instance> instances;
  // Vertices and faces for the accumulated mesh so far.
  std::vector<Vector3> vertexPosition;
  std::vector<Vector3> vertexNormal;
  std::vector<Vector2> vertexTexcoord;
  std::vector<uint32_t> facesVertex;
  std::vector<uint32_t> facesNormal;
  std::vector<uint32_t> facesTexcoord;
  // Last detected material.
  std::shared_ptr<IMaterial> lastActiveMaterial;
  ////////////////////////////////////////////////////////////////////////////////
  // This is a bit cheeky. In order to share constant buffers we're packing
  // transforms into objects and sharing them with shared_ptr. This is nasty
  // but solves our problem of transform constant buffer identity.
  std::shared_ptr<Matrix44> transformIdentity(
      new Matrix44{CreateMatrixTranslate(Vector3{0, 0, 0})});
  ////////////////////////////////////////////////////////////////////////////////
  // Flush the accumulated geometry and materials to a new instance.
  std::function<void()> FLUSHMESH = [&]() {
    if (lastActiveMaterial == nullptr)
      return;
    Instance instance = {};
    instance.TransformObjectToWorld = transformIdentity;
    std::shared_ptr<MeshFromOBJ> mesh(new MeshFromOBJ());
    mesh->m_indexCount = facesVertex.size();
    mesh->m_vertexCount = facesVertex.size();
    // Now we're going to be horribly inefficient and fracture all the face
    // vertices.
    mesh->m_vertices.reset(new Vector3[mesh->m_vertexCount]);
    mesh->m_normals.reset(new Vector3[mesh->m_vertexCount]);
    mesh->m_texcoords.reset(new Vector2[mesh->m_vertexCount]);
    mesh->m_indices.reset(new uint32_t[mesh->m_indexCount]);
    for (int f = 0; f < mesh->m_indexCount; ++f) {
      mesh->m_indices[f] = f;
      mesh->m_vertices[mesh->m_indices[f]] =
          vertexPosition[facesVertex[f]] * 0.01f;
      mesh->m_normals[mesh->m_indices[f]] = vertexNormal[facesNormal[f]];
      mesh->m_texcoords[mesh->m_indices[f]] = vertexTexcoord[facesTexcoord[f]];
    }
    instance.Mesh = mesh;
    instance.Material = lastActiveMaterial;
    instances.push_back(instance);
    facesVertex.clear();
    facesNormal.clear();
    facesTexcoord.clear();
    lastActiveMaterial.reset();
  };
  ////////////////////////////////////////////////////////////////////////////////
  // Parse the model definition.
  std::ifstream fileOBJ(filename);
  std::string fileLine;
  std::map<std::string, std::shared_ptr<IMaterial>> mapNameToMaterial;
  while (std::getline(fileOBJ, fileLine)) {
    ////////////////////////////////////////////////////////////////////////////////
    // Ignore comments (#)
    if (fileLine.size() == 0 || fileLine[0] == '#') {

      ////////////////////////////////////////////////////////////////////////////////
      // Filename of the material definition file (mtllib)
    } else if (fileLine.substr(0, 7) == "mtllib ") {
      std::string materialName = fileLine.substr(7);
      mapNameToMaterial = LoadMTL(materialName.c_str());

      ////////////////////////////////////////////////////////////////////////////////
      // Vertex Position (v)
    } else if (fileLine.substr(0, 2) == "v ") {
      auto bits = split(fileLine);
      if (bits.size() != 4) {
        throw std::exception("Wrong number of components in vertex position.");
      }
      float x = std::stof(std::string(bits[1]));
      float y = std::stof(std::string(bits[2]));
      float z = std::stof(std::string(bits[3]));
      vertexPosition.push_back({x, y, z});

      ////////////////////////////////////////////////////////////////////////////////
      // Vertex Normals (vn)
    } else if (fileLine.substr(0, 3) == "vn ") {
      auto bits = split(fileLine);
      if (bits.size() != 4) {
        throw std::exception("Wrong number of components in vertex normal.");
      }
      float x = std::stof(std::string(bits[1]));
      float y = std::stof(std::string(bits[2]));
      float z = std::stof(std::string(bits[3]));
      vertexNormal.push_back({x, y, z});

      ////////////////////////////////////////////////////////////////////////////////
      // Vertex Texture Coordinates (vt)
    } else if (fileLine.substr(0, 3) == "vt ") {
      auto bits = split(fileLine);
      if (bits.size() != 4) {
        throw std::exception("Wrong number of components in vertex UV.");
      }
      float x = std::stof(std::string(bits[1]));
      float y = std::stof(std::string(bits[2]));
      float z = std::stof(std::string(bits[3]));
      vertexTexcoord.push_back(
          {x, 1 - y}); // Note: Correction for OpenGL flipped V.

      ////////////////////////////////////////////////////////////////////////////////
      // Groups (g?)
    } else if (fileLine.substr(0, 2) == "g ") {
      std::string groupName = fileLine.substr(2);

      ////////////////////////////////////////////////////////////////////////////////
      // Material selection (usemtl)
    } else if (fileLine.substr(0, 7) == "usemtl ") {
      FLUSHMESH();
      std::string materialName = fileLine.substr(7);
      if (mapNameToMaterial.size() > 0 &&
          mapNameToMaterial.find(materialName) == mapNameToMaterial.end()) {
        throw std::exception("Requested material is undefined.");
      }
      lastActiveMaterial = mapNameToMaterial[materialName];

    } else if (fileLine.substr(0, 2) == "s ") {

      ////////////////////////////////////////////////////////////////////////////////
      // Faces (f)
    } else if (fileLine.substr(0, 2) == "f ") {
      auto bits = split(fileLine);
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
        // Extract the indices (pos/uv/nor pos/uv/nor pos/uv/nor)
        int v0 = std::stoi(std::string(facebits1[0])) - 1;
        int v1 = std::stoi(std::string(facebits2[0])) - 1;
        int v2 = std::stoi(std::string(facebits3[0])) - 1;
        int vt0 = std::stoi(std::string(facebits1[1])) - 1;
        int vt1 = std::stoi(std::string(facebits2[1])) - 1;
        int vt2 = std::stoi(std::string(facebits3[1])) - 1;
        int vn0 = std::stoi(std::string(facebits1[2])) - 1;
        int vn1 = std::stoi(std::string(facebits2[2])) - 1;
        int vn2 = std::stoi(std::string(facebits3[2])) - 1;
        if (v0 >= vertexPosition.size() || v1 >= vertexPosition.size() ||
            v2 >= vertexPosition.size()) {
          throw std::exception("Vertex position index out of range.");
        }
        if (vn0 >= vertexNormal.size() || vn1 >= vertexNormal.size() ||
            vn2 >= vertexNormal.size()) {
          throw std::exception("Vertex normal index out of range.");
        }
        if (vt0 >= vertexTexcoord.size() || vt1 >= vertexTexcoord.size() ||
            vt2 >= vertexTexcoord.size()) {
          throw std::exception("Vertex UV index out of range.");
        }
        facesVertex.push_back(v0);
        facesVertex.push_back(v1);
        facesVertex.push_back(v2);
        facesNormal.push_back(vn0);
        facesNormal.push_back(vn1);
        facesNormal.push_back(vn2);
        facesTexcoord.push_back(vt0);
        facesTexcoord.push_back(vt1);
        facesTexcoord.push_back(vt2);
      }

    } else {
      throw std::exception(
          ("Unreadable OBJ file at '" + fileLine + ".").c_str());
    }
  }
  FLUSHMESH();
  return instances;
}