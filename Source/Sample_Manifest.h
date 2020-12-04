#pragma once

#include "Core_D3D11.h"
#include <d3d11.h>
#include <functional>

class DXGISwapChain;
class Direct3D11Device;
class Direct3D12Device;
class OpenGLDevice;
class VKDevice;

#include <memory>

std::function<void(ID3D11RenderTargetView *)>
CreateSample_D3D11Basic(std::shared_ptr<Direct3D11Device> device);

std::shared_ptr<ISample>
CreateSample_D3D11ComputeCanvas(std::shared_ptr<DXGISwapChain> swapchain,
                                std::shared_ptr<Direct3D11Device> device);
std::shared_ptr<ISample>
CreateSample_D3D11DrawingContext(std::shared_ptr<DXGISwapChain> swapchain,
                                 std::shared_ptr<Direct3D11Device> device);
std::shared_ptr<ISample>
CreateSample_D3D11LightProbe(std::shared_ptr<DXGISwapChain> swapchain,
                             std::shared_ptr<Direct3D11Device> device);
std::shared_ptr<ISample>
CreateSample_D3D11Mesh(std::shared_ptr<DXGISwapChain> swapchain,
                       std::shared_ptr<Direct3D11Device> device);

std::shared_ptr<ISample>
CreateSample_D3D11NormalMap(std::shared_ptr<DXGISwapChain> swapchain,
                            std::shared_ptr<Direct3D11Device> device);

std::shared_ptr<ISample>
CreateSample_D3D11ParallaxMap(std::shared_ptr<DXGISwapChain> swapchain,
                              std::shared_ptr<Direct3D11Device> device);

std::shared_ptr<ISample>
CreateSample_D3D11RayMarch(std::shared_ptr<DXGISwapChain> swapchain,
                           std::shared_ptr<Direct3D11Device> device);
std::shared_ptr<ISample>
CreateSample_D3D11Scene(std::shared_ptr<DXGISwapChain> swapchain,
                        std::shared_ptr<Direct3D11Device> device);

std::shared_ptr<ISample>
CreateSample_D3D11ShowTexture(std::shared_ptr<DXGISwapChain> swapchain,
                              std::shared_ptr<Direct3D11Device> device);

std::shared_ptr<ISample>
CreateSample_D3D11Tessellation(std::shared_ptr<DXGISwapChain> swapchain,
                               std::shared_ptr<Direct3D11Device> device);
std::shared_ptr<ISample>
CreateSample_D3D11Voxel(std::shared_ptr<DXGISwapChain> swapchain,
                        std::shared_ptr<Direct3D11Device> device);

std::shared_ptr<Object>
CreateSample_D3D11ShaderToy(std::shared_ptr<Direct3D11Device> device);

std::shared_ptr<ISample>
CreateSample_D3D12Basic(std::shared_ptr<DXGISwapChain> swapchain,
                        std::shared_ptr<Direct3D12Device> device);
std::shared_ptr<ISample>
CreateSample_D3D12Scene(std::shared_ptr<DXGISwapChain> swapchain,
                        std::shared_ptr<Direct3D12Device> device);

std::shared_ptr<ISample>
CreateSample_DXRAmbientOcclusion(std::shared_ptr<DXGISwapChain> swapchain,
                                 std::shared_ptr<Direct3D12Device> device);
std::shared_ptr<ISample>
CreateSample_DXRBasic(std::shared_ptr<DXGISwapChain> swapchain,
                      std::shared_ptr<Direct3D12Device> device);
std::shared_ptr<ISample>
CreateSample_DXRMesh(std::shared_ptr<DXGISwapChain> swapchain,
                     std::shared_ptr<Direct3D12Device> device);
std::shared_ptr<ISample>
CreateSample_DXRPathTrace(std::shared_ptr<DXGISwapChain> swapchain,
                          std::shared_ptr<Direct3D12Device> device);
std::shared_ptr<ISample>
CreateSample_DXRScene(std::shared_ptr<DXGISwapChain> swapchain,
                      std::shared_ptr<Direct3D12Device> device);
std::shared_ptr<ISample>
CreateSample_DXRTexture(std::shared_ptr<DXGISwapChain> swapchain,
                        std::shared_ptr<Direct3D12Device> device);
std::shared_ptr<ISample>
CreateSample_DXRWhitted(std::shared_ptr<DXGISwapChain> swapchain,
                        std::shared_ptr<Direct3D12Device> device);

std::shared_ptr<ISample>
CreateSample_OpenGLBasic(std::shared_ptr<OpenGLDevice> device,
                         std::shared_ptr<IWindow> pWindow);

#if VULKAN_INSTALLED
std::shared_ptr<ISample>
CreateSample_VKBasic(std::shared_ptr<DXGISwapChain> swapchain,
                     std::shared_ptr<VKDevice> pDeviceVK,
                     std::shared_ptr<Direct3D12Device> pDeviceD3D12);
#endif // VULKAN_INSTALLED