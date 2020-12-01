///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Drawing Context
///////////////////////////////////////////////////////////////////////////////
// This sample performs on-demand rendering of simple primitives using an
// accumulated geometry buffer. This approach can be used to simulate GDI-like
// rendering for simple UI.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3D11Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_ISample.h"
#include "Core_Math.h"
#include "Core_Object.h"
#include "Core_Util.h"
#include <array>
#include <atlbase.h>
#include <functional>
#include <vector>

class Sample_D3D11DrawingContext : public Object, public ISample {
private:
  std::shared_ptr<DXGISwapChain> m_pSwapChain;
  std::shared_ptr<Direct3D11Device> m_pDevice;
  CComPtr<ID3D11VertexShader> m_pD3D11VertexShader;
  CComPtr<ID3D11PixelShader> m_pD3D11PixelShader;
  CComPtr<ID3D11InputLayout> m_pD3D11InputLayout;

public:
  Sample_D3D11DrawingContext(std::shared_ptr<DXGISwapChain> swapchain,
                             std::shared_ptr<Direct3D11Device> device)
      : m_pSwapChain(swapchain), m_pDevice(device) {
    ////////////////////////////////////////////////////////////////////////////////
    // Create a vertex shader.
    CComPtr<ID3DBlob> pD3DBlobCodeVS =
        CompileShader("vs_5_0", "main", R"SHADER(
float4 main(float4 pos : SV_Position) : SV_Position
{
        return float4(-1 + pos.x * 2 / 320, 1 - pos.y * 2 / 240, 0, 1);
})SHADER");
    TRYD3D(m_pDevice->GetID3D11Device()->CreateVertexShader(
        pD3DBlobCodeVS->GetBufferPointer(), pD3DBlobCodeVS->GetBufferSize(),
        nullptr, &m_pD3D11VertexShader));

    ////////////////////////////////////////////////////////////////////////////////
    // Create a pixel shader.
    ID3DBlob *pD3DBlobCodePS = CompileShader("ps_5_0", "main", R"SHADER(
float4 main() : SV_Target
{
        return float4(1, 1, 1, 1);
})SHADER");
    TRYD3D(m_pDevice->GetID3D11Device()->CreatePixelShader(
        pD3DBlobCodePS->GetBufferPointer(), pD3DBlobCodePS->GetBufferSize(),
        nullptr, &m_pD3D11PixelShader));

    ////////////////////////////////////////////////////////////////////////////////
    // Create an input layout.
    {
      D3D11_INPUT_ELEMENT_DESC inputdesc = {};
      inputdesc.SemanticName = "SV_Position";
      inputdesc.Format = DXGI_FORMAT_R32G32_FLOAT;
      inputdesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
      TRYD3D(m_pDevice->GetID3D11Device()->CreateInputLayout(
          &inputdesc, 1, pD3DBlobCodeVS->GetBufferPointer(),
          pD3DBlobCodeVS->GetBufferSize(), &m_pD3D11InputLayout));
    }
  }
  void Render() override {
    // Get the backbuffer and create a render target from it.
    CComPtr<ID3D11RenderTargetView> pD3D11RenderTargetView =
        D3D11_Create_RTV_From_SwapChain(m_pDevice->GetID3D11Device(),
                                        m_pSwapChain->GetIDXGISwapChain());
    m_pDevice->GetID3D11DeviceContext()->ClearState();
    // Beginning of rendering.
    m_pDevice->GetID3D11DeviceContext()->ClearRenderTargetView(
        pD3D11RenderTargetView,
        &std::array<FLOAT, 4>{0.1f, 0.1f, 0.1f, 1.0f}[0]);
    m_pDevice->GetID3D11DeviceContext()->RSSetViewports(
        1, &Make_D3D11_VIEWPORT(RENDERTARGET_WIDTH, RENDERTARGET_HEIGHT));
    m_pDevice->GetID3D11DeviceContext()->OMSetRenderTargets(
        1, &pD3D11RenderTargetView.p, nullptr);
    // Simple DrawingContext.
    std::vector<Vector2> vertices;
    std::function<void(const Vector2 &, const Vector2 &)> DCDrawLine =
        [&](const Vector2 &p0, const Vector2 &p1) {
          Vector2 bitangent = Normalize(Perpendicular(p1 - p0));
          vertices.push_back(p0 - bitangent);
          vertices.push_back(p1 - bitangent);
          vertices.push_back(p1 + bitangent);
          vertices.push_back(p1 + bitangent);
          vertices.push_back(p0 + bitangent);
          vertices.push_back(p0 - bitangent);
        };
    std::function<void()> DCFlush = [&]() {
      CComPtr<ID3D11Buffer> pD3D11BufferVertex = D3D11_Create_Buffer(
          m_pDevice->GetID3D11Device(), D3D11_BIND_VERTEX_BUFFER,
          sizeof(Vector2) * vertices.size(), &vertices[0]);
      {
        UINT uStrides[] = {sizeof(Vector2)};
        UINT uOffsets[] = {0};
        m_pDevice->GetID3D11DeviceContext()->IASetVertexBuffers(
            0, 1, &pD3D11BufferVertex.p, uStrides, uOffsets);
      }
      m_pDevice->GetID3D11DeviceContext()->Draw(vertices.size(), 0);
      m_pDevice->GetID3D11DeviceContext()->Flush();
      vertices.clear();
    };
    // Beginning of rendering.
    m_pDevice->GetID3D11DeviceContext()->VSSetShader(m_pD3D11VertexShader,
                                                     nullptr, 0);
    m_pDevice->GetID3D11DeviceContext()->PSSetShader(m_pD3D11PixelShader,
                                                     nullptr, 0);
    m_pDevice->GetID3D11DeviceContext()->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pDevice->GetID3D11DeviceContext()->IASetInputLayout(m_pD3D11InputLayout);
    static float angle = 0;
    for (int i = 0; i < 10; ++i) {
      const float angleThis = angle + i * 0.1f;
      DCDrawLine({160, 120},
                 {160 + sinf(angleThis) * 100, 120 - cosf(angleThis) * 100});
    }
    angle += 0.01f;
    DCFlush();
    m_pDevice->GetID3D11DeviceContext()->ClearState();
    m_pDevice->GetID3D11DeviceContext()->Flush();
    // End of rendering; send to display.
    m_pSwapChain->GetIDXGISwapChain()->Present(0, 0);
  }
};

std::shared_ptr<ISample>
CreateSample_D3D11DrawingContext(std::shared_ptr<DXGISwapChain> swapchain,
                                 std::shared_ptr<Direct3D11Device> device) {
  return std::shared_ptr<ISample>(
      new Sample_D3D11DrawingContext(swapchain, device));
}