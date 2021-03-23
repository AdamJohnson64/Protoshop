#include "Core_D3D.h"
#include "Core_D3D11Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_FontAtlas.h"
#include "Core_Math.h"
#include "Core_Util.h"
#include "ImageUtil.h"
#include "SampleResources.h"
#include <array>
#include <atlbase.h>
#include <functional>

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11Font(std::shared_ptr<Direct3D11Device> device) {
  struct Constants {
    Matrix44 TransformScreenToClip;
  };
  CComPtr<ID3D11Buffer> bufferConstants = D3D11_Create_Buffer(
      device->GetID3D11Device(), D3D11_BIND_CONSTANT_BUFFER, sizeof(Constants));
  const char *szShaderCode = R"SHADER(
cbuffer Constants
{
    float4x4 TransformScreenToClip;
};

struct Vertex
{
    float4 Position : SV_Position;
    float2 Texcoord : TEXCOORD;
};

Texture2D<float4> TextureFont : register(t0);

Vertex mainVS(Vertex vin)
{
Vertex vout;
vout.Position = mul(TransformScreenToClip, vin.Position);
vout.Texcoord = vin.Texcoord;
return vout;
}

float4 mainPS(Vertex vin) : SV_Target
{
    float3 texelFont = TextureFont.Load(float3(vin.Texcoord, 0)).rgb;
    return float4(texelFont, 1);
})SHADER";
  struct Vertex {
    Vector2 Position;
    Vector2 Texcoord;
  };
  CComPtr<ID3D11VertexShader> shaderVertex;
  CComPtr<ID3D11InputLayout> inputLayout;
  {
    CComPtr<ID3DBlob> blobVS = CompileShader("vs_5_0", "mainVS", szShaderCode);
    TRYD3D(device->GetID3D11Device()->CreateVertexShader(
        blobVS->GetBufferPointer(), blobVS->GetBufferSize(), nullptr,
        &shaderVertex));
    {
      std::array<D3D11_INPUT_ELEMENT_DESC, 2> inputdesc = {};
      inputdesc[0].SemanticName = "SV_Position";
      inputdesc[0].Format = DXGI_FORMAT_R32G32_FLOAT;
      inputdesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
      inputdesc[0].AlignedByteOffset = offsetof(Vertex, Position);
      inputdesc[1].SemanticName = "TEXCOORD";
      inputdesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
      inputdesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
      inputdesc[1].AlignedByteOffset = offsetof(Vertex, Texcoord);
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
  // Create the font atlas map.
  std::unique_ptr<FontASCII> theFont = CreateFreeTypeFont();
  CComPtr<ID3D11ShaderResourceView> srvFont =
      D3D11_Create_SRV(device->GetID3D11DeviceContext(), theFont->Atlas.get());
  ////////////////////////////////////////////////////////////////////////////////
  // Create a vertex buffer.
  CComPtr<ID3D11Buffer> bufferVertex;
  std::vector<Vertex> vertices;
  {
    std::function<void(const Vector2 &, char)> drawChar =
        [&](const Vector2 &location, char c) {
          const GlyphMetadata &r = theFont->ASCIIToGlyph[c];
          float minU = r.Image.X;
          float maxU = r.Image.X + r.Image.Width;
          float minV = r.Image.Y;
          float maxV = r.Image.Y + r.Image.Height;
          float minX = location.X + r.OffsetX;
          float maxX = minX + r.Image.Width;
          float minY = location.Y - r.OffsetY;
          float maxY = minY + r.Image.Height;
          vertices.push_back({{minX, minY}, {minU, minV}});
          vertices.push_back({{maxX, minY}, {maxU, minV}});
          vertices.push_back({{maxX, maxY}, {maxU, maxV}});
          vertices.push_back({{maxX, maxY}, {maxU, maxV}});
          vertices.push_back({{minX, maxY}, {minU, maxV}});
          vertices.push_back({{minX, minY}, {minU, minV}});
        };
    std::function<void(const Vector2 &, const char *)> drawString =
        [&](const Vector2 &location, const char *text) {
          float cursorX = location.X;
          while (*text != 0) {
            drawChar({cursorX, location.Y}, *text);
            const GlyphMetadata &r = theFont->ASCIIToGlyph[*text];
            cursorX += r.AdvanceX;
            ++text;
          }
        };
    drawString({0, 64}, "How To Render Text Using FreeType!");
    bufferVertex =
        D3D11_Create_Buffer(device->GetID3D11Device(), D3D11_BIND_VERTEX_BUFFER,
                            sizeof(Vertex) * vertices.size(), &vertices[0]);
  }
  return [=](const SampleResourcesD3D11 &sampleResources) {
    D3D11_TEXTURE2D_DESC descBackbuffer = {};
    sampleResources.BackBufferTexture->GetDesc(&descBackbuffer);
    CComPtr<ID3D11RenderTargetView> rtvBackbuffer =
        D3D11_Create_RTV_From_Texture2D(device->GetID3D11Device(),
                                        sampleResources.BackBufferTexture);
    ////////////////////////////////////////////////////////////////////////////////
    // Update constant buffer.
    {
      Constants data = {};
      data.TransformScreenToClip = Identity<float>;
      data.TransformScreenToClip.M11 = 2.0f / descBackbuffer.Width;
      data.TransformScreenToClip.M22 = -2.0f / descBackbuffer.Height;
      data.TransformScreenToClip.M41 = -1;
      data.TransformScreenToClip.M42 = 1;
      device->GetID3D11DeviceContext()->UpdateSubresource(bufferConstants, 0,
                                                          nullptr, &data, 0, 0);
    }
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
    // Setup and draw.
    device->GetID3D11DeviceContext()->VSSetShader(shaderVertex, nullptr, 0);
    device->GetID3D11DeviceContext()->VSSetConstantBuffers(0, 1,
                                                           &bufferConstants.p);
    device->GetID3D11DeviceContext()->PSSetShader(shaderPixel, nullptr, 0);
    device->GetID3D11DeviceContext()->PSSetShaderResources(0, 1, &srvFont.p);
    device->GetID3D11DeviceContext()->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    device->GetID3D11DeviceContext()->IASetInputLayout(inputLayout);
    {
      UINT uStrides[] = {sizeof(Vertex)};
      UINT uOffsets[] = {0};
      device->GetID3D11DeviceContext()->IASetVertexBuffers(
          0, 1, &bufferVertex.p, uStrides, uOffsets);
    }
    device->GetID3D11DeviceContext()->Draw(vertices.size(), 0);
    device->GetID3D11DeviceContext()->ClearState();
    device->GetID3D11DeviceContext()->Flush();
  };
}