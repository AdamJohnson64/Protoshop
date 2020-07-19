#pragma once

#include "Core_Math.h"
#include "Scene_Mesh.h"
#include <memory>
#include <stdint.h>
#include <vector>

class Instance
{
public:
    Matrix44 Transform;
    uint32_t GeometryIndex;
};

class InstanceTable
{
public:
    static Matrix44 CameraViewProjection();
    static std::shared_ptr<InstanceTable> Default();
    //uint32_t addMaterial(std::shared_ptr<Material> material);
    uint32_t addMesh(std::shared_ptr<Mesh> mesh);
    uint32_t addInstance(const Matrix44& transform, uint32_t geometryIndex);
    std::vector<std::shared_ptr<Mesh>> Meshes;
    std::vector<Instance> Instances;
};