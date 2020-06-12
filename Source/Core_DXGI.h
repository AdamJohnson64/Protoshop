#pragma once

#include "Core_D3D11.h"
#include <dxgi1_6.h>
#include <memory>

class DXGISwapChain
{
public:
    virtual ~DXGISwapChain() = default;
    virtual IDXGISwapChain1* GetIDXGISwapChain() = 0;
};

std::shared_ptr<DXGISwapChain> CreateDXGISwapChain(std::shared_ptr<Direct3D11Device> pDevice, HWND hWindow);