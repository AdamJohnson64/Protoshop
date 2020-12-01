///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Mesh
///////////////////////////////////////////////////////////////////////////////
// This sample demonstrates how to render a triangle using the simplest shader
// setups and a common camera.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D11Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_ISample.h"
#include "Core_Object.h"
#include "Core_Util.h"
#include "Scene_Camera.h"
#include <array>
#include <atlbase.h>

class Sample_D3D11Mesh : public Object, public ISample {
private:
  std::shared_ptr<DXGISwapChain> m_pSwapChain;
  std::shared_ptr<Direct3D11Device> m_pDevice;
  CComPtr<ID3D11Buffer> m_pD3D11BufferConstants;
  CComPtr<ID3D11VertexShader> m_pD3D11VertexShader;
  CComPtr<ID3D11PixelShader> m_pD3D11PixelShader;
  CComPtr<ID3D11InputLayout> m_pD3D11InputLayout;

public:
  Sample_D3D11Mesh(std::shared_ptr<DXGISwapChain> swapchain,
                   std::shared_ptr<Direct3D11Device> device)
      : m_pSwapChain(swapchain), m_pDevice(device) {
    m_pD3D11BufferConstants =
        D3D11_Create_Buffer(m_pDevice->GetID3D11Device(),
                            D3D11_BIND_CONSTANT_BUFFER, sizeof(Matrix44));
    const char *szShaderCode = R"SHADER(
cbuffer Constants
{
    float4x4 TransformWorldToClip;
};

float4 mainVS(float4 pos : SV_Position) : SV_Position
{
    return mul(TransformWorldToClip, pos);
}

float4 mainPS() : SV_Target
{
    return float4(1, 1, 1, 1);
})SHADER";
    CComPtr<ID3DBlob> pD3DBlobCodeVS =
        CompileShader("vs_5_0", "mainVS", szShaderCode);
    TRYD3D(m_pDevice->GetID3D11Device()->CreateVertexShader(
        pD3DBlobCodeVS->GetBufferPointer(), pD3DBlobCodeVS->GetBufferSize(),
        nullptr, &m_pD3D11VertexShader));
    ID3DBlob *pD3DBlobCodePS = CompileShader("ps_5_0", "mainPS", szShaderCode);
    TRYD3D(m_pDevice->GetID3D11Device()->CreatePixelShader(
        pD3DBlobCodePS->GetBufferPointer(), pD3DBlobCodePS->GetBufferSize(),
        nullptr, &m_pD3D11PixelShader));
    {
      D3D11_INPUT_ELEMENT_DESC inputdesc = {};
      inputdesc.SemanticName = "SV_Position";
      inputdesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
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
    // Update constant buffer.
    {
      char constants[1024];
      memcpy(constants, &GetCameraWorldToClip(), sizeof(Matrix44));
      m_pDevice->GetID3D11DeviceContext()->UpdateSubresource(
          m_pD3D11BufferConstants, 0, nullptr, constants, 0, 0);
    }
    // Create a vertex buffer.
    CComPtr<ID3D11Buffer> m_pD3D11BufferVertex;
    {
      Vector3 vertices[] = {{-10, 0, 10}, {10, 0, 10},   {10, 0, -10},
                            {10, 0, -10}, {-10, 0, -10}, {-10, 0, 10}};
      m_pD3D11BufferVertex = D3D11_Create_Buffer(m_pDevice->GetID3D11Device(),
                                                 D3D11_BIND_VERTEX_BUFFER,
                                                 sizeof(vertices), vertices);
    }
    m_pDevice->GetID3D11DeviceContext()->VSSetShader(m_pD3D11VertexShader,
                                                     nullptr, 0);
    m_pDevice->GetID3D11DeviceContext()->VSSetConstantBuffers(
        0, 1, &m_pD3D11BufferConstants.p);
    m_pDevice->GetID3D11DeviceContext()->PSSetShader(m_pD3D11PixelShader,
                                                     nullptr, 0);
    m_pDevice->GetID3D11DeviceContext()->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pDevice->GetID3D11DeviceContext()->IASetInputLayout(m_pD3D11InputLayout);
    {
      UINT uStrides[] = {sizeof(Vector3)};
      UINT uOffsets[] = {0};
      m_pDevice->GetID3D11DeviceContext()->IASetVertexBuffers(
          0, 1, &m_pD3D11BufferVertex.p, uStrides, uOffsets);
    }
    m_pDevice->GetID3D11DeviceContext()->Draw(6, 0);
    m_pDevice->GetID3D11DeviceContext()->ClearState();
    m_pDevice->GetID3D11DeviceContext()->Flush();
    // End of rendering; send to display.
    m_pSwapChain->GetIDXGISwapChain()->Present(0, 0);
  }
};

std::shared_ptr<ISample>
CreateSample_D3D11Mesh(std::shared_ptr<DXGISwapChain> swapchain,
                       std::shared_ptr<Direct3D11Device> device) {
  return std::shared_ptr<ISample>(new Sample_D3D11Mesh(swapchain, device));
}