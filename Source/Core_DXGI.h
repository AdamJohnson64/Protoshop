#pragma once

#include "Core_D3D11.h"
#include "Core_D3D12.h"
#include "Core_Object.h"
#include <dxgi1_6.h>
#include <memory>

class DXGISwapChain : public Object
{
public:
    virtual IDXGISwapChain4* GetIDXGISwapChain() = 0;
};

std::shared_ptr<DXGISwapChain> CreateDXGISwapChain(std::shared_ptr<Direct3D11Device> pDevice, HWND hWindow);
std::shared_ptr<DXGISwapChain> CreateDXGISwapChain(std::shared_ptr<Direct3D12Device> pDevice, HWND hWindow);