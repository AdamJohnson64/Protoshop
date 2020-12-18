#pragma once

#include "Core_D3D11.h"
#include "Core_Math.h"
#include "Scene_InstanceTable.h"
#include <d3d11.h>
#include <functional>

class DXGISwapChain;
class Direct3D11Device;
class Direct3D12Device;
class OpenGLDevice;
class VKDevice;

#include <memory>

std::function<void(ID3D11Texture2D *)>
CreateSample_D3D11Basic(std::shared_ptr<Direct3D11Device> device);

std::function<void(ID3D11Texture2D *)>
CreateSample_D3D11ComputeCanvas(std::shared_ptr<Direct3D11Device> device);

std::function<void(ID3D11Texture2D *)>
CreateSample_D3D11DrawingContext(std::shared_ptr<Direct3D11Device> device);

std::function<void(ID3D11Texture2D *)>
CreateSample_D3D11LightProbe(std::shared_ptr<Direct3D11Device> device);

std::function<void(ID3D11Texture2D *)>
CreateSample_D3D11LightProbeCross(std::shared_ptr<Direct3D11Device> device);

std::function<void(ID3D11Texture2D *, ID3D11DepthStencilView *,
                   const Matrix44 &)>
CreateSample_D3D11Mesh(std::shared_ptr<Direct3D11Device> device);

std::function<void(ID3D11Texture2D *, ID3D11DepthStencilView *,
                   const Matrix44 &)>
CreateSample_D3D11NormalMap(std::shared_ptr<Direct3D11Device> device);

std::function<void(ID3D11Texture2D *, ID3D11DepthStencilView *,
                   const Matrix44 &)>
CreateSample_D3D11ParallaxMap(std::shared_ptr<Direct3D11Device> device);

std::function<void(ID3D11Texture2D *)>
CreateSample_D3D11RayMarch(std::shared_ptr<Direct3D11Device> device);

std::function<void(ID3D11Texture2D *, ID3D11DepthStencilView *,
                   const Matrix44 &)>
CreateSample_D3D11Scene(std::shared_ptr<Direct3D11Device> device,
                        const std::vector<Instance> &scene);

std::function<void(ID3D11Texture2D *, ID3D11DepthStencilView *,
                   const Matrix44 &)>
CreateSample_D3D11ShadowMap(std::shared_ptr<Direct3D11Device> device);

std::function<void(ID3D11Texture2D *)>
CreateSample_D3D11ShowTexture(std::shared_ptr<Direct3D11Device> device);

std::function<void(ID3D11Texture2D *)>
CreateSample_D3D11Tessellation(std::shared_ptr<Direct3D11Device> device);

std::function<void(ID3D11Texture2D *)>
CreateSample_D3D11Voxel(std::shared_ptr<Direct3D11Device> device);

std::shared_ptr<Object>
CreateSample_D3D11ShaderToy(std::shared_ptr<Direct3D11Device> device);

std::function<void(ID3D12Resource *)>
CreateSample_D3D12Basic(std::shared_ptr<Direct3D12Device> device);

std::function<void(ID3D12Resource *)>
CreateSample_D3D12Scene(std::shared_ptr<Direct3D12Device> device,
                        const std::vector<Instance> &scene);

std::function<void(ID3D12Resource *)>
CreateSample_DXRAmbientOcclusion(std::shared_ptr<Direct3D12Device> device);

std::function<void(ID3D12Resource *)>
CreateSample_DXRBasic(std::shared_ptr<Direct3D12Device> device);

std::function<void(ID3D12Resource *)>
CreateSample_DXRMesh(std::shared_ptr<Direct3D12Device> device);

std::function<void(ID3D12Resource *)>
CreateSample_DXRPathTrace(std::shared_ptr<Direct3D12Device> device);

std::function<void(ID3D12Resource *)>
CreateSample_DXRScene(std::shared_ptr<Direct3D12Device> device,
                      const std::vector<Instance> &scene);

std::function<void(ID3D12Resource *)>
CreateSample_DXRTexture(std::shared_ptr<Direct3D12Device> device);

std::function<void(ID3D12Resource *)>
CreateSample_DXRWhitted(std::shared_ptr<Direct3D12Device> device);

std::function<void()>
CreateSample_OpenGLBasic(std::shared_ptr<OpenGLDevice> device);

#if VULKAN_INSTALLED
std::function<void(VKDevice *, vk::Image)> CreateSample_VKBasic();
#endif // VULKAN_INSTALLED