#include "Scene_InstanceTable.h"
#include "Scene_ParametricUV.h"
#include "Scene_ParametricUVToMesh.h"
#include "Scene_Plane.h"
#include "Scene_Sphere.h"

std::shared_ptr<InstanceTable> InstanceTable::Default()
{
    std::shared_ptr<InstanceTable> scene(new InstanceTable());
    // Create Materials.
    std::shared_ptr<Material> _checkerboard(new Checkerboard());
    uint32_t hCheckerboard = scene->addMaterial(_checkerboard);
    std::shared_ptr<Material> _plastic(new RedPlastic());
    uint32_t hPlastic = scene->addMaterial(_plastic);
    // Create Geometry.
    std::shared_ptr<ParametricUV> _plane(new Plane());
    std::shared_ptr<Mesh> _mesh(new ParametricUVToMesh(_plane, 100, 100));
    uint32_t hPlane = scene->addGeometry(_mesh);
    std::shared_ptr<ParametricUV> _sphere(new Sphere());
    std::shared_ptr<Mesh> _mesh2(new ParametricUVToMesh(_sphere, 100, 100));
    // Create Instances.
    Matrix44 transform = {};
    transform.M11 = 10;
    transform.M22 = 0.25f;
    transform.M33 = 10;
    transform.M44 = 1;
    scene->addInstance(transform, hPlane, hCheckerboard);
    uint32_t hSphere = scene->addGeometry(_mesh2);
    transform.M11 = 1;
    transform.M22 = 1;
    transform.M33 = 1;
    transform.M44 = 1;
    transform.M42 = 1;
    transform.M41 = -2;
    scene->addInstance(transform, hSphere, hPlastic);
    transform.M41 = 0;
    scene->addInstance(transform, hSphere, hPlastic);
    transform.M41 = 2;
    scene->addInstance(transform, hSphere, hPlastic);
    return scene;
}

uint32_t InstanceTable::addMaterial(std::shared_ptr<Material> material)
{
    uint32_t index = Materials.size();
    Materials.push_back(material);
    return index;
}

uint32_t InstanceTable::addGeometry(std::shared_ptr<Geometry> geometry)
{
    uint32_t index = Meshes.size();
    Meshes.push_back(geometry);
    return index;
}

uint32_t InstanceTable::addInstance(const Matrix44& transform, uint32_t geometryIndex, uint32_t materialIndex)
{
    uint32_t index = Instances.size();
    Instances.push_back({ transform, geometryIndex, materialIndex });
    return index;
}