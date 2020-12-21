///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Normal Map
///////////////////////////////////////////////////////////////////////////////
// This sample demonstrates normal mapped rendering.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D11Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_Math.h"
#include "Core_Util.h"
#include "ImageUtil.h"
#include "SampleResources.h"
#include <array>
#include <atlbase.h>
#include <functional>

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11NormalMap(std::shared_ptr<Direct3D11Device> device) {

  CComPtr<ID3D11SamplerState> samplerState;
  TRYD3D(device->GetID3D11Device()->CreateSamplerState(
      &Make_D3D11_SAMPLER_DESC_DefaultWrap(), &samplerState.p));
  CComPtr<ID3D11Buffer> bufferConstants =
      D3D11_Create_Buffer(device->GetID3D11Device(), D3D11_BIND_CONSTANT_BUFFER,
                          sizeof(ConstantsWorld));
  const char *szShaderCode = R"SHADER(
#include "Sample_D3D11_Common.inc"

float4 mainPS(VertexPS vin) : SV_Target
{
    float3x3 matTangentFrame = cotangent_frame(vin.Normal, vin.WorldPosition, vin.Texcoord);    
    float3 texelNormal = TextureNormalMap.Sample(userSampler, vin.Texcoord).xyz * 2 - 1;
    float3 vectorNormal = normalize(mul(texelNormal, matTangentFrame));
    
    float3 texelAlbedoMap = TextureAlbedoMap.Sample(userSampler, vin.Texcoord).xyz;
    return float4(texelAlbedoMap * dot(vectorNormal, LightPosition), 1);
})SHADER";
  CComPtr<ID3D11VertexShader> shaderVertex;
  CComPtr<ID3D11InputLayout> inputLayout;
  {
    CComPtr<ID3DBlob> blobVS =
        CompileShader("vs_5_0", "mainVS_NOOBJTRANSFORM", szShaderCode);
    TRYD3D(device->GetID3D11Device()->CreateVertexShader(
        blobVS->GetBufferPointer(), blobVS->GetBufferSize(), nullptr,
        &shaderVertex));
    {
      std::array<D3D11_INPUT_ELEMENT_DESC, 3> inputdesc = {};
      inputdesc[0].SemanticName = "SV_Position";
      inputdesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
      inputdesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
      inputdesc[0].AlignedByteOffset = offsetof(VertexVS, Position);
      inputdesc[1].SemanticName = "NORMAL";
      inputdesc[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
      inputdesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
      inputdesc[1].AlignedByteOffset = offsetof(VertexVS, Normal);
      inputdesc[2].SemanticName = "TEXCOORD";
      inputdesc[2].Format = DXGI_FORMAT_R32G32_FLOAT;
      inputdesc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
      inputdesc[2].AlignedByteOffset = offsetof(VertexVS, Texcoord);
      TRYD3D(device->GetID3D11Device()->CreateInputLayout(
          &inputdesc[0], inputdesc.size(), blobVS->GetBufferPointer(),
          blobVS->GetBufferSize(), &inputLayout));
    }
  }
  CComPtr<ID3D11PixelShader> shaderPixel;
  {
    ID3DBlob *blobPS = CompileShader("ps_5_0", "mainPS", szShaderCode);
    TRYD3D(device->GetID3D11Device()->CreatePixelShader(
        blobPS->GetBufferPointer(), blobPS->GetBufferSize(), nullptr,
        &shaderPixel));
  }
  ////////////////////////////////////////////////////////////////////////////////
  // Create the albedo map.
  CComPtr<ID3D11ShaderResourceView> srvAlbedoMap = D3D11_Create_SRV(
      device->GetID3D11DeviceContext(), Image_BrickAlbedo(256, 256).get());
  ////////////////////////////////////////////////////////////////////////////////
  // Create the normal map.
  CComPtr<ID3D11ShaderResourceView> srvNormalMap = D3D11_Create_SRV(
      device->GetID3D11DeviceContext(), Image_BrickNormal(256, 256).get());
  ////////////////////////////////////////////////////////////////////////////////
  // Create a vertex buffer.
  CComPtr<ID3D11Buffer> bufferVertex;
  {
    VertexVS vertices[] = {
        // clang format off
        {{-10, 0, 10}, {0, 1, 0}, {0, 0}},  {{10, 0, 10}, {0, 1, 0}, {1, 0}},
        {{10, 0, -10}, {0, 1, 0}, {1, 1}},  {{10, 0, -10}, {0, 1, 0}, {1, 1}},
        {{-10, 0, -10}, {0, 1, 0}, {0, 1}}, {{-10, 0, 10}, {0, 1, 0}, {0, 0}},
        // clang format on
    };
    bufferVertex =
        D3D11_Create_Buffer(device->GetID3D11Device(), D3D11_BIND_VERTEX_BUFFER,
                            sizeof(vertices), vertices);
  }
  return [=](const SampleResourcesD3D11 &sampleResources) {
    D3D11_TEXTURE2D_DESC descBackbuffer = {};
    sampleResources.BackBufferTexture->GetDesc(&descBackbuffer);
    CComPtr<ID3D11RenderTargetView> rtvBackbuffer =
        D3D11_Create_RTV_From_Texture2D(device->GetID3D11Device(),
                                        sampleResources.BackBufferTexture);
    device->GetID3D11DeviceContext()->ClearState();
    ////////////////////////////////////////////////////////////////////////////////
    // Beginning of rendering.
    device->GetID3D11DeviceContext()->ClearRenderTargetView(
        rtvBackbuffer, &std::array<FLOAT, 4>{0.1f, 0.1f, 0.1f, 1.0f}[0]);
    device->GetID3D11DeviceContext()->ClearDepthStencilView(
        sampleResources.DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
    device->GetID3D11DeviceContext()->RSSetViewports(
        1, &Make_D3D11_VIEWPORT(descBackbuffer.Width, descBackbuffer.Height));
    device->GetID3D11DeviceContext()->OMSetRenderTargets(
        1, &rtvBackbuffer.p, sampleResources.DepthStencilView);
    ////////////////////////////////////////////////////////////////////////////////
    // Update constant buffer.
    {
      static float angle = 0;
      ConstantsWorld data = {};
      data.TransformWorldToClip = sampleResources.TransformWorldToClip;
      data.LightPosition = Normalize(Vector3{sinf(angle), 1, -cosf(angle)});
      angle += 0.05f;
      device->GetID3D11DeviceContext()->UpdateSubresource(bufferConstants, 0,
                                                          nullptr, &data, 0, 0);
    }
    ////////////////////////////////////////////////////////////////////////////////
    // Setup and draw.
    device->GetID3D11DeviceContext()->VSSetShader(shaderVertex, nullptr, 0);
    device->GetID3D11DeviceContext()->VSSetConstantBuffers(0, 1,
                                                           &bufferConstants.p);
    device->GetID3D11DeviceContext()->PSSetShader(shaderPixel, nullptr, 0);
    device->GetID3D11DeviceContext()->PSSetConstantBuffers(0, 1,
                                                           &bufferConstants.p);
    device->GetID3D11DeviceContext()->PSSetSamplers(0, 1, &samplerState.p);
    device->GetID3D11DeviceContext()->PSSetShaderResources(0, 1,
                                                           &srvAlbedoMap.p);
    device->GetID3D11DeviceContext()->PSSetShaderResources(1, 1,
                                                           &srvNormalMap.p);
    device->GetID3D11DeviceContext()->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    device->GetID3D11DeviceContext()->IASetInputLayout(inputLayout);
    {
      UINT uStrides[] = {sizeof(VertexVS)};
      UINT uOffsets[] = {0};
      device->GetID3D11DeviceContext()->IASetVertexBuffers(
          0, 1, &bufferVertex.p, uStrides, uOffsets);
    }
    device->GetID3D11DeviceContext()->Draw(6, 0);
    device->GetID3D11DeviceContext()->ClearState();
    device->GetID3D11DeviceContext()->Flush();
  };
}
