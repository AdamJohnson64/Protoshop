///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Parallax Mapping
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D11Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_Util.h"
#include "ImageUtil.h"
#include "Sample.h"
#include "Scene_Camera.h"
#include <array>
#include <atlbase.h>

class Sample_D3D11ParallaxMap : public Sample {
private:
  std::shared_ptr<DXGISwapChain> m_pSwapChain;
  std::shared_ptr<Direct3D11Device> m_pDevice;
  CComPtr<ID3D11SamplerState> m_pD3D11SamplerState;
  CComPtr<ID3D11Buffer> m_pD3D11BufferConstants;
  CComPtr<ID3D11VertexShader> m_pD3D11VertexShader;
  CComPtr<ID3D11PixelShader> m_pD3D11PixelShader;
  CComPtr<ID3D11InputLayout> m_pD3D11InputLayout;
  CComPtr<ID3D11Texture2D> m_pTex2DAlbedoMap;
  CComPtr<ID3D11ShaderResourceView> m_pSRVAlbedoMap;
  CComPtr<ID3D11Texture2D> m_pTex2DNormalMap;
  CComPtr<ID3D11ShaderResourceView> m_pSRVNormalMap;
  CComPtr<ID3D11Texture2D> m_pTex2DDepthMap;
  CComPtr<ID3D11ShaderResourceView> m_pSRVDepthMap;
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

public:
  Sample_D3D11ParallaxMap(std::shared_ptr<DXGISwapChain> swapchain,
                          std::shared_ptr<Direct3D11Device> device)
      : m_pSwapChain(swapchain), m_pDevice(device) {
    TRYD3D(m_pDevice->GetID3D11Device()->CreateSamplerState(
        &Make_D3D11_SAMPLER_DESC_DefaultWrap(), &m_pD3D11SamplerState.p));
    m_pD3D11BufferConstants =
        D3D11_Create_Buffer(m_pDevice->GetID3D11Device(),
                            D3D11_BIND_CONSTANT_BUFFER, sizeof(Constants));
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
      TRYD3D(m_pDevice->GetID3D11Device()->CreateInputLayout(
          &inputdesc[0], 5, pD3DBlobCodeVS->GetBufferPointer(),
          pD3DBlobCodeVS->GetBufferSize(), &m_pD3D11InputLayout));
    }
    ////////////////////////////////////////////////////////////////////////////////
    // Create the albedo map.
    {
      struct Pixel {
        uint8_t B, G, R, A;
      };
      const uint32_t imageWidth = 256;
      const uint32_t imageHeight = 256;
      const uint32_t imageStride = sizeof(Pixel) * imageWidth;
      Pixel imageRaw[imageWidth * imageHeight];
      Image_Fill_BrickAlbedo(imageRaw, imageWidth, imageHeight, imageStride);
      m_pTex2DAlbedoMap = D3D11_Create_Texture2D(
          m_pDevice->GetID3D11Device(), DXGI_FORMAT_B8G8R8A8_UNORM, imageWidth,
          imageHeight, imageRaw);
    }
    TRYD3D(m_pDevice->GetID3D11Device()->CreateShaderResourceView(
        m_pTex2DAlbedoMap,
        &Make_D3D11_SHADER_RESOURCE_VIEW_DESC_Texture2D(
            DXGI_FORMAT_B8G8R8A8_UNORM),
        &m_pSRVAlbedoMap.p));
    ////////////////////////////////////////////////////////////////////////////////
    // Create the normal map.
    {
      struct Pixel {
        uint8_t B, G, R, A;
      };
      const uint32_t imageWidth = 256;
      const uint32_t imageHeight = 256;
      const uint32_t imageStride = sizeof(Pixel) * imageWidth;
      Pixel imageRaw[imageWidth * imageHeight];
      Image_Fill_BrickNormal(imageRaw, imageWidth, imageHeight, imageStride);
      m_pTex2DNormalMap = D3D11_Create_Texture2D(
          m_pDevice->GetID3D11Device(), DXGI_FORMAT_B8G8R8A8_UNORM, imageWidth,
          imageHeight, imageRaw);
    }
    TRYD3D(m_pDevice->GetID3D11Device()->CreateShaderResourceView(
        m_pTex2DNormalMap,
        &Make_D3D11_SHADER_RESOURCE_VIEW_DESC_Texture2D(
            DXGI_FORMAT_B8G8R8A8_UNORM),
        &m_pSRVNormalMap.p));
    ////////////////////////////////////////////////////////////////////////////////
    // Create the depth map.
    {
      struct Pixel {
        uint8_t B, G, R, A;
      };
      const uint32_t imageWidth = 256;
      const uint32_t imageHeight = 256;
      const uint32_t imageStride = sizeof(Pixel) * imageWidth;
      Pixel imageRaw[imageWidth * imageHeight];
      Image_Fill_BrickDepth(imageRaw, imageWidth, imageHeight, imageStride);
      m_pTex2DDepthMap = D3D11_Create_Texture2D(
          m_pDevice->GetID3D11Device(), DXGI_FORMAT_B8G8R8A8_UNORM, imageWidth,
          imageHeight, imageRaw);
    }
    TRYD3D(m_pDevice->GetID3D11Device()->CreateShaderResourceView(
        m_pTex2DDepthMap,
        &Make_D3D11_SHADER_RESOURCE_VIEW_DESC_Texture2D(
            DXGI_FORMAT_B8G8R8A8_UNORM),
        &m_pSRVDepthMap.p));
  }
  void Render() override {
    ////////////////////////////////////////////////////////////////////////////////
    // Get the backbuffer and create a render target from it.
    CComPtr<ID3D11RenderTargetView> pD3D11RenderTargetView =
        D3D11_Create_RTV_From_SwapChain(m_pDevice->GetID3D11Device(),
                                        m_pSwapChain->GetIDXGISwapChain());
    m_pDevice->GetID3D11DeviceContext()->ClearState();
    ////////////////////////////////////////////////////////////////////////////////
    // Beginning of rendering.
    m_pDevice->GetID3D11DeviceContext()->ClearRenderTargetView(
        pD3D11RenderTargetView,
        &std::array<FLOAT, 4>{0.1f, 0.1f, 0.1f, 1.0f}[0]);
    m_pDevice->GetID3D11DeviceContext()->RSSetViewports(
        1, &Make_D3D11_VIEWPORT(RENDERTARGET_WIDTH, RENDERTARGET_HEIGHT));
    m_pDevice->GetID3D11DeviceContext()->OMSetRenderTargets(
        1, &pD3D11RenderTargetView.p, nullptr);
    ////////////////////////////////////////////////////////////////////////////////
    // Update constant buffer.
    {
      Constants constants = {};
      constants.TransformWorldToClip = GetCameraWorldToClip();
      constants.TransformWorldToView = GetCameraWorldToView();
      Matrix44 t = Invert(GetCameraWorldToView());
      constants.CameraPosition = Vector3 { t.M41, t.M42, t.M43 };
      m_pDevice->GetID3D11DeviceContext()->UpdateSubresource(
          m_pD3D11BufferConstants, 0, nullptr, &constants, 0, 0);
    }
    ////////////////////////////////////////////////////////////////////////////////
    // Create a vertex buffer.
    CComPtr<ID3D11Buffer> m_pD3D11BufferVertex;
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
      m_pD3D11BufferVertex = D3D11_Create_Buffer(m_pDevice->GetID3D11Device(),
                                                 D3D11_BIND_VERTEX_BUFFER,
                                                 sizeof(vertices), vertices);
    }
    ////////////////////////////////////////////////////////////////////////////////
    // Setup and draw.
    m_pDevice->GetID3D11DeviceContext()->VSSetShader(m_pD3D11VertexShader,
                                                     nullptr, 0);
    m_pDevice->GetID3D11DeviceContext()->VSSetConstantBuffers(
        0, 1, &m_pD3D11BufferConstants.p);
    m_pDevice->GetID3D11DeviceContext()->PSSetShader(m_pD3D11PixelShader,
                                                     nullptr, 0);
    m_pDevice->GetID3D11DeviceContext()->PSSetConstantBuffers(
        0, 1, &m_pD3D11BufferConstants.p);
    m_pDevice->GetID3D11DeviceContext()->PSSetSamplers(0, 1,
                                                       &m_pD3D11SamplerState.p);
    m_pDevice->GetID3D11DeviceContext()->PSSetShaderResources(
        0, 1, &m_pSRVAlbedoMap.p);
    m_pDevice->GetID3D11DeviceContext()->PSSetShaderResources(
        1, 1, &m_pSRVNormalMap.p);
    m_pDevice->GetID3D11DeviceContext()->PSSetShaderResources(
        2, 1, &m_pSRVDepthMap.p);
    m_pDevice->GetID3D11DeviceContext()->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pDevice->GetID3D11DeviceContext()->IASetInputLayout(m_pD3D11InputLayout);
    {
      UINT uStrides[] = {sizeof(VertexFormat)};
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

std::shared_ptr<Sample>
CreateSample_D3D11ParallaxMap(std::shared_ptr<DXGISwapChain> swapchain,
                              std::shared_ptr<Direct3D11Device> device) {
  return std::shared_ptr<Sample>(
      new Sample_D3D11ParallaxMap(swapchain, device));
}