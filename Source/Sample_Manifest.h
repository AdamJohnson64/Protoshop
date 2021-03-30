#pragma once

#include "Core_D3D11.h"
#include "Core_Math.h"
#include "SampleResources.h"
#include "Scene_InstanceTable.h"
#include <d3d11.h>
#include <functional>
#include <memory>

class DXGISwapChain;
class Direct3D11Device;
class Direct3D12Device;
class OpenGLDevice;
class VKDevice;

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11Basic(std::shared_ptr<Direct3D11Device> device);

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11ComputeCanvas(std::shared_ptr<Direct3D11Device> device);

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11DrawingContext(std::shared_ptr<Direct3D11Device> device);

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11DXGICapture(std::shared_ptr<Direct3D11Device> device);

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11Font(std::shared_ptr<Direct3D11Device> device);

void CreateSample_D3D11Keyboard(SampleRequestD3D11 &request);

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11LightProbe(std::shared_ptr<Direct3D11Device> device);

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11LightProbeCross(std::shared_ptr<Direct3D11Device> device);

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11MarchingTetrahedra(std::shared_ptr<Direct3D11Device> device);

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11Mesh(std::shared_ptr<Direct3D11Device> device);

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11NormalMap(std::shared_ptr<Direct3D11Device> device);

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11ParallaxMap(std::shared_ptr<Direct3D11Device> device);

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11RayMarch(std::shared_ptr<Direct3D11Device> device);

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11Scene(std::shared_ptr<Direct3D11Device> device,
                        const std::vector<Instance> &scene);

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11ShadowMap(std::shared_ptr<Direct3D11Device> device);

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11Tessellation(std::shared_ptr<Direct3D11Device> device);

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11Voxel(std::shared_ptr<Direct3D11Device> device);

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11VoxelPlanet(std::shared_ptr<Direct3D11Device> device);

std::function<void(const SampleResourcesD3D12RTV &)>
CreateSample_D3D12Basic(std::shared_ptr<Direct3D12Device> device);

std::function<void(const SampleResourcesD3D12RTV &)>
CreateSample_D3D12Scene(std::shared_ptr<Direct3D12Device> device,
                        const std::vector<Instance> &scene);

std::function<void(const SampleResourcesD3D12UAV &)>
CreateSample_DXRAmbientOcclusion(std::shared_ptr<Direct3D12Device> device);

std::function<void(const SampleResourcesD3D12UAV &)>
CreateSample_DXRBasic(std::shared_ptr<Direct3D12Device> device);

std::function<void(const SampleResourcesD3D12UAV &)>
CreateSample_DXRMesh(std::shared_ptr<Direct3D12Device> device);

std::function<void(const SampleResourcesD3D12UAV &)>
CreateSample_DXRPathTrace(std::shared_ptr<Direct3D12Device> device);

std::function<void(const SampleResourcesD3D12UAV &)>
CreateSample_DXRScene(std::shared_ptr<Direct3D12Device> device,
                      const std::vector<Instance> &scene);

std::function<void(const SampleResourcesD3D12UAV &)>
CreateSample_DXRTexture(std::shared_ptr<Direct3D12Device> device);

std::function<void(const SampleResourcesD3D12UAV &)>
CreateSample_DXRWhitted(std::shared_ptr<Direct3D12Device> device);

std::unique_ptr<IImage> CreateSample_Image_FreeTypeAtlas();

std::function<void()>
CreateSample_OpenGLBasic(std::shared_ptr<OpenGLDevice> device);

#if VULKAN_INSTALLED
std::function<void(VKDevice *, vk::Image)> CreateSample_VKBasic();
#endif // VULKAN_INSTALLED