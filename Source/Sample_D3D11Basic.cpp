///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Basic
///////////////////////////////////////////////////////////////////////////////
// This sample is about as basic as you can get with a window and Direct3D 11.
// We create a window and swap chain then clear a single render target to a
// color. This shows the most basic swap chain and device operation required to
// get something to display.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D11.h"
#include "Core_D3D11Util.h"
#include "SampleResources.h"
#include <d3d11.h>
#include <functional>
#include <memory>

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11Basic(std::shared_ptr<Direct3D11Device> device) {
  return [=](const SampleResourcesD3D11 &sampleResources) {
    {
      CComPtr<ID3D11RenderTargetView> rtvBackbuffer =
          D3D11_Create_RTV_From_Texture2D(device->GetID3D11Device(),
                                          sampleResources.BackBufferTexture);
      float color[4] = {0.5f, 0.1f, 0.1f, 1.0f};
      device->GetID3D11DeviceContext()->ClearRenderTargetView(rtvBackbuffer,
                                                              color);
    }
    device->GetID3D11DeviceContext()->Flush();
  };
}