#include "Core_D3D.h"
#include "Core_D3D11.h"

#include <atlbase.h>
#include <d3d11.h>
#include <dxgi1_6.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

CComPtr<IDXGISwapChain1> pDXGISwapChain;
CComPtr<ID3D11Device> pD3D11Device;
CComPtr<ID3D11DeviceContext> pD3D11DeviceContext;

void InitializeDirect3D11(HWND hWindow)
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
        TRYD3D(pDXGIFactory->CreateSwapChainForHwnd(GetID3D11Device(), hWindow, &descSwapChain, nullptr, nullptr, &pDXGISwapChain));
    }
}

static void CreateDirect3D11()
{
    if (pD3D11Device == nullptr && pD3D11DeviceContext == nullptr)
    {
        TRYD3D(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_DEBUG, nullptr, 0, D3D11_SDK_VERSION, &pD3D11Device, nullptr, &pD3D11DeviceContext));
    }
}

IDXGISwapChain1* GetIDXGISwapChain()
{
    return pDXGISwapChain.p;
}

ID3D11Device* GetID3D11Device()
{
    CreateDirect3D11();
    return pD3D11Device.p;
}

ID3D11DeviceContext* GetID3D11DeviceContext()
{
    CreateDirect3D11();
    return pD3D11DeviceContext.p;
}