#pragma once

#include "Core_D3D11.h"
#include "Core_DXGI.h"
#include "Sample.h"
#include <memory>

class Sample_D3D11Base : public Sample
{
public:
    Sample_D3D11Base(std::shared_ptr<DXGISwapChain> swapchain, std::shared_ptr<Direct3D11Device> device);
    void Render() override;
    virtual void RenderSample() = 0;
protected:
    std::shared_ptr<DXGISwapChain> m_pSwapChain;
    std::shared_ptr<Direct3D11Device> m_pDevice;
};