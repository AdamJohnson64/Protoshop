#include "Core_D3D.h"
#include "Core_DXGI.h"
#include <atlbase.h>
#include <memory>

#pragma comment(lib, "dxgi.lib")

class DXGISwapChainImpl : public DXGISwapChain
{
public:
    DXGISwapChainImpl(std::shared_ptr<Direct3D11Device> pDevice, HWND hWindow)
    {
        CComPtr<IDXGIFactory7> pDXGIFactory;
        TRYD3D(CreateDXGIFactory(__uuidof(IDXGIFactory7), (void**)&pDXGIFactory));
        {
            DXGI_SWAP_CHAIN_DESC1 descSwapChain = {};
            descSwapChain.Width = RENDERTARGET_WIDTH;
            descSwapChain.Height = RENDERTARGET_HEIGHT;
            descSwapChain.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            descSwapChain.SampleDesc.Count = 1;
            descSwapChain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS;
            descSwapChain.BufferCount = 2;
            descSwapChain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            TRYD3D(pDXGIFactory->CreateSwapChainForHwnd(pDevice->GetID3D11Device(), hWindow, &descSwapChain, nullptr, nullptr, &pDXGISwapChain));
        }
    }
    IDXGISwapChain1* GetIDXGISwapChain() override
    {
        return pDXGISwapChain.p;
    }
private:
    CComPtr<IDXGISwapChain1> pDXGISwapChain;
};

std::shared_ptr<DXGISwapChain> CreateDXGISwapChain(std::shared_ptr<Direct3D11Device> pDevice, HWND hWindow)
{
    return std::shared_ptr<DXGISwapChain>(new DXGISwapChainImpl(pDevice, hWindow));
}