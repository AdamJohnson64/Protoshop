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
#include "Core_Math.h"
#include "Core_Util.h"
#include <array>
#include <atlbase.h>
#include <functional>
#include <vector>

std::function<void(ID3D11Texture2D *)>
CreateSample_D3D11DrawingContext(std::shared_ptr<Direct3D11Device> device) {
  ////////////////////////////////////////////////////////////////////////////////
  // Create a vertex shader and matching input layout.
  CComPtr<ID3D11VertexShader> shaderVertex;
  CComPtr<ID3D11InputLayout> inputLayout;
  {
    CComPtr<ID3DBlob> blobVS = CompileShader("vs_5_0", "main", R"SHADER(
float4 main(float4 pos : SV_Position) : SV_Position
{
        return float4(-1 + pos.x * 2 / 320, 1 - pos.y * 2 / 240, 0, 1);
})SHADER");
    TRYD3D(device->GetID3D11Device()->CreateVertexShader(
        blobVS->GetBufferPointer(), blobVS->GetBufferSize(), nullptr,
        &shaderVertex));
    {
      D3D11_INPUT_ELEMENT_DESC desc = {};
      desc.SemanticName = "SV_Position";
      desc.Format = DXGI_FORMAT_R32G32_FLOAT;
      desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
      TRYD3D(device->GetID3D11Device()->CreateInputLayout(
          &desc, 1, blobVS->GetBufferPointer(), blobVS->GetBufferSize(),
          &inputLayout));
    }
  }
  ////////////////////////////////////////////////////////////////////////////////
  // Create a pixel shader.
  CComPtr<ID3D11PixelShader> shaderPixel;
  {
    ID3DBlob *blobPS = CompileShader("ps_5_0", "main", R"SHADER(
float4 main() : SV_Target
{
        return float4(1, 1, 1, 1);
})SHADER");
    TRYD3D(device->GetID3D11Device()->CreatePixelShader(
        blobPS->GetBufferPointer(), blobPS->GetBufferSize(), nullptr,
        &shaderPixel));
  }
  return [=](ID3D11Texture2D *textureBackbuffer) {
    D3D11_TEXTURE2D_DESC descBackbuffer = {};
    textureBackbuffer->GetDesc(&descBackbuffer);
    CComPtr<ID3D11RenderTargetView> rtvBackbuffer =
        D3D11_Create_RTV_From_Texture2D(device->GetID3D11Device(),
                                        textureBackbuffer);
    device->GetID3D11DeviceContext()->ClearState();
    // Beginning of rendering.
    device->GetID3D11DeviceContext()->ClearRenderTargetView(
        rtvBackbuffer, &std::array<FLOAT, 4>{0.1f, 0.1f, 0.1f, 1.0f}[0]);
    device->GetID3D11DeviceContext()->RSSetViewports(
        1, &Make_D3D11_VIEWPORT(descBackbuffer.Width, descBackbuffer.Height));
    device->GetID3D11DeviceContext()->OMSetRenderTargets(1, &rtvBackbuffer.p,
                                                         nullptr);
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
          device->GetID3D11Device(), D3D11_BIND_VERTEX_BUFFER,
          sizeof(Vector2) * vertices.size(), &vertices[0]);
      {
        UINT uStrides[] = {sizeof(Vector2)};
        UINT uOffsets[] = {0};
        device->GetID3D11DeviceContext()->IASetVertexBuffers(
            0, 1, &pD3D11BufferVertex.p, uStrides, uOffsets);
      }
      device->GetID3D11DeviceContext()->Draw(vertices.size(), 0);
      device->GetID3D11DeviceContext()->Flush();
      vertices.clear();
    };
    // Beginning of rendering.
    device->GetID3D11DeviceContext()->VSSetShader(shaderVertex, nullptr, 0);
    device->GetID3D11DeviceContext()->PSSetShader(shaderPixel, nullptr, 0);
    device->GetID3D11DeviceContext()->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    device->GetID3D11DeviceContext()->IASetInputLayout(inputLayout);
    static float angle = 0;
    for (int i = 0; i < 10; ++i) {
      const float angleThis = angle + i * 0.1f;
      DCDrawLine({160, 120},
                 {160 + sinf(angleThis) * 100, 120 - cosf(angleThis) * 100});
    }
    angle += 0.01f;
    DCFlush();
    device->GetID3D11DeviceContext()->ClearState();
    device->GetID3D11DeviceContext()->Flush();
  };
}