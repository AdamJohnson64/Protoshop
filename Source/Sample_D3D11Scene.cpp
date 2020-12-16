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
#include "ImageUtil.h"
#include "Image_TGA.h"
#include "MutableMap.h"
#include "Scene_IMaterial.h"
#include "Scene_IMesh.h"
#include "Scene_InstanceTable.h"
#include <array>
#include <atlbase.h>
#include <functional>
#include <future>
#include <map>
#include <vector>

#define LOAD_TEXTURE_ASYNC

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

Texture2D TextureAlbedoMap : register(t0);
Texture2D TextureMaskMap : register(t1);
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
    float3 l = normalize(float3(1, 4, -1) - p);
    float illum = dot(n, l);
    return float4(illum, illum, illum, 1);
}

float4 mainPSTextured(VertexPS vin) : SV_Target
{
    float4 mask = TextureMaskMap.Sample(userSampler, vin.Texcoord);
    if (mask.x < 0.5) discard;
    float3 p = vin.WorldPosition;
    float3 n = vin.Normal;
    float3 l = normalize(float3(1, 4, -1) - p);
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

  MutableMap<const Matrix44 *, CComPtr<ID3D11Buffer>> factoryConstants;
  factoryConstants.fnGenerator = [=](const Matrix44 *transform) {
    return D3D11_Create_Buffer(device->GetID3D11Device(),
                               D3D11_BIND_CONSTANT_BUFFER, sizeof(Constants));
  };

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

#ifdef LOAD_TEXTURE_ASYNC
  // Textures currently being loaded (async).
  std::map<const TextureImage *, std::shared_future<std::shared_ptr<IImage>>>
      mapTexturesLoading;
  // Textures processed and ready for use.
  std::map<const TextureImage *, CComPtr<ID3D11ShaderResourceView>>
      mapTexturesReady;

  // The "Texture Server" is just a function that returns SRVs ready for use.
  std::function<CComPtr<ID3D11ShaderResourceView>(const TextureImage *)>
      factoryTexture = [=](const TextureImage *texture) mutable {
        // Check if there's a fully processed version ready.
        auto findIt = mapTexturesReady.find(texture);
        if (findIt != mapTexturesReady.end()) {
          return findIt->second;
        }
        // There isn't one ready; check for a generator in flight.
        auto findGen = mapTexturesLoading.find(texture);
        if (findGen == mapTexturesLoading.end()) {
          mapTexturesLoading[texture] = std::async(
              [&](const TextureImage *texture) {
                return Load_TGA(texture != nullptr ? texture->Filename.c_str()
                                                   : nullptr);
              },
              texture);
          return CComPtr<ID3D11ShaderResourceView>(nullptr);
        }
        // The final result isn't ready (see above).
        // Check on the async generator for a final result.
        if (findGen->second.wait_for(std::chrono::milliseconds(0)) !=
            std::future_status::ready) {
          return CComPtr<ID3D11ShaderResourceView>(nullptr);
        }
        // We have a result pending; construct the SRV in sync.
        // Load the result into the completed map to short out the async
        // generator. If the texture is null then just fall back to the
        // default texture.
        auto result = findGen->second.get().get() != nullptr
                          ? D3D11_Create_SRV(device->GetID3D11DeviceContext(),
                                             findGen->second.get().get())
                          : CComPtr<ID3D11ShaderResourceView>(nullptr);
        mapTexturesReady[texture] = result;
        return result;
      };
#else
  MutableMap<const TextureImage *, CComPtr<ID3D11ShaderResourceView>>
      factoryTexture;
  factoryTexture.fnGenerator = [&](const TextureImage *texture) {
    if (texture == nullptr) {
      return CComPtr<ID3D11ShaderResourceView>(nullptr);
    }
    auto image = Load_TGA(texture->Filename.c_str());
    if (image == nullptr) {
      return CComPtr<ID3D11ShaderResourceView>(nullptr);
    }
    return D3D11_Create_SRV(device->GetID3D11DeviceContext(), image.get());
  };
#endif

  // Textures to use if all else fails.
  CComPtr<ID3D11ShaderResourceView> defaultAlbedoMap =
      D3D11_Create_SRV(device->GetID3D11DeviceContext(),
                       Image_SolidColor(1, 1, 0xFFFF0000).get());
  CComPtr<ID3D11ShaderResourceView> defaultMaskMap =
      D3D11_Create_SRV(device->GetID3D11DeviceContext(),
                       Image_SolidColor(1, 1, 0xFFFFFFFF).get());

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

    for (int instanceIndex = 0; instanceIndex < scene.size(); ++instanceIndex) {
      const Instance &instance = scene[instanceIndex];
      ////////////////////////////////////////////////////////////////////////
      // Setup the material; specific shaders and shader parameters.
      {
        device->GetID3D11DeviceContext()->VSSetShader(shaderVertex, nullptr, 0);
        OBJMaterial *textured =
            dynamic_cast<OBJMaterial *>(instance.Material.get());
        if (textured != nullptr) {
          device->GetID3D11DeviceContext()->PSSetShader(shaderPixelTextured,
                                                        nullptr, 0);
          device->GetID3D11DeviceContext()->PSSetSamplers(0, 1,
                                                          &samplerState.p);
          CComPtr<ID3D11ShaderResourceView> srvAlbedoMap =
              factoryTexture(textured->DiffuseMap.get());
          if (srvAlbedoMap == nullptr) {
            srvAlbedoMap = defaultAlbedoMap;
          }
          device->GetID3D11DeviceContext()->PSSetShaderResources(
              0, 1, &srvAlbedoMap.p);
          CComPtr<ID3D11ShaderResourceView> srvMaskMap =
              factoryTexture(textured->DissolveMap.get());
          if (srvMaskMap == nullptr) {
            srvMaskMap = defaultMaskMap;
          }
          device->GetID3D11DeviceContext()->PSSetShaderResources(1, 1,
                                                                 &srvMaskMap.p);
        } else {
          device->GetID3D11DeviceContext()->PSSetShader(shaderPixel, nullptr,
                                                        0);
        }
      };
      ////////////////////////////////////////////////////////////////////////
      // Setup geometry for draw.
      {
        auto constantBuffer =
            factoryConstants(instance.TransformObjectToWorld.get());
        {
          Constants constants = {};
          constants.TransformWorldToClip =
              *instance.TransformObjectToWorld * transform;
          constants.TransformObjectToWorld = *instance.TransformObjectToWorld;
          device->GetID3D11DeviceContext()->UpdateSubresource(
              constantBuffer, 0, nullptr, &constants, 0, 0);
        }
        device->GetID3D11DeviceContext()->VSSetConstantBuffers(
            0, 1, &constantBuffer.p);
      }
      {
        const UINT vertexStride[] = {sizeof(VertexFormat)};
        const UINT vertexOffset[] = {0};
        auto vb = factoryVertex(scene[instanceIndex].Mesh.get());
        device->GetID3D11DeviceContext()->IASetVertexBuffers(
            0, 1, &vb.p, vertexStride, vertexOffset);
      }
      auto ib = factoryIndex(scene[instanceIndex].Mesh.get());
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