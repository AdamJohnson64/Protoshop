///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 DXGI Capture
///////////////////////////////////////////////////////////////////////////////
// This sample grabs the desktop image via Windows DXGI and presents it in a
// 3D view.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3DCompiler.h"
#include "Core_D3D11.h"
#include "Core_D3D11Util.h"
#include "SampleResources.h"
#include <array>
#include <d3d11.h>
#include <dxgi1_6.h>
#include <functional>
#include <memory>

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11DXGICapture(std::shared_ptr<Direct3D11Device> device) {
  // Create a DXGI duplicated output for the desktop.
  CComPtr<IDXGIFactory7> pDXGIFactory7;
  TRYD3D(CreateDXGIFactory(__uuidof(IDXGIFactory7), (void**)&pDXGIFactory7));
  CComPtr<IDXGIAdapter1> pDXGIAdapter1;
  TRYD3D(pDXGIFactory7->EnumAdapters1(0, &pDXGIAdapter1));
  CComPtr<IDXGIOutput> pDXGIOutput;
  TRYD3D(pDXGIAdapter1->EnumOutputs(0, &pDXGIOutput));
  CComPtr<IDXGIOutput6> pDXGIOutput6;
  TRYD3D(pDXGIOutput->QueryInterface(&pDXGIOutput6));
  CComPtr<IDXGIOutputDuplication> pDXGIOutputDuplication;
  TRYD3D(pDXGIOutput6->DuplicateOutput(device->GetID3D11Device(), &pDXGIOutputDuplication));
  // Now for the rendering resources...
  CComPtr<ID3D11Texture2D> texAlbedoMap;
  {
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = 1920;
    desc.Height = 1080;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    TRYD3D(device->GetID3D11Device()->CreateTexture2D(&desc, nullptr, &texAlbedoMap));
  }
  CComPtr<ID3D11ShaderResourceView> srvAlbedoMap;
  {
    D3D11_TEXTURE2D_DESC descTexture = {};
    texAlbedoMap->GetDesc(&descTexture);
    D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MipLevels = descTexture.MipLevels;
    TRYD3D(device->GetID3D11Device()->CreateShaderResourceView(texAlbedoMap, &desc, &srvAlbedoMap.p));
  }
  CComPtr<ID3D11SamplerState> samplerDefaultWrap;
  TRYD3D(device->GetID3D11Device()->CreateSamplerState(
      &Make_D3D11_SAMPLER_DESC_DefaultWrap(), &samplerDefaultWrap.p));
  const char *szShaderCode = R"SHADER(
#include "Sample_D3D11_Common.inc"

float4 mainPS(VertexPS vin) : SV_Target
{
    // 1 TAP - This is going to look mighty ugly in VR.
    //return float4(TextureAlbedoMap.Sample(SamplerDefaultWrap, vin.Texcoord).xyz, 1);
    // X*Y TAP - Crank this up as high as you like for in-shader anti-aliasing of a single MIP texture.
    const int samplesU = 15;
    const int samplesV = 15;
    const float2 duvdx = ddx(vin.Texcoord.xy); 
    const float2 duvdy = ddy(vin.Texcoord.xy);
    float4 acc = float4(0, 0, 0, 0);
    for (int y = 0; y < samplesV; ++y) {
        for (int x = 0; x < samplesU; ++x) {
            acc += TextureAlbedoMap.Sample(SamplerDefaultWrap, vin.Texcoord.xy + duvdx * lerp(-0.5, 0.5, (x + 0.5) / samplesU) + duvdy * lerp(-0.5, 0.5, (y + 0.5) / samplesV));
        }
    }
    return acc / (samplesU * samplesV);
})SHADER";
  CComPtr<ID3D11VertexShader> shaderVertex;
  CComPtr<ID3D11InputLayout> inputLayout;
  {
    CComPtr<ID3DBlob> blobVS = CompileShader("vs_5_0", "mainVS_NOOBJTRANSFORM", szShaderCode);
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
    {
    D3D11_TEXTURE2D_DESC descBackbuffer = {};
    sampleResources.BackBufferTexture->GetDesc(&descBackbuffer);
    CComPtr<ID3D11RenderTargetView> rtvBackbuffer =
        D3D11_Create_RTV_From_Texture2D(device->GetID3D11Device(),
                                        sampleResources.BackBufferTexture);
    CComPtr<IDXGIResource> pDXGIResource;
    DXGI_OUTDUPL_FRAME_INFO info = {};
    if (pDXGIOutputDuplication->AcquireNextFrame(0, &info, &pDXGIResource) == S_OK) {
      CComPtr<ID3D11Resource> pD3D11ResourceDXGI;
      TRYD3D(pDXGIResource->QueryInterface<ID3D11Resource>(&pD3D11ResourceDXGI));
      device->GetID3D11DeviceContext()->CopyResource(texAlbedoMap, pD3D11ResourceDXGI);
      TRYD3D(pDXGIOutputDuplication->ReleaseFrame());
    }
    device->GetID3D11DeviceContext()->ClearState();
    // Beginning of rendering.
    device->GetID3D11DeviceContext()->ClearRenderTargetView(
        rtvBackbuffer, &std::array<FLOAT, 4>{0.1f, 0.1f, 0.1f, 1.0f}[0]);
    device->GetID3D11DeviceContext()->ClearDepthStencilView(
        sampleResources.DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
    device->GetID3D11DeviceContext()->RSSetViewports(
        1, &Make_D3D11_VIEWPORT(descBackbuffer.Width, descBackbuffer.Height));
    device->GetID3D11DeviceContext()->OMSetRenderTargets(
        1, &rtvBackbuffer.p, sampleResources.DepthStencilView);
    // Update constant buffer.
    {
      char constants[1024];
      memcpy(constants, &sampleResources.TransformWorldToClip,
             sizeof(Matrix44));
      device->GetID3D11DeviceContext()->UpdateSubresource(
          bufferConstants, 0, nullptr, constants, 0, 0);
    }
    device->GetID3D11DeviceContext()->VSSetShader(shaderVertex, nullptr, 0);
    device->GetID3D11DeviceContext()->VSSetConstantBuffers(0, 1,
                                                           &bufferConstants.p);
    device->GetID3D11DeviceContext()->PSSetShader(shaderPixel, nullptr, 0);
    device->GetID3D11DeviceContext()->PSSetSamplers(kSamplerRegisterDefaultWrap,
                                                    1, &samplerDefaultWrap.p);
    device->GetID3D11DeviceContext()->PSSetShaderResources(
        kTextureRegisterAlbedoMap, 1, &srvAlbedoMap.p);
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
    }
  };
}