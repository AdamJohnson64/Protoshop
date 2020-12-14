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
  std::shared_ptr<Matrix44> TransformObjectToWorld;
  std::shared_ptr<IMesh> Mesh;
  std::shared_ptr<IMaterial> Material;
};

const std::vector<Instance> &Scene_Default();

const std::vector<Instance> &Scene_Sponza();