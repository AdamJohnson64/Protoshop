#include "Core_D3D.h"
#include "Core_D3D11.h"

#include <atlbase.h>
#include <d3d11.h>

#pragma comment(lib, "d3d11.lib")

CComPtr<IDXGISwapChain> pDXGISwapChain;
CComPtr<ID3D11Device> pD3D11Device;
CComPtr<ID3D11DeviceContext> pD3D11DeviceContext;

void InitializeDirect3D11(HWND hWindow)
{
    DXGI_SWAP_CHAIN_DESC swapchain = {};
    swapchain.BufferDesc.Width = RENDERTARGET_WIDTH;
    swapchain.BufferDesc.Height = RENDERTARGET_HEIGHT;
    swapchain.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapchain.BufferUsage = DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS;
    swapchain.BufferCount = 2;
    swapchain.SampleDesc.Count = 1;
    swapchain.OutputWindow = hWindow;
    swapchain.Windowed = TRUE;
    swapchain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    TRYD3D(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_DEBUG, nullptr, 0, D3D11_SDK_VERSION, &swapchain, &pDXGISwapChain, &pD3D11Device, nullptr, &pD3D11DeviceContext));
}

IDXGISwapChain* GetIDXGISwapChain()
{
    return pDXGISwapChain.p;
}

ID3D11Device* GetID3D11Device()
{
    return pD3D11Device.p;
}

ID3D11DeviceContext* GetID3D11DeviceContext()
{
    return pD3D11DeviceContext.p;
}