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
#include "SampleResources.h"
#include <atlbase.h>
#include <functional>
#include <memory>

std::function<void(const SampleResourcesD3D12RTV &)>
CreateSample_D3D12Basic(std::shared_ptr<Direct3D12Device> m_pDevice) {
  return [=](const SampleResourcesD3D12RTV &sampleResources) {
    m_pDevice->m_pDevice->CreateRenderTargetView(
        sampleResources.BackBufferResource,
        &Make_D3D12_RENDER_TARGET_VIEW_DESC_SwapChainDefault(),
        m_pDevice->m_pDescriptorHeapRTV->GetCPUDescriptorHandleForHeapStart());
    D3D12_Run_Synchronously(
        m_pDevice.get(), [&](ID3D12GraphicsCommandList5 *commandList) {
          // Put the RTV into render target state and clear it before use.
          commandList->ResourceBarrier(
              1,
              &Make_D3D12_RESOURCE_BARRIER(sampleResources.BackBufferResource,
                                           D3D12_RESOURCE_STATE_COMMON,
                                           D3D12_RESOURCE_STATE_RENDER_TARGET));
          {
            static float r = 0;
            float color[4] = {r, 1, 0, 1};
            r += 0.01f;
            if (r > 1.0f)
              r -= 1.0f;
            commandList->ClearRenderTargetView(
                m_pDevice->m_pDescriptorHeapRTV
                    ->GetCPUDescriptorHandleForHeapStart(),
                color, 0, nullptr);
          }
          // Transition the render target into presentation state for display.
          commandList->ResourceBarrier(
              1,
              &Make_D3D12_RESOURCE_BARRIER(sampleResources.BackBufferResource,
                                           D3D12_RESOURCE_STATE_RENDER_TARGET,
                                           D3D12_RESOURCE_STATE_PRESENT));
        });
  };
}