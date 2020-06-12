#pragma once

#include "Core_D3D11.h"
#include "Core_DXGI.h"
#include <memory>

class Sample
{
public:
    virtual ~Sample() = default;
    virtual void Render() = 0;
};

Sample* CreateSample_ComputeCanvas(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D11Device> pDevice);
Sample* CreateSample_DrawingContext(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D11Device> pDevice);