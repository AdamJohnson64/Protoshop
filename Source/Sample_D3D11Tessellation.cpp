///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Tessellation
///////////////////////////////////////////////////////////////////////////////
// This sample demonstrates a basic tessellator setup and renders a tesselation
// amplified triangle.
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

std::function<void(ID3D11Texture2D *)>
CreateSample_D3D11Tessellation(std::shared_ptr<Direct3D11Device> device) {
  CComPtr<ID3D11RasterizerState> rasterizerState;
  {
    D3D11_RASTERIZER_DESC desc = {};
    desc.CullMode = D3D11_CULL_NONE;
    desc.FillMode = D3D11_FILL_WIREFRAME;
    device->GetID3D11Device()->CreateRasterizerState(&desc, &rasterizerState);
  }
  CComPtr<ID3D11VertexShader> shaderVertex;
  CComPtr<ID3D11HullShader> shaderHull;
  CComPtr<ID3D11DomainShader> shaderDomain;
  CComPtr<ID3D11PixelShader> shaderPixel;
  CComPtr<ID3D11InputLayout> inputLayout;
  {
    const char *szShaderCode = R"SHADER(
struct ControlPoint
{
    float2 vPosition : CONTROLPOINT;
};

struct PatchConstants
{
    float Edges[3] : SV_TessFactor;
    float Inside : SV_InsideTessFactor;
};

struct VertexOutput
{
    float4 vPosition : SV_Position;
};

ControlPoint mainVS(ControlPoint controlpoint)
{
    return controlpoint;
}

PatchConstants mainHSConst(InputPatch<ControlPoint, 3> patch)
{   
    PatchConstants Output;
    Output.Edges[2] = Output.Edges[1] = Output.Edges[0] = 10;
    Output.Inside = 10;
    return Output;
}

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("mainHSConst")]
ControlPoint mainHS(InputPatch<ControlPoint, 3> patch, uint i : SV_OutputControlPointID)
{
    return patch[i];
}

[domain("tri")]
VertexOutput mainDS(
    PatchConstants patchconstants, 
    float3 bary : SV_DomainLocation,
    const OutputPatch<ControlPoint, 3> patch)
{
    VertexOutput Output;
    Output.vPosition = float4(patch[0].vPosition * bary.x + patch[1].vPosition * bary.y + patch[2].vPosition * bary.z, 0, 1);
    return Output;    
}

float4 mainPS() : SV_Target
{
        return float4(1, 1, 1, 1);
})SHADER";
    CComPtr<ID3DBlob> blobVS = CompileShader("vs_5_0", "mainVS", szShaderCode);
    TRYD3D(device->GetID3D11Device()->CreateVertexShader(
        blobVS->GetBufferPointer(), blobVS->GetBufferSize(), nullptr,
        &shaderVertex));
    CComPtr<ID3DBlob> blobHS = CompileShader("hs_5_0", "mainHS", szShaderCode);
    TRYD3D(device->GetID3D11Device()->CreateHullShader(
        blobHS->GetBufferPointer(), blobHS->GetBufferSize(), nullptr,
        &shaderHull));
    CComPtr<ID3DBlob> blobDS = CompileShader("ds_5_0", "mainDS", szShaderCode);
    TRYD3D(device->GetID3D11Device()->CreateDomainShader(
        blobDS->GetBufferPointer(), blobDS->GetBufferSize(), nullptr,
        &shaderDomain));
    CComPtr<ID3DBlob> blobPS = CompileShader("ps_5_0", "mainPS", szShaderCode);
    TRYD3D(device->GetID3D11Device()->CreatePixelShader(
        blobPS->GetBufferPointer(), blobPS->GetBufferSize(), nullptr,
        &shaderPixel));
    ////////////////////////////////////////////////////////////////////////////////
    // Create an input layout.
    {
      D3D11_INPUT_ELEMENT_DESC desc = {};
      desc.SemanticName = "CONTROLPOINT";
      desc.Format = DXGI_FORMAT_R32G32_FLOAT;
      desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
      TRYD3D(device->GetID3D11Device()->CreateInputLayout(
          &desc, 1, blobVS->GetBufferPointer(), blobVS->GetBufferSize(),
          &inputLayout));
    }
  }
  ////////////////////////////////////////////////////////////////////////////////
  // Create a vertex buffer.
  CComPtr<ID3D11Buffer> bufferVertex;
  {
    Vector2 vertices[] = {{0, 0}, {0, 1}, {1, 0}};
    bufferVertex =
        D3D11_Create_Buffer(device->GetID3D11Device(), D3D11_BIND_VERTEX_BUFFER,
                            sizeof(vertices), vertices);
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
    device->GetID3D11DeviceContext()->VSSetShader(shaderVertex, nullptr, 0);
    device->GetID3D11DeviceContext()->HSSetShader(shaderHull, nullptr, 0);
    device->GetID3D11DeviceContext()->DSSetShader(shaderDomain, nullptr, 0);
    device->GetID3D11DeviceContext()->RSSetState(rasterizerState);
    device->GetID3D11DeviceContext()->PSSetShader(shaderPixel, nullptr, 0);
    device->GetID3D11DeviceContext()->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
    device->GetID3D11DeviceContext()->IASetInputLayout(inputLayout);
    {
      UINT uStrides[] = {sizeof(Vector2)};
      UINT uOffsets[] = {0};
      device->GetID3D11DeviceContext()->IASetVertexBuffers(
          0, 1, &bufferVertex.p, uStrides, uOffsets);
    }
    device->GetID3D11DeviceContext()->Draw(3, 0);
    device->GetID3D11DeviceContext()->ClearState();
    device->GetID3D11DeviceContext()->Flush();
  };
}