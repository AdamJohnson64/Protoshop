#include "Scene_InstanceTable.h"
#include "Scene_ParametricUV.h"
#include "Scene_ParametricUVToMesh.h"
#include "Scene_Plane.h"
#include "Scene_Sphere.h"

std::shared_ptr<InstanceTable> InstanceTable::Default()
{
    static std::shared_ptr<InstanceTable> scene(new InstanceTable());
    static bool initialized = false;
    if (!initialized)
    {
        // Create Materials.
        std::shared_ptr<Material> _checkerboard(new Checkerboard());
        uint32_t hCheckerboard = scene->addMaterial(_checkerboard);
        std::shared_ptr<Material> _plastic(new RedPlastic());
        uint32_t hPlastic = scene->addMaterial(_plastic);
        // Create Geometry.
        std::shared_ptr<ParametricUV> _plane(new Plane());
        std::shared_ptr<Mesh> _mesh(new ParametricUVToMesh(_plane, 100, 100));
        uint32_t hPlane = scene->addMesh(_mesh);
        std::shared_ptr<ParametricUV> _sphere(new Sphere());
        std::shared_ptr<Mesh> _mesh2(new ParametricUVToMesh(_sphere, 100, 100));
        uint32_t hSphere = scene->addMesh(_mesh2);
        // Create Instances.
        Matrix44 transformObjectToWorld = {};
        transformObjectToWorld.M11 = 10;
        transformObjectToWorld.M22 = 0.25f;
        transformObjectToWorld.M33 = 10;
        transformObjectToWorld.M44 = 1;
        scene->addInstance(transformObjectToWorld, hPlane, hCheckerboard);
        transformObjectToWorld.M11 = 1;
        transformObjectToWorld.M22 = 1;
        transformObjectToWorld.M33 = 1;
        transformObjectToWorld.M44 = 1;
        transformObjectToWorld.M42 = 1;
        transformObjectToWorld.M41 = -2;
        scene->addInstance(transformObjectToWorld, hSphere, hPlastic);
        transformObjectToWorld.M41 = 0;
        scene->addInstance(transformObjectToWorld, hSphere, hPlastic);
        transformObjectToWorld.M41 = 2;
        scene->addInstance(transformObjectToWorld, hSphere, hPlastic);
        initialized = true;
    }
    return scene;
}

uint32_t InstanceTable::addMaterial(std::shared_ptr<Material> material)
{
    uint32_t index = Materials.size();
    Materials.push_back(material);
    return index;
}

uint32_t InstanceTable::addMesh(std::shared_ptr<Mesh> mesh)
{
    uint32_t index = Meshes.size();
    Meshes.push_back(mesh);
    return index;
}

uint32_t InstanceTable::addInstance(const Matrix44& transformObjectToWorld, uint32_t geometryIndex, uint32_t materialIndex)
{
    uint32_t index = Instances.size();
    Instances.push_back({ transformObjectToWorld, geometryIndex, materialIndex });
    return index;
}