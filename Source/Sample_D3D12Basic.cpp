#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include "Core_DXGI.h"
#include "Sample.h"
#include <atlbase.h>
#include <functional>
#include <memory>

class Sample_D3D12Basic : public Sample
{
private:
    std::shared_ptr<DXGISwapChain> m_pSwapChain;
    std::shared_ptr<Direct3D12Device> m_pDevice;
public:
    Sample_D3D12Basic(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D12Device> pDevice) :
        m_pSwapChain(pSwapChain),
        m_pDevice(pDevice)
    {
    }
    void Render() override
    {
        CComPtr<ID3D12Resource> pD3D12Resource;
        TRYD3D(m_pSwapChain->GetIDXGISwapChain()->GetBuffer(m_pSwapChain->GetIDXGISwapChain()->GetCurrentBackBufferIndex(), __uuidof(ID3D12Resource), (void**)&pD3D12Resource));
        pD3D12Resource->SetName(L"D3D12Resource (Backbuffer)");
        {
            D3D12_RENDER_TARGET_VIEW_DESC descRTV = {};
            descRTV.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            descRTV.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            m_pDevice->GetID3D12Device()->CreateRenderTargetView(pD3D12Resource, &descRTV, m_pDevice->GetID3D12DescriptorHeapRTV()->GetCPUDescriptorHandleForHeapStart());
        }
        RunOnGPU(m_pDevice, [&](ID3D12GraphicsCommandList* pD3D12GraphicsCommandList)
        {
            pD3D12GraphicsCommandList->SetGraphicsRootSignature(m_pDevice->GetID3D12RootSignature());
            // Put the RTV into render target state and clear it before use.
            {
                D3D12_RESOURCE_BARRIER descBarrier = {};
                descBarrier.Transition.pResource = pD3D12Resource;
                descBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
                descBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
                pD3D12GraphicsCommandList->ResourceBarrier(1, &descBarrier);
            }
            {
                static float r = 0;
                float color[4] = {r, 1, 0, 1};
                r += 0.01f;
                if (r > 1.0f) r -= 1.0f;
                pD3D12GraphicsCommandList->ClearRenderTargetView(m_pDevice->GetID3D12DescriptorHeapRTV()->GetCPUDescriptorHandleForHeapStart(), color, 0, nullptr);
            }
            // Transition the render target into presentation state for display.
            {
                D3D12_RESOURCE_BARRIER descBarrier = {};
                descBarrier.Transition.pResource = pD3D12Resource;
                descBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
                descBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
                pD3D12GraphicsCommandList->ResourceBarrier(1, &descBarrier);
            }
        });
        // Swap the backbuffer and send this to the desktop composer for display.
        TRYD3D(m_pSwapChain->GetIDXGISwapChain()->Present(0, 0));
    }
};

std::shared_ptr<Sample> CreateSample_D3D12Basic(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D12Device> pDevice)
{
    return std::shared_ptr<Sample>(new Sample_D3D12Basic(pSwapChain, pDevice));
}