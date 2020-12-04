///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Basic
///////////////////////////////////////////////////////////////////////////////
// This sample is about as basic as you can get with a window and Direct3D 11.
// We create a window and swap chain then clear a single render target to a
// color. This shows the most basic swap chain and device operation required to
// get something to display.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D11.h"
#include <d3d11.h>
#include <functional>
#include <memory>

std::function<void(ID3D11RenderTargetView *)>
CreateSample_D3D11Basic(std::shared_ptr<Direct3D11Device> device) {
  return [=](ID3D11RenderTargetView *rtvBackbuffer) {
    {
      float color[4] = {0.5f, 0.1f, 0.1f, 1.0f};
      device->GetID3D11DeviceContext()->ClearRenderTargetView(
          rtvBackbuffer, color);
    }
    device->GetID3D11DeviceContext()->Flush();
  };
}