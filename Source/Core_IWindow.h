#pragma once

#include "Core_D3D11.h"
#include "Core_DXGI.h"
#include <d3d11.h>
#include <Windows.h>
#include <functional>
#include <memory>

class ISample;

class IWindow {
public:
  virtual HWND GetWindowHandle() = 0;
  virtual void SetSample(std::shared_ptr<ISample> sample) = 0;
};

std::shared_ptr<IWindow> CreateNewWindow();
std::shared_ptr<Object> CreateNewWindow(std::shared_ptr<Direct3D11Device> device, std::function<void(IDXGISwapChain*)> fnRender);
std::shared_ptr<Object> CreateNewWindow(std::shared_ptr<Direct3D11Device> device, std::function<void(ID3D11RenderTargetView*)> fnRender);
std::shared_ptr<Object> CreateNewWindow(std::shared_ptr<Direct3D11Device> device, std::function<void(ID3D11UnorderedAccessView*)> fnRender);
std::shared_ptr<Object> CreateNewWindow(std::shared_ptr<Direct3D12Device> device, std::function<void(ID3D12Resource*)> fnRender);