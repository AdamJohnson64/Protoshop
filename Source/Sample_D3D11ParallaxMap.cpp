///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Parallax Map
///////////////////////////////////////////////////////////////////////////////
// BAD MATH
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

std::function<void(ID3D11Texture2D *, ID3D11DepthStencilView *,
                   const Matrix44 &)>
CreateSample_D3D11ParallaxMap(std::shared_ptr<Direct3D11Device> device) {
  __declspec(align(16)) struct Constants {
    Matrix44 TransformWorldToClip;
    Matrix44 TransformWorldToView;
    Vector3 CameraPosition;
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
    float4x4 TransformWorldToView;
    float3 CameraPosition;
};

Texture2D<float4> TextureAlbedoMap : register(t0);
Texture2D<float4> TextureNormalMap : register(t1);
Texture2D<float4> TextureDepthMap : register(t2);
sampler userSampler;

struct VertexVS
{
    float4 Position : SV_Position;
    float3 Normal : NORMAL;
    float2 Texcoord : TEXCOORD;
    float3 Tangent : TANGENT;
    float3 Bitangent : BITANGENT;
};

struct VertexPS
{
    float4 Position : SV_Position;
    float3 Normal : NORMAL;
    float2 Texcoord : TEXCOORD;
    float3 Tangent : TANGENT;
    float3 Bitangent : BITANGENT;
    float3 EyeTBN : TEXCOORD1;
};

VertexPS mainVS(VertexVS vin)
{
    float3x3 tbn = float3x3(vin.Tangent, -vin.Bitangent, vin.Normal);
    VertexPS vout;
    vout.Position = mul(TransformWorldToClip, vin.Position);
    vout.Normal = vin.Normal;
    vout.Texcoord = vin.Texcoord * 10;
    vout.Tangent = vin.Tangent;
    vout.Bitangent = vin.Bitangent;
    vout.EyeTBN = mul(tbn, CameraPosition - vin.Position.xyz);
    return vout;
}

float2 ParallaxMapping(float2 texCoords, float3 viewDir)
{
    float height_scale = 0.2;
    float height = 1 - TextureDepthMap.Sample(userSampler, texCoords).r;
    float2 p = viewDir.xy / viewDir.z * (height * height_scale);
    return texCoords - p;
}

float4 mainPS(VertexPS vin) : SV_Target
{
    float3x3 tbn = float3x3(vin.Tangent, vin.Bitangent, vin.Normal);
    float3 viewer = normalize(vin.EyeTBN.xyz);
    float2 uv = ParallaxMapping(vin.Texcoord, viewer);
    float3 AlbedoMap = TextureAlbedoMap.Sample(userSampler, uv).xyz;
    float3 NormalMap = mul(tbn, TextureNormalMap.Sample(userSampler, uv).xyz * 2 - 1);
    float light = dot(NormalMap, normalize(float3(1, 1, -1)));
    return float4(AlbedoMap * light, 1);
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
        &Make_D3D11_SHADER_RESOURCE_VIEW_DESC_Texture2D(textureAlbedoMap),
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
        &Make_D3D11_SHADER_RESOURCE_VIEW_DESC_Texture2D(textureNormalMap),
        &srvNormalMap.p));
  }
  ////////////////////////////////////////////////////////////////////////////////
  // Create the depth map.
  CComPtr<ID3D11ShaderResourceView> srvDepthMap;
  {
    CComPtr<ID3D11Texture2D> textureDepthMap;
    struct Pixel {
      uint8_t B, G, R, A;
    };
    const uint32_t imageWidth = 256;
    const uint32_t imageHeight = 256;
    const uint32_t imageStride = sizeof(Pixel) * imageWidth;
    Pixel imageRaw[imageWidth * imageHeight];
    Image_Fill_BrickDepth(imageRaw, imageWidth, imageHeight, imageStride);
    textureDepthMap = D3D11_Create_Texture2D(device->GetID3D11Device(),
                                             DXGI_FORMAT_B8G8R8A8_UNORM,
                                             imageWidth, imageHeight, imageRaw);
    TRYD3D(device->GetID3D11Device()->CreateShaderResourceView(
        textureDepthMap,
        &Make_D3D11_SHADER_RESOURCE_VIEW_DESC_Texture2D(textureDepthMap),
        &srvDepthMap.p));
  }
  return [=](ID3D11Texture2D *textureBackbuffer,
             ID3D11DepthStencilView *dsvDepth,
             const Matrix44 &transformWorldToClip) {
    D3D11_TEXTURE2D_DESC descBackbuffer = {};
    textureBackbuffer->GetDesc(&descBackbuffer);
    CComPtr<ID3D11RenderTargetView> rtvBackbuffer =
        D3D11_Create_RTV_From_Texture2D(device->GetID3D11Device(),
                                        textureBackbuffer);
    device->GetID3D11DeviceContext()->ClearState();
    ////////////////////////////////////////////////////////////////////////////////
    // Beginning of rendering.
    device->GetID3D11DeviceContext()->ClearRenderTargetView(
        rtvBackbuffer, &std::array<FLOAT, 4>{0.1f, 0.1f, 0.1f, 1.0f}[0]);
    device->GetID3D11DeviceContext()->ClearDepthStencilView(
        dsvDepth, D3D11_CLEAR_DEPTH, 1.0f, 0);
    device->GetID3D11DeviceContext()->RSSetViewports(
        1, &Make_D3D11_VIEWPORT(descBackbuffer.Width, descBackbuffer.Height));
    device->GetID3D11DeviceContext()->OMSetRenderTargets(1, &rtvBackbuffer.p,
                                                         dsvDepth);
    ////////////////////////////////////////////////////////////////////////////////
    // Update constant buffer.
    {
      Constants constants = {};
      constants.TransformWorldToClip = transformWorldToClip;
      constants.TransformWorldToView =
          GetTransformSource()->GetTransformWorldToView();
      Matrix44 t = Invert(GetTransformSource()->GetTransformWorldToView());
      constants.CameraPosition = Vector3{t.M41, t.M42, t.M43};
      device->GetID3D11DeviceContext()->UpdateSubresource(
          bufferConstants, 0, nullptr, &constants, 0, 0);
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
      bufferVertex = D3D11_Create_Buffer(device->GetID3D11Device(),
                                         D3D11_BIND_VERTEX_BUFFER,
                                         sizeof(vertices), vertices);
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
    device->GetID3D11DeviceContext()->PSSetShaderResources(2, 1,
                                                           &srvDepthMap.p);
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
