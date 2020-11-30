///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 12 Basic
///////////////////////////////////////////////////////////////////////////////
// This sample is about as basic as you can get with a window and Direct3D 12.
// We create a window and swap chain then clear a single render target to a
// color. This shows the most basic swap chain and device operation required to
// get something to display. This sample does not need a root signature as no
// shader operation is performed beyond a blit-to-rendertarget.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include "Core_DXGI.h"
#include "Sample.h"
#include <atlbase.h>
#include <functional>
#include <memory>

class Sample_D3D12Basic : public Sample {
private:
  std::shared_ptr<DXGISwapChain> m_pSwapChain;
  std::shared_ptr<Direct3D12Device> m_pDevice;

public:
  Sample_D3D12Basic(std::shared_ptr<DXGISwapChain> swapchain,
                    std::shared_ptr<Direct3D12Device> device)
      : m_pSwapChain(swapchain), m_pDevice(device) {}
  void Render() override {
    CComPtr<ID3D12Resource> pD3D12Resource;
    TRYD3D(m_pSwapChain->GetIDXGISwapChain()->GetBuffer(
        m_pSwapChain->GetIDXGISwapChain()->GetCurrentBackBufferIndex(),
        __uuidof(ID3D12Resource), (void **)&pD3D12Resource));
    pD3D12Resource->SetName(L"D3D12Resource (Backbuffer)");
    m_pDevice->m_pDevice->CreateRenderTargetView(
        pD3D12Resource, &Make_D3D12_RENDER_TARGET_VIEW_DESC_SwapChainDefault(),
        m_pDevice->m_pDescriptorHeapRTV->GetCPUDescriptorHandleForHeapStart());
    D3D12_Run_Synchronously(
        m_pDevice.get(),
        [&](ID3D12GraphicsCommandList5 *pD3D12GraphicsCommandList) {
          // Put the RTV into render target state and clear it before use.
          pD3D12GraphicsCommandList->ResourceBarrier(
              1, &Make_D3D12_RESOURCE_BARRIER(
                     pD3D12Resource, D3D12_RESOURCE_STATE_COMMON,
                     D3D12_RESOURCE_STATE_RENDER_TARGET));
          {
            static float r = 0;
            float color[4] = {r, 1, 0, 1};
            r += 0.01f;
            if (r > 1.0f)
              r -= 1.0f;
            pD3D12GraphicsCommandList->ClearRenderTargetView(
                m_pDevice->m_pDescriptorHeapRTV
                    ->GetCPUDescriptorHandleForHeapStart(),
                color, 0, nullptr);
          }
          // Transition the render target into presentation state for display.
          pD3D12GraphicsCommandList->ResourceBarrier(
              1, &Make_D3D12_RESOURCE_BARRIER(
                     pD3D12Resource, D3D12_RESOURCE_STATE_RENDER_TARGET,
                     D3D12_RESOURCE_STATE_PRESENT));
        });
    // Swap the backbuffer and send this to the desktop composer for display.
    TRYD3D(m_pSwapChain->GetIDXGISwapChain()->Present(0, 0));
  }
};

std::shared_ptr<Sample>
CreateSample_D3D12Basic(std::shared_ptr<DXGISwapChain> swapchain,
                        std::shared_ptr<Direct3D12Device> device) {
  return std::shared_ptr<Sample>(new Sample_D3D12Basic(swapchain, device));
}