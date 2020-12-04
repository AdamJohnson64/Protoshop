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
#include "Core_Util.h"
#include "Scene_Camera.h"
#include <array>
#include <atlbase.h>
#include <functional>

std::function<void(ID3D11RenderTargetView *)>
CreateSample_D3D11Mesh(std::shared_ptr<Direct3D11Device> device) {
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
  CComPtr<ID3D11VertexShader> shaderVertex;
  CComPtr<ID3D11InputLayout> inputLayout;
  {
    CComPtr<ID3DBlob> blobVS = CompileShader("vs_5_0", "mainVS", szShaderCode);
    TRYD3D(device->GetID3D11Device()->CreateVertexShader(
        blobVS->GetBufferPointer(), blobVS->GetBufferSize(), nullptr,
        &shaderVertex));
    {
      D3D11_INPUT_ELEMENT_DESC inputdesc = {};
      inputdesc.SemanticName = "SV_Position";
      inputdesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
      inputdesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
      TRYD3D(device->GetID3D11Device()->CreateInputLayout(
          &inputdesc, 1, blobVS->GetBufferPointer(), blobVS->GetBufferSize(),
          &inputLayout));
    }
  }
  CComPtr<ID3D11PixelShader> shaderPixel;
  {
    CComPtr<ID3DBlob> blobPS = CompileShader("ps_5_0", "mainPS", szShaderCode);
    TRYD3D(device->GetID3D11Device()->CreatePixelShader(
        blobPS->GetBufferPointer(), blobPS->GetBufferSize(), nullptr,
        &shaderPixel));
  }
  CComPtr<ID3D11Buffer> bufferConstants = D3D11_Create_Buffer(
      device->GetID3D11Device(), D3D11_BIND_CONSTANT_BUFFER, sizeof(Matrix44));
  // Create a vertex buffer.
  CComPtr<ID3D11Buffer> bufferVertex;
  {
    Vector3 vertices[] = {{-10, 0, 10}, {10, 0, 10},   {10, 0, -10},
                          {10, 0, -10}, {-10, 0, -10}, {-10, 0, 10}};
    bufferVertex =
        D3D11_Create_Buffer(device->GetID3D11Device(), D3D11_BIND_VERTEX_BUFFER,
                            sizeof(vertices), vertices);
  }
  return [=](ID3D11RenderTargetView *rtvBackbuffer) {
    device->GetID3D11DeviceContext()->ClearState();
    // Beginning of rendering.
    device->GetID3D11DeviceContext()->ClearRenderTargetView(
        rtvBackbuffer, &std::array<FLOAT, 4>{0.1f, 0.1f, 0.1f, 1.0f}[0]);
    device->GetID3D11DeviceContext()->RSSetViewports(
        1, &Make_D3D11_VIEWPORT(RENDERTARGET_WIDTH, RENDERTARGET_HEIGHT));
    device->GetID3D11DeviceContext()->OMSetRenderTargets(1, &rtvBackbuffer,
                                                         nullptr);
    // Update constant buffer.
    {
      char constants[1024];
      memcpy(constants, &GetCameraWorldToClip(), sizeof(Matrix44));
      device->GetID3D11DeviceContext()->UpdateSubresource(
          bufferConstants, 0, nullptr, constants, 0, 0);
    }
    device->GetID3D11DeviceContext()->VSSetShader(shaderVertex, nullptr, 0);
    device->GetID3D11DeviceContext()->VSSetConstantBuffers(0, 1,
                                                           &bufferConstants.p);
    device->GetID3D11DeviceContext()->PSSetShader(shaderPixel, nullptr, 0);
    device->GetID3D11DeviceContext()->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    device->GetID3D11DeviceContext()->IASetInputLayout(inputLayout);
    {
      UINT uStrides[] = {sizeof(Vector3)};
      UINT uOffsets[] = {0};
      device->GetID3D11DeviceContext()->IASetVertexBuffers(
          0, 1, &bufferVertex.p, uStrides, uOffsets);
    }
    device->GetID3D11DeviceContext()->Draw(6, 0);
    device->GetID3D11DeviceContext()->ClearState();
    device->GetID3D11DeviceContext()->Flush();
  };
}
