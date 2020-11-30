///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Basic
///////////////////////////////////////////////////////////////////////////////
// This sample is about as basic as you can get with a window and Direct3D 11.
// We create a window and swap chain then clear a single render target to a
// color. This shows the most basic swap chain and device operation required to
// get something to display.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3D11Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Sample.h"
#include <atlbase.h>
#include <memory>

class Sample_D3D11Basic : public Sample
{
private:
    std::shared_ptr<DXGISwapChain> m_pSwapChain;
    std::shared_ptr<Direct3D11Device> m_pDevice;
public:
    Sample_D3D11Basic(std::shared_ptr<DXGISwapChain> swapchain, std::shared_ptr<Direct3D11Device> device) :
        m_pSwapChain(swapchain),
        m_pDevice(device)
    {
    }
    void Render()
    {
        // Get the backbuffer and create a render target from it.
        CComPtr<ID3D11RenderTargetView> pD3D11RenderTargetView = D3D11_Create_RTV_From_SwapChain(m_pDevice->GetID3D11Device(), m_pSwapChain->GetIDXGISwapChain());
        // Beginning of rendering.
        {
            float color[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
            m_pDevice->GetID3D11DeviceContext()->ClearRenderTargetView(pD3D11RenderTargetView, color);
        }
        m_pDevice->GetID3D11DeviceContext()->Flush();
        // End of rendering; send to display.
        m_pSwapChain->GetIDXGISwapChain()->Present(0, 0);
    }
};

std::shared_ptr<Sample> CreateSample_D3D11Basic(std::shared_ptr<DXGISwapChain> swapchain, std::shared_ptr<Direct3D11Device> device)
{
    return std::shared_ptr<Sample>(new Sample_D3D11Basic(swapchain, device));
}