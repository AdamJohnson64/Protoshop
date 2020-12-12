#pragma once

class IMaterial;
class IMesh;

#include "Core_Math.h"
#include <map>
#include <memory>
#include <stdint.h>
#include <vector>

class Instance {
public:
  Matrix44 TransformObjectToWorld;
  std::shared_ptr<IMesh> Mesh;
  std::shared_ptr<IMaterial> Material;
};

typedef std::vector<Instance> InstanceTable;

struct InstanceFlat {
  Matrix44 TransformObjectToWorld;
  uint32_t MeshID;
  uint32_t MaterialID;
};

class SceneCollector {
public:
  SceneCollector(const std::vector<Instance> &scene);
  std::vector<InstanceFlat> InstanceTable;
  std::vector<const IMesh *> MeshTable;
  std::vector<const IMaterial *> MaterialTable;
};

const std::vector<Instance> &Scene_Default();

const std::vector<Instance> &Scene_Sponza();