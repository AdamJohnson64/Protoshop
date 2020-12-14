#include "Scene_InstanceTable.h"
#include "Scene_IMaterial.h"
#include "Scene_MeshOBJ.h"
#include "Scene_ParametricUVToMesh.h"
#include "Scene_Plane.h"
#include "Scene_Sphere.h"

const std::vector<Instance> &Scene_Default() {
  static std::vector<Instance> scene;
  static bool initialized = false;
  if (!initialized) {
    // Create Materials.
    std::shared_ptr<IMaterial> _checkerboard(new Checkerboard());
    std::shared_ptr<IMaterial> _plastic(new RedPlastic());
    // Create Geometry.
    std::shared_ptr<IParametricUV> _plane(new Plane());
    std::shared_ptr<IMesh> _mesh(new ParametricUVToMesh(_plane, 1, 1));
    std::shared_ptr<IParametricUV> _sphere(new Sphere());
    std::shared_ptr<IMesh> _mesh2(new ParametricUVToMesh(_sphere, 100, 100));
    // Create Instances.
    {
      Instance instance = {};
      instance.TransformObjectToWorld =
          CreateMatrixScale(Vector3{10, 0.25f, 10});
      instance.Mesh = _mesh;
      instance.Material = _checkerboard;
      scene.push_back(instance);
    }
    {
      Instance instance = {};
      instance.TransformObjectToWorld =
          CreateMatrixTranslate(Vector3{-2, 1, 0});
      instance.Mesh = _mesh2;
      instance.Material = _plastic;
      scene.push_back(instance);
    }
    {
      Instance instance = {};
      instance.TransformObjectToWorld = CreateMatrixTranslate(Vector3{0, 1, 0});
      instance.Mesh = _mesh2;
      instance.Material = _plastic;
      scene.push_back(instance);
    }
    {
      Instance instance = {};
      instance.TransformObjectToWorld = CreateMatrixTranslate(Vector3{2, 1, 0});
      instance.Mesh = _mesh2;
      instance.Material = _plastic;
      scene.push_back(instance);
    }
    initialized = true;
  }
  return scene;
}

const std::vector<Instance> &Scene_Sponza() {
  static std::vector<Instance> scene;
  static bool initialized = false;
  if (!initialized) {
    scene = LoadOBJ("Submodules\\RenderToyAssets\\Models\\Sponza\\sponza.obj");
    initialized = true;
  }
  return scene;
}