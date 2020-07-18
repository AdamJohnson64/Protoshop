#include "Scene_InstanceTable.h"
#include "Scene_ParametricUV.h"
#include "Scene_ParametricUVToMesh.h"
#include "Scene_Plane.h"
#include "Scene_Sphere.h"

std::shared_ptr<InstanceTable> InstanceTable::Default()
{
    std::shared_ptr<InstanceTable> scene(new InstanceTable());
    std::shared_ptr<ParametricUV> _shape(new Plane());
    std::shared_ptr<Mesh> _mesh(new ParametricUVToMesh(_shape, 100, 100));
    uint32_t meshHandle = scene->addMesh(_mesh);
    Matrix44 transform = {};
    transform.M11 = 10;
    transform.M22 = 0.25f;
    transform.M33 = 10;
    transform.M44 = 1;
    scene->addInstance(transform, meshHandle);
    std::shared_ptr<ParametricUV> _shape2(new Sphere());
    std::shared_ptr<Mesh> _mesh2(new ParametricUVToMesh(_shape2, 100, 100));
    meshHandle = scene->addMesh(_mesh2);
    transform.M11 = 1;
    transform.M22 = 1;
    transform.M33 = 1;
    transform.M44 = 1;
    scene->addInstance(transform, meshHandle);
    return scene;
}

uint32_t InstanceTable::addMesh(std::shared_ptr<Mesh> mesh)
{
    uint32_t index = Meshes.size();
    Meshes.push_back(mesh);
    return index;
}

uint32_t InstanceTable::addInstance(const Matrix44& transform, uint32_t geometryIndex)
{
    uint32_t index = Instances.size();
    Instances.push_back({ transform, geometryIndex });
    return index;
}