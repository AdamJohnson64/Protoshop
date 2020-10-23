#include "Core_D3D.h"
#include "Core_D3D12Util.h"
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
            CComPtr<IDXGISwapChain1> pDXGISwapChain1;
            TRYD3D(pDXGIFactory->CreateSwapChainForHwnd(pDevice->GetID3D11Device(), hWindow, &descSwapChain, nullptr, nullptr, &pDXGISwapChain1));
            TRYD3D(pDXGISwapChain1->QueryInterface<IDXGISwapChain4>(&pDXGISwapChain));
        }
    }
    ~DXGISwapChainImpl()
    {
        if (pDevice12 != nullptr)
        {
            D3D12WaitForGPUIdle(pDevice12);
        }
    }
    DXGISwapChainImpl(std::shared_ptr<Direct3D12Device> pDevice, HWND hWindow) :
        pDevice12(pDevice)
    {
        CComPtr<IDXGIFactory7> pDXGIFactory;
        TRYD3D(CreateDXGIFactory(__uuidof(IDXGIFactory7), (void**)&pDXGIFactory));
        {
            DXGI_SWAP_CHAIN_DESC1 descSwapChain = {};
            descSwapChain.Width = RENDERTARGET_WIDTH;
            descSwapChain.Height = RENDERTARGET_HEIGHT;
            descSwapChain.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            descSwapChain.SampleDesc.Count = 1;
            descSwapChain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            descSwapChain.BufferCount = 2;
            descSwapChain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            CComPtr<IDXGISwapChain1> pDXGISwapChain1;
            TRYD3D(pDXGIFactory->CreateSwapChainForHwnd(pDevice->GetID3D12CommandQueue(), hWindow, &descSwapChain, nullptr, nullptr, &pDXGISwapChain1));
            TRYD3D(pDXGISwapChain1->QueryInterface<IDXGISwapChain4>(&pDXGISwapChain));
        }
    }
    IDXGISwapChain4* GetIDXGISwapChain() override
    {
        return pDXGISwapChain.p;
    }
private:
    std::shared_ptr<Direct3D12Device> pDevice12;
    CComPtr<IDXGISwapChain4> pDXGISwapChain;
};

std::shared_ptr<DXGISwapChain> CreateDXGISwapChain(std::shared_ptr<Direct3D11Device> pDevice, HWND hWindow)
{
    return std::shared_ptr<DXGISwapChain>(new DXGISwapChainImpl(pDevice, hWindow));
}

std::shared_ptr<DXGISwapChain> CreateDXGISwapChain(std::shared_ptr<Direct3D12Device> pDevice, HWND hWindow)
{
    return std::shared_ptr<DXGISwapChain>(new DXGISwapChainImpl(pDevice, hWindow));
}