///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Show Texture
///////////////////////////////////////////////////////////////////////////////
// If you ever find yourself with a texture and just want to see what's in it
// then drop that image in here. We use a compute render to extract the texels
// directly - the image on screen should be a 1-1 reproduction.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3D11Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_Util.h"
#include "ImageUtil.h"
#include "Sample.h"
#include <atlbase.h>
#include <cstdint>
#include <memory>

class Sample_D3D11ShowTexture : public Sample {
private:
  std::shared_ptr<DXGISwapChain> m_pSwapChain;
  std::shared_ptr<Direct3D11Device> m_pDevice;
  CComPtr<ID3D11ComputeShader> m_pComputeShader;
  CComPtr<ID3D11Texture2D> m_pTex2DImage;
  CComPtr<ID3D11ShaderResourceView> m_pSRVImage;

public:
  Sample_D3D11ShowTexture(std::shared_ptr<DXGISwapChain> swapchain,
                            std::shared_ptr<Direct3D11Device> device)
      : m_pSwapChain(swapchain), m_pDevice(device) {
    // Create a compute shader.
    CComPtr<ID3DBlob> pD3DBlobCodeCS =
        CompileShader("cs_5_0", "main", R"SHADER(
RWTexture2D<float4> renderTarget;
Texture2D<float4> userImage;

[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    renderTarget[dispatchThreadId.xy] = userImage.Load(int3(dispatchThreadId.xy, 0));
})SHADER");
    {
      struct Pixel {
        uint8_t B, G, R, A;
      };
      const uint32_t imageWidth = 320;
      const uint32_t imageHeight = 200;
      const uint32_t imageStride = sizeof(Pixel) * imageWidth;
      Pixel imageRaw[imageWidth * imageHeight];
      Image_Fill_Commodore64(imageRaw, imageWidth, imageHeight, imageStride);
      m_pTex2DImage = D3D11_Create_Texture2D(
          m_pDevice->GetID3D11Device(), DXGI_FORMAT_B8G8R8A8_UNORM, imageWidth,
          imageHeight, imageRaw);
    }
    {
      D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
      desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
      desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
      desc.Texture2D.MipLevels = 1;
      TRYD3D(m_pDevice->GetID3D11Device()->CreateShaderResourceView(
          m_pTex2DImage, &desc, &m_pSRVImage.p));
    }
    TRYD3D(m_pDevice->GetID3D11Device()->CreateComputeShader(
        pD3DBlobCodeCS->GetBufferPointer(), pD3DBlobCodeCS->GetBufferSize(),
        nullptr, &m_pComputeShader));
  }
  void Render() {
    CComPtr<ID3D11UnorderedAccessView> pUAVTarget =
        D3D11_Create_UAV_From_SwapChain(m_pDevice->GetID3D11Device(),
                                        m_pSwapChain->GetIDXGISwapChain());
    m_pDevice->GetID3D11DeviceContext()->ClearState();
    // Beginning of rendering.
    m_pDevice->GetID3D11DeviceContext()->CSSetUnorderedAccessViews(
        0, 1, &pUAVTarget.p, nullptr);
    m_pDevice->GetID3D11DeviceContext()->CSSetShader(m_pComputeShader, nullptr,
                                                     0);
    m_pDevice->GetID3D11DeviceContext()->CSSetShaderResources(0, 1,
                                                              &m_pSRVImage.p);
    m_pDevice->GetID3D11DeviceContext()->Dispatch(RENDERTARGET_WIDTH,
                                                  RENDERTARGET_HEIGHT, 1);
    m_pDevice->GetID3D11DeviceContext()->ClearState();
    m_pDevice->GetID3D11DeviceContext()->Flush();
    // End of rendering; send to display.
    m_pSwapChain->GetIDXGISwapChain()->Present(0, 0);
  }
};

std::shared_ptr<Sample>
CreateSample_D3D11ShowTexture(std::shared_ptr<DXGISwapChain> swapchain,
                                std::shared_ptr<Direct3D11Device> device) {
  return std::shared_ptr<Sample>(
      new Sample_D3D11ShowTexture(swapchain, device));
}