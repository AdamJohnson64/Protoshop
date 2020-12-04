///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Normal Map
///////////////////////////////////////////////////////////////////////////////
// This sample demonstrates normal mapped rendering.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D11Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_ITransformSource.h"
#include "Core_Math.h"
#include "Core_Util.h"
#include "ImageUtil.h"
#include <array>
#include <atlbase.h>
#include <functional>

std::function<void(ID3D11Texture2D *)>
CreateSample_D3D11NormalMap(std::shared_ptr<Direct3D11Device> device) {

  __declspec(align(16)) struct Constants {
    Matrix44 TransformWorldToClip;
    Vector3 Light;
  };
  struct VertexFormat {
    Vector3 Position;
    Vector3 Normal;
    Vector2 Texcoord;
    Vector3 Tangent;
    Vector3 Bitangent;
  };
  CComPtr<ID3D11SamplerState> samplerState;
  TRYD3D(device->GetID3D11Device()->CreateSamplerState(
      &Make_D3D11_SAMPLER_DESC_DefaultWrap(), &samplerState.p));
  CComPtr<ID3D11Buffer> bufferConstants = D3D11_Create_Buffer(
      device->GetID3D11Device(), D3D11_BIND_CONSTANT_BUFFER, sizeof(Constants));
  const char *szShaderCode = R"SHADER(
cbuffer Constants
{
    float4x4 TransformWorldToClip;
    float3 Light;
};

Texture2D<float4> TextureAlbedoMap;
Texture2D<float4> TextureNormalMap;
sampler userSampler;

struct Vertex
{
    float4 Position : SV_Position;
    float3 Normal : NORMAL;
    float2 Texcoord : TEXCOORD;
    float3 Tangent : TANGENT;
    float3 Bitangent : BITANGENT;
};

Vertex mainVS(Vertex vin)
{
    Vertex vout;
    vout.Position = mul(TransformWorldToClip, vin.Position);
    vout.Normal = vin.Normal;
    vout.Texcoord = vin.Texcoord * 10;
    vout.Tangent = vin.Tangent;
    vout.Bitangent = vin.Bitangent;
    return vout;
}

float4 mainPS(Vertex vin) : SV_Target
{
    float3 AlbedoMap = TextureAlbedoMap.Sample(userSampler, vin.Texcoord).xyz;
    float3 NormalMap = TextureNormalMap.Sample(userSampler, vin.Texcoord).xyz * 2 - 1;
    float3x3 TangentFrame = float3x3(vin.Tangent, vin.Bitangent, vin.Normal);
    float3 Normal = mul(TangentFrame, NormalMap);
    return float4(AlbedoMap * dot(Normal, Light), 1);
})SHADER";
  CComPtr<ID3D11VertexShader> shaderVertex;
  CComPtr<ID3D11InputLayout> inputLayout;
  {
    CComPtr<ID3DBlob> blobVS = CompileShader("vs_5_0", "mainVS", szShaderCode);
    TRYD3D(device->GetID3D11Device()->CreateVertexShader(
        blobVS->GetBufferPointer(), blobVS->GetBufferSize(), nullptr,
        &shaderVertex));
    {
      std::array<D3D11_INPUT_ELEMENT_DESC, 5> inputdesc = {};
      inputdesc[0].SemanticName = "SV_Position";
      inputdesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
      inputdesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
      inputdesc[0].AlignedByteOffset = offsetof(VertexFormat, Position);
      inputdesc[1].SemanticName = "NORMAL";
      inputdesc[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
      inputdesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
      inputdesc[1].AlignedByteOffset = offsetof(VertexFormat, Normal);
      inputdesc[2].SemanticName = "TEXCOORD";
      inputdesc[2].Format = DXGI_FORMAT_R32G32_FLOAT;
      inputdesc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
      inputdesc[2].AlignedByteOffset = offsetof(VertexFormat, Texcoord);
      inputdesc[3].SemanticName = "TANGENT";
      inputdesc[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
      inputdesc[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
      inputdesc[3].AlignedByteOffset = offsetof(VertexFormat, Tangent);
      inputdesc[4].SemanticName = "BITANGENT";
      inputdesc[4].Format = DXGI_FORMAT_R32G32B32_FLOAT;
      inputdesc[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
      inputdesc[4].AlignedByteOffset = offsetof(VertexFormat, Bitangent);
      TRYD3D(device->GetID3D11Device()->CreateInputLayout(
          &inputdesc[0], 5, blobVS->GetBufferPointer(), blobVS->GetBufferSize(),
          &inputLayout));
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
  CComPtr<ID3D11ShaderResourceView> srvAlbedoMap;
  {
    CComPtr<ID3D11Texture2D> textureAlbedoMap;
    struct Pixel {
      uint8_t B, G, R, A;
    };
    const uint32_t imageWidth = 256;
    const uint32_t imageHeight = 256;
    const uint32_t imageStride = sizeof(Pixel) * imageWidth;
    Pixel imageRaw[imageWidth * imageHeight];
    Image_Fill_BrickAlbedo(imageRaw, imageWidth, imageHeight, imageStride);
    textureAlbedoMap = D3D11_Create_Texture2D(
        device->GetID3D11Device(), DXGI_FORMAT_B8G8R8A8_UNORM, imageWidth,
        imageHeight, imageRaw);
    TRYD3D(device->GetID3D11Device()->CreateShaderResourceView(
        textureAlbedoMap,
        &Make_D3D11_SHADER_RESOURCE_VIEW_DESC_Texture2D(
            DXGI_FORMAT_B8G8R8A8_UNORM),
        &srvAlbedoMap.p));
  }
  ////////////////////////////////////////////////////////////////////////////////
  // Create the normal map.
  CComPtr<ID3D11ShaderResourceView> srvNormalMap;
  {
    CComPtr<ID3D11Texture2D> textureNormalMap;
    struct Pixel {
      uint8_t B, G, R, A;
    };
    const uint32_t imageWidth = 256;
    const uint32_t imageHeight = 256;
    const uint32_t imageStride = sizeof(Pixel) * imageWidth;
    Pixel imageRaw[imageWidth * imageHeight];
    Image_Fill_BrickNormal(imageRaw, imageWidth, imageHeight, imageStride);
    textureNormalMap = D3D11_Create_Texture2D(
        device->GetID3D11Device(), DXGI_FORMAT_B8G8R8A8_UNORM, imageWidth,
        imageHeight, imageRaw);
    TRYD3D(device->GetID3D11Device()->CreateShaderResourceView(
        textureNormalMap,
        &Make_D3D11_SHADER_RESOURCE_VIEW_DESC_Texture2D(
            DXGI_FORMAT_B8G8R8A8_UNORM),
        &srvNormalMap.p));
  }
  ////////////////////////////////////////////////////////////////////////////////
  // Create a vertex buffer.
  CComPtr<ID3D11Buffer> bufferVertex;
  {
    VertexFormat vertices[] = {
        // clang format off
        {{-10, 0, 10}, {0, 1, 0}, {0, 0}, {1, 0, 0}, {0, 0, 1}},
        {{10, 0, 10}, {0, 1, 0}, {1, 0}, {1, 0, 0}, {0, 0, 1}},
        {{10, 0, -10}, {0, 1, 0}, {1, 1}, {1, 0, 0}, {0, 0, 1}},
        {{10, 0, -10}, {0, 1, 0}, {1, 1}, {1, 0, 0}, {0, 0, 1}},
        {{-10, 0, -10}, {0, 1, 0}, {0, 1}, {1, 0, 0}, {0, 0, 1}},
        {{-10, 0, 10}, {0, 1, 0}, {0, 0}, {1, 0, 0}, {0, 0, 1}},
        // clang format on
    };
    bufferVertex =
        D3D11_Create_Buffer(device->GetID3D11Device(), D3D11_BIND_VERTEX_BUFFER,
                            sizeof(vertices), vertices);
  }
  return [=](ID3D11Texture2D *textureBackbuffer) {
    CComPtr<ID3D11RenderTargetView> rtvBackbuffer =
        D3D11_Create_RTV_From_Texture2D(device->GetID3D11Device(),
                                        textureBackbuffer);
    device->GetID3D11DeviceContext()->ClearState();
    ////////////////////////////////////////////////////////////////////////////////
    // Beginning of rendering.
    device->GetID3D11DeviceContext()->ClearRenderTargetView(
        rtvBackbuffer, &std::array<FLOAT, 4>{0.1f, 0.1f, 0.1f, 1.0f}[0]);
    device->GetID3D11DeviceContext()->RSSetViewports(
        1, &Make_D3D11_VIEWPORT(RENDERTARGET_WIDTH, RENDERTARGET_HEIGHT));
    device->GetID3D11DeviceContext()->OMSetRenderTargets(1, &rtvBackbuffer.p,
                                                         nullptr);
    ////////////////////////////////////////////////////////////////////////////////
    // Update constant buffer.
    {
      static float angle = 0;
      Constants constants = {};
      constants.TransformWorldToClip =
          GetTransformSource()->GetTransformWorldToClip();
      constants.Light = Normalize(Vector3{sinf(angle), 1, -cosf(angle)});
      angle += 0.05f;
      device->GetID3D11DeviceContext()->UpdateSubresource(
          bufferConstants, 0, nullptr, &constants, 0, 0);
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
      UINT uStrides[] = {sizeof(VertexFormat)};
      UINT uOffsets[] = {0};
      device->GetID3D11DeviceContext()->IASetVertexBuffers(
          0, 1, &bufferVertex.p, uStrides, uOffsets);
    }
    device->GetID3D11DeviceContext()->Draw(6, 0);
    device->GetID3D11DeviceContext()->ClearState();
    device->GetID3D11DeviceContext()->Flush();
  };
}
