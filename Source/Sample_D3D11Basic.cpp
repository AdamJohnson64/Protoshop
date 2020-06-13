#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3DCompiler.h"
#include "Sample.h"
#include <atlbase.h>
#include <memory>

class Sample_D3D11Basic : public Sample
{
private:
    std::shared_ptr<DXGISwapChain> m_pSwapChain;
    std::shared_ptr<Direct3D11Device> m_pDevice;
public:
    Sample_D3D11Basic(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D11Device> pDevice) :
        m_pSwapChain(pSwapChain),
        m_pDevice(pDevice)
    {
    }
    void Render()
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
        m_pDevice->GetID3D11DeviceContext()->Flush();
        // End of rendering; send to display.
        m_pSwapChain->GetIDXGISwapChain()->Present(0, 0);
    }
};

std::shared_ptr<Sample> CreateSample_D3D11Basic(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D11Device> pDevice)
{
    return std::shared_ptr<Sample>(new Sample_D3D11Basic(pSwapChain, pDevice));
}