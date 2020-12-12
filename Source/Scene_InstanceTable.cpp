#include "Scene_InstanceTable.h"
#include "Scene_Material.h"
#include "Scene_MeshOBJ.h"
#include "Scene_ParametricUV.h"
#include "Scene_ParametricUVToMesh.h"
#include "Scene_Plane.h"
#include "Scene_Sphere.h"

SceneCollector::SceneCollector(const std::vector<Instance> &scene) {
  std::map<const Mesh *, uint32_t> MeshToIndex;
  std::map<const Material *, uint32_t> MaterialToIndex;
  for (int instanceIndex = 0; instanceIndex < scene.size(); ++instanceIndex) {
    // Convert the pointer-based instance (DOM) into a tabular form.
    // Rewrite pointers as indices into other tables.
    InstanceFlat rewrite = {};
    rewrite.TransformObjectToWorld =
        scene[instanceIndex].TransformObjectToWorld;
    // Look up the mesh in our map; if we don't have it then allocate a new
    // index and append.
    {
      if (MeshToIndex.find(scene[instanceIndex].Mesh.get()) ==
          MeshToIndex.end()) {
        MeshToIndex[scene[instanceIndex].Mesh.get()] = rewrite.MeshID =
            MeshTable.size();
        MeshTable.push_back(scene[instanceIndex].Mesh.get());
      } else {
        rewrite.MeshID = MeshToIndex[scene[instanceIndex].Mesh.get()];
      }
    }
    // Look up the material in our map; if we don't have it then allocate a new
    // index and append.
    {
      if (MaterialToIndex.find(scene[instanceIndex].Material.get()) ==
          MaterialToIndex.end()) {
        MaterialToIndex[scene[instanceIndex].Material.get()] =
            rewrite.MaterialID = MaterialTable.size();
        MaterialTable.push_back(scene[instanceIndex].Material.get());
      } else {
        rewrite.MaterialID =
            MaterialToIndex[scene[instanceIndex].Material.get()];
      }
    }
    // Add this complete instance to our instance table.
    InstanceTable.push_back(rewrite);
  }
}

const std::vector<Instance> &Scene_Default() {
  static std::vector<Instance> scene;
  static bool initialized = false;
  if (!initialized) {
    // Create Materials.
    std::shared_ptr<Material> _checkerboard(new Checkerboard());
    std::shared_ptr<Material> _plastic(new RedPlastic());
    // Create Geometry.
    std::shared_ptr<ParametricUV> _plane(new Plane());
    std::shared_ptr<Mesh> _mesh(new ParametricUVToMesh(_plane, 1, 1));
    std::shared_ptr<ParametricUV> _sphere(new Sphere());
    std::shared_ptr<Mesh> _mesh2(new ParametricUVToMesh(_sphere, 100, 100));
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