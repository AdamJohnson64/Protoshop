///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Scene
///////////////////////////////////////////////////////////////////////////////
// This sample renders the contents of a Scene object with Direct3D 11.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D11Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_ITransformSource.h"
#include "Core_Math.h"
#include "Core_Util.h"
#include "Image_TGA.h"
#include "MutableMap.h"
#include "Scene_IMaterial.h"
#include "Scene_IMesh.h"
#include "Scene_InstanceTable.h"
#include <array>
#include <atlbase.h>
#include <functional>
#include <map>
#include <vector>

std::function<void(ID3D11Texture2D *, ID3D11DepthStencilView *,
                   const Matrix44 &)>
CreateSample_D3D11Scene(std::shared_ptr<Direct3D11Device> device,
                        const std::vector<Instance> &scene) {
  struct Constants {
    Matrix44 TransformWorldToClip;
    Matrix44 TransformObjectToWorld;
  };
  struct VertexFormat {
    Vector3 Position;
    Vector3 Normal;
    Vector2 Texcoord;
  };
  ////////////////////////////////////////////////////////////////////////////////
  // Create all the shaders that we might need.
  const char *szShaderCode = R"SHADER(
cbuffer Constants
{
    float4x4 TransformWorldToClip;
    float4x4 TransformObjectToWorld;
};

Texture2D TextureAlbedoMap;
sampler userSampler;

struct VertexVS
{
    float4 Position : SV_Position;
    float3 Normal : NORMAL;
    float2 Texcoord : TEXCOORD;
};

struct VertexPS
{
    float4 Position : SV_Position;
    float3 Normal : NORMAL;
    float2 Texcoord : TEXCOORD;
    float3 WorldPosition : POSITION1;
};

VertexPS mainVS(VertexVS vin)
{
    VertexPS vout;
    vout.Position = mul(TransformWorldToClip, vin.Position);
    vout.Normal = normalize(mul(TransformObjectToWorld, float4(vin.Normal, 0)).xyz);
    vout.Texcoord = vin.Texcoord;
    vout.WorldPosition = mul(TransformObjectToWorld, vin.Position).xyz;
    return vout;
}

float4 mainPS(VertexPS vin) : SV_Target
{
    float3 p = vin.WorldPosition;
    float3 n = vin.Normal;
    float3 l = normalize(float3(4, 4, -4) - p);
    float illum = dot(n, l);
    return float4(illum, illum, illum, 1);
}

float4 mainPSTextured(VertexPS vin) : SV_Target
{
    float3 p = vin.WorldPosition;
    float3 n = vin.Normal;
    float3 l = normalize(float3(4, 4, -4) - p);
    float illum = dot(n, l);
    float4 albedo = TextureAlbedoMap.Sample(userSampler, vin.Texcoord);
    return float4(albedo.xyz * illum, albedo.w);
})SHADER";
  CComPtr<ID3D11VertexShader> shaderVertex;
  CComPtr<ID3DBlob> blobShaderVertex =
      CompileShader("vs_5_0", "mainVS", szShaderCode);
  TRYD3D(device->GetID3D11Device()->CreateVertexShader(
      blobShaderVertex->GetBufferPointer(), blobShaderVertex->GetBufferSize(),
      nullptr, &shaderVertex.p));
  CComPtr<ID3D11PixelShader> shaderPixel;
  {
    ID3DBlob *blob = CompileShader("ps_5_0", "mainPS", szShaderCode);
    TRYD3D(device->GetID3D11Device()->CreatePixelShader(
        blob->GetBufferPointer(), blob->GetBufferSize(), nullptr,
        &shaderPixel.p));
  }
  CComPtr<ID3D11PixelShader> shaderPixelTextured;
  {
    ID3DBlob *blob = CompileShader("ps_5_0", "mainPSTextured", szShaderCode);
    TRYD3D(device->GetID3D11Device()->CreatePixelShader(
        blob->GetBufferPointer(), blob->GetBufferSize(), nullptr,
        &shaderPixelTextured.p));
  }
  ////////////////////////////////////////////////////////////////////////////////
  // Create the input vertex layout.
  CComPtr<ID3D11InputLayout> inputLayout;
  {
    std::array<D3D11_INPUT_ELEMENT_DESC, 3> desc = {};
    desc[0].SemanticName = "SV_Position";
    desc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    desc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    desc[0].AlignedByteOffset = offsetof(VertexFormat, Position);
    desc[1].SemanticName = "NORMAL";
    desc[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    desc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    desc[1].AlignedByteOffset = offsetof(VertexFormat, Normal);
    desc[2].SemanticName = "TEXCOORD";
    desc[2].Format = DXGI_FORMAT_R32G32_FLOAT;
    desc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    desc[2].AlignedByteOffset = offsetof(VertexFormat, Texcoord);
    TRYD3D(device->GetID3D11Device()->CreateInputLayout(
        &desc[0], desc.size(), blobShaderVertex->GetBufferPointer(),
        blobShaderVertex->GetBufferSize(), &inputLayout));
  }
  ////////////////////////////////////////////////////////////////////////////////
  // Default wrapping sampler.
  CComPtr<ID3D11SamplerState> samplerState;
  TRYD3D(device->GetID3D11Device()->CreateSamplerState(
      &Make_D3D11_SAMPLER_DESC_DefaultWrap(), &samplerState.p));
  ////////////////////////////////////////////////////////////////////////////////
  // Our so-called Factory Storage (aka) "Mutable Map".
  ////////////////////////////////////////////////////////////////////////////////
  // The role of these bizarre objects is to be captured into a lambda and to
  // persist storage between frames. This approach allows us to build render
  // resources on-demand and keep them hanging around for future frames.

  MutableMap<const IMesh *, CComPtr<ID3D11Buffer>> factoryIndex;
  factoryIndex.fnGenerator = [&](const IMesh *mesh) {
    int sizeIndices = sizeof(int32_t) * mesh->getIndexCount();
    std::unique_ptr<int8_t[]> bytesIndex(new int8_t[sizeIndices]);
    mesh->copyIndices(reinterpret_cast<uint32_t *>(bytesIndex.get()),
                      sizeof(uint32_t));
    return D3D11_Create_Buffer(device->GetID3D11Device(),
                               D3D11_BIND_INDEX_BUFFER, sizeIndices,
                               bytesIndex.get());
  };

  MutableMap<const IMesh *, CComPtr<ID3D11Buffer>> factoryVertex;
  factoryVertex.fnGenerator = [&](const IMesh *mesh) {
    int sizeVertex = sizeof(VertexFormat) * mesh->getVertexCount();
    std::unique_ptr<int8_t[]> bytesVertex(new int8_t[sizeVertex]);
    mesh->copyVertices(
        reinterpret_cast<Vector3 *>(bytesVertex.get() +
                                    offsetof(VertexFormat, Position)),
        sizeof(VertexFormat));
    mesh->copyNormals(reinterpret_cast<Vector3 *>(
                          bytesVertex.get() + offsetof(VertexFormat, Normal)),
                      sizeof(VertexFormat));
    mesh->copyTexcoords(
        reinterpret_cast<Vector2 *>(bytesVertex.get() +
                                    offsetof(VertexFormat, Texcoord)),
        sizeof(VertexFormat));
    return D3D11_Create_Buffer(device->GetID3D11Device(),
                               D3D11_BIND_VERTEX_BUFFER, sizeVertex,
                               bytesVertex.get());
  };

  MutableMap<const TextureImage *, CComPtr<ID3D11ShaderResourceView>>
      factoryTexture;
  factoryTexture.fnGenerator = [&](const TextureImage *texture) {
    auto image =
        Load_TGA(texture != nullptr ? texture->Filename.c_str() : nullptr);
    auto texture2D = D3D11_Create_Texture(device->GetID3D11Device(), &image);
    CComPtr<ID3D11ShaderResourceView> srv;
    TRYD3D(device->GetID3D11Device()->CreateShaderResourceView(
        texture2D.p,
        &Make_D3D11_SHADER_RESOURCE_VIEW_DESC_Texture2D(
            DXGI_FORMAT_B8G8R8A8_UNORM),
        &srv.p));
    return srv;
  };

  ////////////////////////////////////////////////////////////////////////////////
  // Capture all of the above into the rendering function for every frame.
  //
  // One really good reason to use a lambda is that they can implicitly
  // capture state moving into the render. There's no need to declare class
  // members to carry this state. This should make it easier to offload
  // processing into construction without changing things too much (you should
  // just move the code out of the lambda).
  return [=](ID3D11Texture2D *textureBackbuffer,
             ID3D11DepthStencilView *dsvDepth, const Matrix44 &transform) {
    D3D11_TEXTURE2D_DESC descBackbuffer = {};
    textureBackbuffer->GetDesc(&descBackbuffer);
    CComPtr<ID3D11RenderTargetView> rtvBackbuffer =
        D3D11_Create_RTV_From_Texture2D(device->GetID3D11Device(),
                                        textureBackbuffer);
    ////////////////////////////////////////////////////////////////////////
    // Setup primary state; render targets, merger, prim assembly, etc.
    device->GetID3D11DeviceContext()->ClearState();
    device->GetID3D11DeviceContext()->ClearRenderTargetView(
        rtvBackbuffer, &std::array<FLOAT, 4>{0.1f, 0.1f, 0.1f, 1.0f}[0]);
    device->GetID3D11DeviceContext()->ClearDepthStencilView(
        dsvDepth, D3D11_CLEAR_DEPTH, 1.0f, 0);
    device->GetID3D11DeviceContext()->RSSetViewports(
        1, &Make_D3D11_VIEWPORT(descBackbuffer.Width, descBackbuffer.Height));
    device->GetID3D11DeviceContext()->OMSetRenderTargets(1, &rtvBackbuffer.p,
                                                         dsvDepth);
    device->GetID3D11DeviceContext()->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    device->GetID3D11DeviceContext()->IASetInputLayout(inputLayout);
    std::vector<CComPtr<ID3D11Buffer>> bufferInstanceConstants;
    for (int instanceIndex = 0; instanceIndex < scene.size(); ++instanceIndex) {
      const Instance &instance = scene[instanceIndex];
      ////////////////////////////////////////////////////////////////////////
      // Setup the material; specific shaders and shader parameters.
      {
        device->GetID3D11DeviceContext()->VSSetShader(shaderVertex, nullptr, 0);
        Textured *textured = dynamic_cast<Textured *>(instance.Material.get());
        if (textured != nullptr) {
          CComPtr<ID3D11ShaderResourceView> albedo =
              factoryTexture.get(textured->AlbedoMap.get());
          device->GetID3D11DeviceContext()->PSSetShader(shaderPixelTextured,
                                                        nullptr, 0);
          device->GetID3D11DeviceContext()->PSSetSamplers(0, 1,
                                                          &samplerState.p);
          device->GetID3D11DeviceContext()->PSSetShaderResources(0, 1,
                                                                 &albedo.p);
        } else {
          device->GetID3D11DeviceContext()->PSSetShader(shaderPixel, nullptr,
                                                        0);
        }
      };
      ////////////////////////////////////////////////////////////////////////
      // Setup the constant buffers.
      {
        Constants constants = {};
        constants.TransformWorldToClip =
            instance.TransformObjectToWorld * transform;
        constants.TransformObjectToWorld = instance.TransformObjectToWorld;
        bufferInstanceConstants.push_back(D3D11_Create_Buffer(
            device->GetID3D11Device(), D3D11_BIND_CONSTANT_BUFFER,
            sizeof(Constants), &constants));
      }
      ////////////////////////////////////////////////////////////////////////
      // Setup geometry for draw.
      device->GetID3D11DeviceContext()->VSSetConstantBuffers(
          0, 1, &bufferInstanceConstants[instanceIndex].p);
      {
        const UINT vertexStride[] = {sizeof(VertexFormat)};
        const UINT vertexOffset[] = {0};
        auto vb = factoryVertex.get(scene[instanceIndex].Mesh.get());
        device->GetID3D11DeviceContext()->IASetVertexBuffers(
            0, 1, &vb.p, vertexStride, vertexOffset);
      }
      auto ib = factoryIndex.get(scene[instanceIndex].Mesh.get());
      device->GetID3D11DeviceContext()->IASetIndexBuffer(
          ib, DXGI_FORMAT_R32_UINT, 0);
      device->GetID3D11DeviceContext()->DrawIndexed(
          scene[instanceIndex].Mesh->getIndexCount(), 0, 0);
    }
    ////////////////////////////////////////////////////////////////////////
    // Clean up the DC state and flush everything to GPU
    device->GetID3D11DeviceContext()->ClearState();
    device->GetID3D11DeviceContext()->Flush();
  };
}