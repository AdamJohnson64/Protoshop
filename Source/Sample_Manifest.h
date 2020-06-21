#pragma once

#include "Core_D3D11.h"
#include "Core_DXGI.h"
#include "Core_OpenGL.h"
#include "Core_Window.h"
#include <memory>

std::shared_ptr<Sample> CreateSample_D3D11Basic(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D11Device> pDevice);
std::shared_ptr<Sample> CreateSample_D3D11ComputeCanvas(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D11Device> pDevice);
std::shared_ptr<Sample> CreateSample_D3D11DrawingContext(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D11Device> pDevice);
std::shared_ptr<Sample> CreateSample_D3D11Imgui(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D11Device> pDevice);

std::shared_ptr<Sample> CreateSample_D3D12Basic(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D12Device> pDevice);
std::shared_ptr<Sample> CreateSample_D3D12Imgui(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D12Device> pDevice);

std::shared_ptr<Sample> CreateSample_OpenGLBasic(std::shared_ptr<OpenGLDevice> pDevice, std::shared_ptr<Window> pWindow);