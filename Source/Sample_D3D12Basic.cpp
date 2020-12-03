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
#include "Core_ISample.h"
#include "Core_Object.h"
#include <atlbase.h>
#include <functional>
#include <memory>

class Sample_D3D12Basic : public Object, public ISample {
private:
  ////////////////////////////////////////////////////////////////////////////////
  // Use a lambda capture to pass initialized objects into the rendering loop
  // function. This is just a nice way to avoid having lots of member variables
  // and initializers all over the place.
  std::function<void()> m_fnRender;

public:
  Sample_D3D12Basic(std::shared_ptr<DXGISwapChain> swapchain,
                    std::shared_ptr<Direct3D12Device> device) {
    m_fnRender = [=]() {
      CComPtr<ID3D12Resource> resourceBackbuffer;
      TRYD3D(swapchain->GetIDXGISwapChain()->GetBuffer(
          swapchain->GetIDXGISwapChain()->GetCurrentBackBufferIndex(),
          __uuidof(ID3D12Resource), (void **)&resourceBackbuffer));
      resourceBackbuffer->SetName(L"D3D12Resource (Backbuffer)");
      device->m_pDevice->CreateRenderTargetView(
          resourceBackbuffer,
          &Make_D3D12_RENDER_TARGET_VIEW_DESC_SwapChainDefault(),
          device->m_pDescriptorHeapRTV->GetCPUDescriptorHandleForHeapStart());
      D3D12_Run_Synchronously(
          device.get(), [&](ID3D12GraphicsCommandList5 *commandList) {
            // Put the RTV into render target state and clear it before use.
            commandList->ResourceBarrier(
                1, &Make_D3D12_RESOURCE_BARRIER(
                       resourceBackbuffer, D3D12_RESOURCE_STATE_COMMON,
                       D3D12_RESOURCE_STATE_RENDER_TARGET));
            {
              static float r = 0;
              float color[4] = {r, 1, 0, 1};
              r += 0.01f;
              if (r > 1.0f)
                r -= 1.0f;
              commandList->ClearRenderTargetView(
                  device->m_pDescriptorHeapRTV
                      ->GetCPUDescriptorHandleForHeapStart(),
                  color, 0, nullptr);
            }
            // Transition the render target into presentation state for display.
            commandList->ResourceBarrier(
                1, &Make_D3D12_RESOURCE_BARRIER(
                       resourceBackbuffer, D3D12_RESOURCE_STATE_RENDER_TARGET,
                       D3D12_RESOURCE_STATE_PRESENT));
          });
      // Swap the backbuffer and send this to the desktop composer for display.
      TRYD3D(swapchain->GetIDXGISwapChain()->Present(0, 0));
    };
  }
  void Render() override { m_fnRender(); }
};

std::shared_ptr<ISample>
CreateSample_D3D12Basic(std::shared_ptr<DXGISwapChain> swapchain,
                        std::shared_ptr<Direct3D12Device> device) {
  return std::shared_ptr<ISample>(new Sample_D3D12Basic(swapchain, device));
}