#include "Sample_D3D11Base.h"
#include "Core_D3D.h"

#include <atlbase.h>

Sample_D3D11Base::Sample_D3D11Base(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D11Device> pDevice) :
    m_pSwapChain(pSwapChain),
    m_pDevice(pDevice)
{
}

void Sample_D3D11Base::Render()
{
    // Get the backbuffer and create a render target from it.
    CComPtr<ID3D11RenderTargetView> pD3D11RenderTargetView;
    {
        CComPtr<ID3D11Texture2D> pD3D11Texture2D;
        TRYD3D(m_pSwapChain->GetIDXGISwapChain()->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pD3D11Texture2D));
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        TRYD3D(m_pDevice->GetID3D11Device()->CreateRenderTargetView(pD3D11Texture2D, &rtvDesc, &pD3D11RenderTargetView));
    }
    // Beginning of rendering.
    {
        float color[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
        m_pDevice->GetID3D11DeviceContext()->ClearRenderTargetView(pD3D11RenderTargetView, color);
    }
    {
        D3D11_VIEWPORT viewportdesc = {};
        viewportdesc.Width = RENDERTARGET_WIDTH;
        viewportdesc.Height = RENDERTARGET_HEIGHT;
        viewportdesc.MaxDepth = 1.0f;
        m_pDevice->GetID3D11DeviceContext()->RSSetViewports(1, &viewportdesc);
    }
    m_pDevice->GetID3D11DeviceContext()->OMSetRenderTargets(1, &pD3D11RenderTargetView.p, nullptr);
    RenderSample();
    m_pDevice->GetID3D11DeviceContext()->Flush();
    // End of rendering; send to display.
    m_pSwapChain->GetIDXGISwapChain()->Present(0, 0);
}