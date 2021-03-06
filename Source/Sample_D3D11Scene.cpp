///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Scene
///////////////////////////////////////////////////////////////////////////////
// This sample renders the contents of a Scene object with Direct3D 11.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D11Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_Math.h"
#include "Core_Util.h"
#include "ImageUtil.h"
#include "Image_TGA.h"
#include "MutableMap.h"
#include "SampleResources.h"
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

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11Scene(std::shared_ptr<Direct3D11Device> device,
                        const std::vector<Instance> &scene) {
  ////////////////////////////////////////////////////////////////////////////////
  // Create all the shaders that we might need.
  const char *szShaderCode = R"SHADER(
#include "Sample_D3D11_Common.inc"

float4 mainPS(VertexPS vin) : SV_Target
{
    float3 p = vin.WorldPosition;
    float3 n = vin.Normal;
    float3 l = normalize(float3(1, 4, -1) - p);
    float illum = dot(n, l);
    return float4(illum, illum, illum, 1);
}

float4 mainPSOBJDepthOnly(VertexPS vin) : SV_Target
{
    ////////////////////////////////////////////////////////////////////////////////
    // Alpha Masking (Alpha Test)
    float4 texelMask = TextureMaskMap.Sample(SamplerDefaultWrap, vin.Texcoord);
    if (texelMask.x < 0.5) discard;
    return float4(1, 1, 1, 1);
}

float4 mainPSOBJMaterial(VertexPS vin) : SV_Target
{
    ////////////////////////////////////////////////////////////////////////////////
    // Alpha Masking (Alpha Test)
    float4 texelMask = TextureMaskMap.Sample(SamplerDefaultWrap, vin.Texcoord);
    if (texelMask.x < 0.5) discard;

    ////////////////////////////////////////////////////////////////////////////////
    // Shadow Mapping
    float spotlight = CalculateSpotlightShadowMap(vin.WorldPosition);

    ////////////////////////////////////////////////////////////////////////////////
    // Normal Mapping.
    // Calculate the normal (Pixel/Analytical Based).
    float3x3 matTangentFrame = cotangent_frame(vin.Normal, vin.WorldPosition, vin.Texcoord);    
    float3 texelNormal = TextureNormalMap.Sample(SamplerDefaultWrap, vin.Texcoord).xyz * 2 - 1;
    float3 vectorNormal = normalize(mul(texelNormal, matTangentFrame));

    ////////////////////////////////////////////////////////////////////////////////
    // Final Lighting.
    float3 vectorLight = float3(-10, 4, -3) - vin.WorldPosition;
    float lightDistance = length(vectorLight);
    vectorLight /= lightDistance;
    float illumination = max(0, dot(vectorNormal, vectorLight) * spotlight - 0.005f * lightDistance * lightDistance);

    float4 texelAlbedo = TextureAlbedoMap.Sample(SamplerDefaultWrap, vin.Texcoord);
    return float4(texelAlbedo.xyz * illumination, texelAlbedo.w);
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
  CComPtr<ID3D11PixelShader> shaderPixelOBJDepthOnly;
  {
    ID3DBlob *blob =
        CompileShader("ps_5_0", "mainPSOBJDepthOnly", szShaderCode);
    TRYD3D(device->GetID3D11Device()->CreatePixelShader(
        blob->GetBufferPointer(), blob->GetBufferSize(), nullptr,
        &shaderPixelOBJDepthOnly.p));
  }
  CComPtr<ID3D11PixelShader> shaderPixelOBJMaterial;
  {
    ID3DBlob *blob = CompileShader("ps_5_0", "mainPSOBJMaterial", szShaderCode);
    TRYD3D(device->GetID3D11Device()->CreatePixelShader(
        blob->GetBufferPointer(), blob->GetBufferSize(), nullptr,
        &shaderPixelOBJMaterial.p));
  }
  ////////////////////////////////////////////////////////////////////////////////
  // Create the input vertex layout.
  CComPtr<ID3D11InputLayout> inputLayout;
  {
    std::array<D3D11_INPUT_ELEMENT_DESC, 3> desc = {};
    desc[0].SemanticName = "SV_Position";
    desc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    desc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    desc[0].AlignedByteOffset = offsetof(VertexVS, Position);
    desc[1].SemanticName = "NORMAL";
    desc[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    desc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    desc[1].AlignedByteOffset = offsetof(VertexVS, Normal);
    desc[2].SemanticName = "TEXCOORD";
    desc[2].Format = DXGI_FORMAT_R32G32_FLOAT;
    desc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    desc[2].AlignedByteOffset = offsetof(VertexVS, Texcoord);
    TRYD3D(device->GetID3D11Device()->CreateInputLayout(
        &desc[0], desc.size(), blobShaderVertex->GetBufferPointer(),
        blobShaderVertex->GetBufferSize(), &inputLayout));
  }
  ////////////////////////////////////////////////////////////////////////
  // Constant slot 0 : World Constants
  // These constants persist through all draw calls in the scene for any
  // single pass. The shadow map and primary render will have *different*
  // versions of this since the depth map uses a different camera.

  CComPtr<ID3D11Buffer> constantsWorld =
      D3D11_Create_Buffer(device->GetID3D11Device(), D3D11_BIND_CONSTANT_BUFFER,
                          sizeof(ConstantsWorld));

  ////////////////////////////////////////////////////////////////////////////////
  // Default wrapping sampler.

  CComPtr<ID3D11SamplerState> samplerDefaultWrap;
  TRYD3D(device->GetID3D11Device()->CreateSamplerState(
      &Make_D3D11_SAMPLER_DESC_DefaultWrap(), &samplerDefaultWrap.p));

  CComPtr<ID3D11SamplerState> samplerDefaultBorder;
  TRYD3D(device->GetID3D11Device()->CreateSamplerState(
      &Make_D3D11_SAMPLER_DESC_DefaultBorder(), &samplerDefaultBorder.p));

  ////////////////////////////////////////////////////////////////////////////////
  // Our so-called Factory Storage (aka) "Mutable Map".
  ////////////////////////////////////////////////////////////////////////////////
  // The role of these bizarre objects is to be captured into a lambda and to
  // persist storage between frames. This approach allows us to build render
  // resources on-demand and keep them hanging around for future frames.

  MutableMap<const Matrix44 *, CComPtr<ID3D11Buffer>> factoryConstants;
  factoryConstants.fnGenerator = [=](const Matrix44 *transform) {
    ConstantsObject data = {};
    data.TransformObjectToWorld = *transform;
    return D3D11_Create_Buffer(device->GetID3D11Device(),
                               D3D11_BIND_CONSTANT_BUFFER,
                               sizeof(ConstantsObject), &data);
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
    int sizeVertex = sizeof(VertexVS) * mesh->getVertexCount();
    std::unique_ptr<int8_t[]> bytesVertex(new int8_t[sizeVertex]);
    mesh->copyVertices(reinterpret_cast<Vector3 *>(
                           bytesVertex.get() + offsetof(VertexVS, Position)),
                       sizeof(VertexVS));
    mesh->copyNormals(reinterpret_cast<Vector3 *>(bytesVertex.get() +
                                                  offsetof(VertexVS, Normal)),
                      sizeof(VertexVS));
    mesh->copyTexcoords(reinterpret_cast<Vector2 *>(
                            bytesVertex.get() + offsetof(VertexVS, Texcoord)),
                        sizeof(VertexVS));
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
  CComPtr<ID3D11ShaderResourceView> defaultNormalMap =
      D3D11_Create_SRV(device->GetID3D11DeviceContext(),
                       Image_SolidColor(1, 1, 0xFF8080FF).get());
  CComPtr<ID3D11ShaderResourceView> defaultMaskMap =
      D3D11_Create_SRV(device->GetID3D11DeviceContext(),
                       Image_SolidColor(1, 1, 0xFFFFFFFF).get());

  const int SHADOW_MAP_WIDTH = 4096;
  const int SHADOW_MAP_HEIGHT = 4096;
  CComPtr<ID3D11DepthStencilView> dsvDepthShadow;
  CComPtr<ID3D11ShaderResourceView> srvDepthShadow;
  {
    CComPtr<ID3D11Texture2D> textureDepthShadow;
    {
      D3D11_TEXTURE2D_DESC desc = {};
      desc.Width = SHADOW_MAP_WIDTH;
      desc.Height = SHADOW_MAP_HEIGHT;
      desc.ArraySize = 1;
      desc.Format = DXGI_FORMAT_R32_TYPELESS; // DXGI_FORMAT_D24_UNORM_S8_UINT;
      desc.SampleDesc.Count = 1;
      desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
      TRYD3D(device->GetID3D11Device()->CreateTexture2D(&desc, nullptr,
                                                        &textureDepthShadow));
    }
    {
      D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
      desc.Format = DXGI_FORMAT_D32_FLOAT;
      desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
      TRYD3D(device->GetID3D11Device()->CreateDepthStencilView(
          textureDepthShadow, &desc, &dsvDepthShadow.p));
    }
    {
      D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
      desc.Format = DXGI_FORMAT_R32_FLOAT;
      desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
      desc.Texture2D.MipLevels = 1;
      TRYD3D(device->GetID3D11Device()->CreateShaderResourceView(
          textureDepthShadow, &desc, &srvDepthShadow.p));
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Capture all of the above into the rendering function for every frame.
  //
  // One really good reason to use a lambda is that they can implicitly
  // capture state moving into the render. There's no need to declare class
  // members to carry this state. This should make it easier to offload
  // processing into construction without changing things too much (you should
  // just move the code out of the lambda).
  return [=](const SampleResourcesD3D11 &sampleResources) {
    D3D11_TEXTURE2D_DESC descBackbuffer = {};
    sampleResources.BackBufferTexture->GetDesc(&descBackbuffer);
    CComPtr<ID3D11RenderTargetView> rtvBackbuffer =
        D3D11_Create_RTV_From_Texture2D(device->GetID3D11Device(),
                                        sampleResources.BackBufferTexture);
    ////////////////////////////////////////////////////////////////////////
    // Setup primary state; render targets, merger, prim assembly, etc.
    device->GetID3D11DeviceContext()->ClearState();
    device->GetID3D11DeviceContext()->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    device->GetID3D11DeviceContext()->IASetInputLayout(inputLayout);
    device->GetID3D11DeviceContext()->PSSetSamplers(kSamplerRegisterDefaultWrap,
                                                    1, &samplerDefaultWrap.p);
    device->GetID3D11DeviceContext()->PSSetSamplers(
        kSamplerRegisterDefaultBorder, 1, &samplerDefaultBorder.p);

    // Perform a shadowmap material setup.
    std::function<void(IMaterial *)> MATERIALSETUP_OBJDEPTHONLY =
        [&](IMaterial *material) {
          device->GetID3D11DeviceContext()->VSSetShader(shaderVertex, nullptr,
                                                        0);
          OBJMaterial *textured = dynamic_cast<OBJMaterial *>(material);
          if (textured != nullptr) {
            device->GetID3D11DeviceContext()->PSSetShader(
                shaderPixelOBJDepthOnly, nullptr, 0);
            // Bind Mask Map.
            CComPtr<ID3D11ShaderResourceView> srvMaskMap =
                factoryTexture(textured->DissolveMap.get());
            if (srvMaskMap == nullptr) {
              srvMaskMap = defaultMaskMap;
            }
            device->GetID3D11DeviceContext()->PSSetShaderResources(
                kTextureRegisterMaskMap, 1, &srvMaskMap.p);
          } else {
            device->GetID3D11DeviceContext()->PSSetShader(
                shaderPixelOBJDepthOnly, nullptr, 0);
          }
        };

    // Perform a full material setup.
    std::function<void(IMaterial *)> MATERIALSETUP_OBJMATERIAL =
        [&](IMaterial *material) {
          device->GetID3D11DeviceContext()->VSSetShader(shaderVertex, nullptr,
                                                        0);
          OBJMaterial *textured = dynamic_cast<OBJMaterial *>(material);
          if (textured != nullptr) {
            device->GetID3D11DeviceContext()->PSSetShader(
                shaderPixelOBJMaterial, nullptr, 0);
            device->GetID3D11DeviceContext()->PSSetSamplers(
                kSamplerRegisterDefaultWrap, 1, &samplerDefaultWrap.p);
            // Bind Albedo Map.
            CComPtr<ID3D11ShaderResourceView> srvAlbedoMap =
                factoryTexture(textured->DiffuseMap.get());
            if (srvAlbedoMap == nullptr) {
              srvAlbedoMap = defaultAlbedoMap;
            }
            device->GetID3D11DeviceContext()->PSSetShaderResources(
                kTextureRegisterAlbedoMap, 1, &srvAlbedoMap.p);

            // Bind Normal Map.
            CComPtr<ID3D11ShaderResourceView> srvNormalMap =
                factoryTexture(textured->NormalMap.get());
            if (srvNormalMap == nullptr) {
              srvNormalMap = defaultNormalMap;
            }
            device->GetID3D11DeviceContext()->PSSetShaderResources(
                kTextureRegisterNormalMap, 1, &srvNormalMap.p);

            // Bind Mask Map.
            CComPtr<ID3D11ShaderResourceView> srvMaskMap =
                factoryTexture(textured->DissolveMap.get());
            if (srvMaskMap == nullptr) {
              srvMaskMap = defaultMaskMap;
            }
            device->GetID3D11DeviceContext()->PSSetShaderResources(
                kTextureRegisterMaskMap, 1, &srvMaskMap.p);
          } else {
            device->GetID3D11DeviceContext()->PSSetShader(shaderPixel, nullptr,
                                                          0);
          }
        };

    std::function<void(std::function<void(IMaterial *)>)> DRAWEVERYTHING =
        [&](std::function<void(IMaterial *)> fnMaterialSetup) {
          for (int instanceIndex = 0; instanceIndex < scene.size();
               ++instanceIndex) {
            const Instance &instance = scene[instanceIndex];
            ////////////////////////////////////////////////////////////////////////
            // Setup the material; specific shaders and shader parameters.
            fnMaterialSetup(instance.Material.get());
            ////////////////////////////////////////////////////////////////////////
            // Setup geometry for draw.
            {
              auto constantBuffer =
                  factoryConstants(instance.TransformObjectToWorld.get());
              device->GetID3D11DeviceContext()->VSSetConstantBuffers(
                  1, 1, &constantBuffer.p);
            }
            {
              const UINT vertexStride[] = {sizeof(VertexVS)};
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
        };
    ////////////////////////////////////////////////////////////////////////
    // Create the camera projection matrix.
    // Right now this matrix just mimics the light in the pixel shader.

    Matrix44 TransformWorldToClipShadow =
        CreateMatrixLookAt(Vector3{-1, 2, 0}, Vector3{-4, 2, 0},
                           Vector3{0, 1, 0}) *
        CreateProjection(0.01f, 100.0f, 60 * (Pi<float> / 180),
                         60 * (Pi<float> / 180));

    ////////////////////////////////////////////////////////////////////////////////
    // Shadow Map (Depth Only) Pass.

    {
      ConstantsWorld data = {};
      data.TransformWorldToClip = TransformWorldToClipShadow;
      data.TransformWorldToClipShadow = TransformWorldToClipShadow;
      data.TransformWorldToClipShadowInverse =
          Invert(TransformWorldToClipShadow);
      device->GetID3D11DeviceContext()->UpdateSubresource(constantsWorld, 0,
                                                          nullptr, &data, 0, 0);
      device->GetID3D11DeviceContext()->VSSetConstantBuffers(0, 1,
                                                             &constantsWorld.p);
      device->GetID3D11DeviceContext()->PSSetConstantBuffers(0, 1,
                                                             &constantsWorld.p);
    }
    device->GetID3D11DeviceContext()->ClearDepthStencilView(
        dsvDepthShadow, D3D11_CLEAR_DEPTH, 1.0f, 0);
    device->GetID3D11DeviceContext()->RSSetViewports(
        1, &Make_D3D11_VIEWPORT(SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT));
    ID3D11RenderTargetView *rtvNULL = {};
    device->GetID3D11DeviceContext()->OMSetRenderTargets(1, &rtvNULL,
                                                         dsvDepthShadow);
    DRAWEVERYTHING(MATERIALSETUP_OBJDEPTHONLY);

    ////////////////////////////////////////////////////////////////////////////////
    // Main Pass - Draw the shaded materials.

    {
      ConstantsWorld data = {};
      data.TransformWorldToClip = sampleResources.TransformWorldToClip;
      data.TransformWorldToClipShadow = TransformWorldToClipShadow;
      data.TransformWorldToClipShadowInverse =
          Invert(TransformWorldToClipShadow);
      device->GetID3D11DeviceContext()->UpdateSubresource(constantsWorld, 0,
                                                          nullptr, &data, 0, 0);
      device->GetID3D11DeviceContext()->VSSetConstantBuffers(0, 1,
                                                             &constantsWorld.p);
      device->GetID3D11DeviceContext()->PSSetConstantBuffers(0, 1,
                                                             &constantsWorld.p);
    }
    device->GetID3D11DeviceContext()->ClearRenderTargetView(
        rtvBackbuffer, &std::array<FLOAT, 4>{0.1f, 0.1f, 0.1f, 1.0f}[0]);
    device->GetID3D11DeviceContext()->ClearDepthStencilView(
        sampleResources.DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
    device->GetID3D11DeviceContext()->RSSetViewports(
        1, &Make_D3D11_VIEWPORT(descBackbuffer.Width, descBackbuffer.Height));
    device->GetID3D11DeviceContext()->OMSetRenderTargets(
        1, &rtvBackbuffer.p, sampleResources.DepthStencilView);
    device->GetID3D11DeviceContext()->PSSetShaderResources(
        kTextureRegisterShadowMap, 1, &srvDepthShadow.p);
    DRAWEVERYTHING(MATERIALSETUP_OBJMATERIAL);

    ////////////////////////////////////////////////////////////////////////
    // Clean up the DC state and flush everything to GPU
    device->GetID3D11DeviceContext()->ClearState();
    device->GetID3D11DeviceContext()->Flush();
  };
}