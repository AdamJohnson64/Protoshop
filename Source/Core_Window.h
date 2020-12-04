#pragma once

#include "Core_D3D11.h"
#include "Core_D3D12.h"
#include "Core_OpenGL.h"
#include "Core_VK.h"
#include <d3d11.h>
#include <d3d12.h>
#include <functional>
#include <memory>

std::shared_ptr<Object> CreateNewWindow(std::shared_ptr<Direct3D11Device> device, std::function<void(ID3D11RenderTargetView*)> fnRender);
std::shared_ptr<Object> CreateNewWindow(std::shared_ptr<Direct3D11Device> device, std::function<void(ID3D11UnorderedAccessView*)> fnRender);
std::shared_ptr<Object> CreateNewWindow(std::shared_ptr<Direct3D12Device> device, std::function<void(ID3D12Resource*)> fnRender);
std::shared_ptr<Object> CreateNewWindow(std::shared_ptr<OpenGLDevice> device, std::function<void()> fnRender);
std::shared_ptr<Object> CreateNewWindow(std::shared_ptr<VKDevice> deviceVK, std::shared_ptr<Direct3D12Device> device12, std::function<void(VKDevice*,vk::Image)> fnRender);