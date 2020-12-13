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
    std::shared_ptr<Textured> currentMaterialDefinition;
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
        currentMaterialDefinition.reset(new Textured());
      } else if (line.substr(0, 8) == "\tmap_Ka ") {
        if (currentMaterialName == "")
          throw std::exception("No material name specified.");
        std::string textureFilename = line.substr(8);
        if (mapPathToTexture.find(textureFilename) == mapPathToTexture.end()) {
          mapPathToTexture[textureFilename].reset(new TextureImage());
          mapPathToTexture[textureFilename]->Filename =
              pathPrefix + textureFilename;
        }
        currentMaterialDefinition->AlbedoMap =
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
  std::vector<Vector3> vertices, normals;
  std::vector<Vector2> uvs;
  std::vector<uint32_t> facesVertex, facesNormal, facesUV;
  // Last detected material.
  std::shared_ptr<IMaterial> currentMaterial;
  ////////////////////////////////////////////////////////////////////////////////
  // Flush the accumulated geometry and materials to a new instance.
  std::function<void()> FLUSHMESH = [&]() {
    if (currentMaterial == nullptr)
      return;
    Instance instance = {};
    instance.TransformObjectToWorld = CreateMatrixScale(Vector3{1, 1, 1});
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
      mesh->m_vertices[mesh->m_indices[f]] = vertices[facesVertex[f]] * 0.01f;
      mesh->m_normals[mesh->m_indices[f]] = normals[facesNormal[f]];
      mesh->m_texcoords[mesh->m_indices[f]] = uvs[facesUV[f]];
    }
    instance.Mesh = mesh;
    instance.Material = currentMaterial;
    instances.push_back(instance);
    facesVertex.clear();
    facesNormal.clear();
    facesUV.clear();
    currentMaterial.reset();
  };
  ////////////////////////////////////////////////////////////////////////////////
  // Parse the model definition.
  std::ifstream model(filename);
  std::string line;
  std::map<std::string, std::shared_ptr<IMaterial>> materials;
  while (std::getline(model, line)) {
    if (false) {
    } else if (line.size() == 0 || line[0] == '#') {
      // Ignore comment.
    } else if (line.substr(0, 7) == "mtllib ") {
      // Filename of the material definition file (mtllib)
      std::string materialName = line.substr(7);
      materials = LoadMTL(materialName.c_str());
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
      uvs.push_back({x, y});
    } else if (line.substr(0, 2) == "g ") {
      // Groups (g?)
      std::string groupName = line.substr(2);
    } else if (line.substr(0, 7) == "usemtl ") {
      FLUSHMESH();
      std::string materialName = line.substr(7);
      if (materials.size() > 0 &&
          materials.find(materialName) == materials.end()) {
        throw std::exception("Requested material is undefined.");
      }
      currentMaterial = materials[materialName];
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
        facesVertex.push_back(v0);
        facesVertex.push_back(v1);
        facesVertex.push_back(v2);
        facesNormal.push_back(vn0);
        facesNormal.push_back(vn1);
        facesNormal.push_back(vn2);
        facesUV.push_back(vt0);
        facesUV.push_back(vt1);
        facesUV.push_back(vt2);
      }
    } else {
      throw std::exception(("Unreadable OBJ file at '" + line + ".").c_str());
    }
  }
  FLUSHMESH();
  return instances;
}