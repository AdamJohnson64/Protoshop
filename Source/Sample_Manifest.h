#pragma once

#include "Core_D3D11.h"
#include "Core_DXGI.h"
#include "Core_OpenGL.h"
#include "Core_VK.h"
#include "Core_Window.h"
#include <memory>

std::shared_ptr<Sample>
CreateSample_D3D11Basic(std::shared_ptr<DXGISwapChain> swapchain,
                        std::shared_ptr<Direct3D11Device> device);
std::shared_ptr<Sample>
CreateSample_D3D11ComputeCanvas(std::shared_ptr<DXGISwapChain> swapchain,
                                std::shared_ptr<Direct3D11Device> device);
std::shared_ptr<Sample>
CreateSample_D3D11DrawingContext(std::shared_ptr<DXGISwapChain> swapchain,
                                 std::shared_ptr<Direct3D11Device> device);
std::shared_ptr<Sample>
CreateSample_D3D11LightProbe(std::shared_ptr<DXGISwapChain> swapchain,
                             std::shared_ptr<Direct3D11Device> device);
std::shared_ptr<Sample>
CreateSample_D3D11Mesh(std::shared_ptr<DXGISwapChain> swapchain,
                       std::shared_ptr<Direct3D11Device> device);

std::shared_ptr<Sample>
CreateSample_D3D11NormalMap(std::shared_ptr<DXGISwapChain> swapchain,
                            std::shared_ptr<Direct3D11Device> device);

std::shared_ptr<Sample>
CreateSample_D3D11RayMarch(std::shared_ptr<DXGISwapChain> swapchain,
                           std::shared_ptr<Direct3D11Device> device);
std::shared_ptr<Sample>
CreateSample_D3D11Scene(std::shared_ptr<DXGISwapChain> swapchain,
                        std::shared_ptr<Direct3D11Device> device);

std::shared_ptr<Sample>
CreateSample_D3D11ShowTexture(std::shared_ptr<DXGISwapChain> swapchain,
                              std::shared_ptr<Direct3D11Device> device);

std::shared_ptr<Sample>
CreateSample_D3D11Tessellation(std::shared_ptr<DXGISwapChain> swapchain,
                               std::shared_ptr<Direct3D11Device> device);
std::shared_ptr<Sample>
CreateSample_D3D11Voxel(std::shared_ptr<DXGISwapChain> swapchain,
                        std::shared_ptr<Direct3D11Device> device);

std::shared_ptr<Object>
CreateSample_D3D11ShaderToy(std::shared_ptr<Direct3D11Device> device);

std::shared_ptr<Sample>
CreateSample_D3D12Basic(std::shared_ptr<DXGISwapChain> swapchain,
                        std::shared_ptr<Direct3D12Device> device);
std::shared_ptr<Sample>
CreateSample_D3D12Mesh(std::shared_ptr<DXGISwapChain> swapchain,
                       std::shared_ptr<Direct3D12Device> device);

std::shared_ptr<Sample>
CreateSample_DXRAmbientOcclusion(std::shared_ptr<DXGISwapChain> swapchain,
                                 std::shared_ptr<Direct3D12Device> device);
std::shared_ptr<Sample>
CreateSample_DXRBasic(std::shared_ptr<DXGISwapChain> swapchain,
                      std::shared_ptr<Direct3D12Device> device);
std::shared_ptr<Sample>
CreateSample_DXRMesh(std::shared_ptr<DXGISwapChain> swapchain,
                     std::shared_ptr<Direct3D12Device> device);
std::shared_ptr<Sample>
CreateSample_DXRPathTrace(std::shared_ptr<DXGISwapChain> swapchain,
                          std::shared_ptr<Direct3D12Device> device);
std::shared_ptr<Sample>
CreateSample_DXRScene(std::shared_ptr<DXGISwapChain> swapchain,
                      std::shared_ptr<Direct3D12Device> device);
std::shared_ptr<Sample>
CreateSample_DXRTexture(std::shared_ptr<DXGISwapChain> swapchain,
                        std::shared_ptr<Direct3D12Device> device);
std::shared_ptr<Sample>
CreateSample_DXRWhitted(std::shared_ptr<DXGISwapChain> swapchain,
                        std::shared_ptr<Direct3D12Device> device);

std::shared_ptr<Sample>
CreateSample_OpenGLBasic(std::shared_ptr<OpenGLDevice> device,
                         std::shared_ptr<Window> pWindow);

#if VULKAN_INSTALLED
std::shared_ptr<Sample>
CreateSample_VKBasic(std::shared_ptr<DXGISwapChain> swapchain,
                     std::shared_ptr<VKDevice> pDeviceVK,
                     std::shared_ptr<Direct3D12Device> pDeviceD3D12);
#endif // VULKAN_INSTALLED