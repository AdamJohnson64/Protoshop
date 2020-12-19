///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Mesh
///////////////////////////////////////////////////////////////////////////////
// A simple shadow map (no cascades or fancy filtering).
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D11Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_ITransformSource.h"
#include "Core_Math.h"
#include "Core_Util.h"
#include "MutableMap.h"
#include "Scene_IMesh.h"
#include "Scene_InstanceTable.h"
#include <array>
#include <atlbase.h>
#include <functional>

std::function<void(ID3D11Texture2D *, ID3D11DepthStencilView *,
                   const Matrix44 &)>
CreateSample_D3D11ShadowMap(std::shared_ptr<Direct3D11Device> device) {

  ////////////////////////////////////////////////////////////////////////////////
  // Note here that we finally split the transform stack into world and object
  // transforms. This wastes a bit more processing in the vertex stage since we
  // need to perform two matrix multiplies to get from object to clip space. We
  // do this so we don't have to update all the objects when we switch between
  // camera views for the shadow map - that cost far exceeds the multiplies.

  __declspec(align(16)) struct ConstantsWorld {
    Matrix44 TransformWorldToClip;
    Matrix44 TransformWorldToClipShadow;
    Matrix44 TransformWorldToClipShadowInverse;
  };

  struct ConstantsObject {
    Matrix44 TransformObjectToWorld;
  };

  ////////////////////////////////////////////////////////////////////////////////
  // Create shaders & input layout.
  // We'll need shaders for both the depth render and the primary render pass.
  // Depth render doesn't need shadow maps since it's only rendering geometry
  // for future depth information.
  // Primary render will have color information and will reuse the shadow map
  // as a depth comparison test target to evaluate shadowing.

  const char *szShaderCode = R"SHADER(
cbuffer ConstantsWorld : register(b0)
{
    float4x4 TransformWorldToClip;
    float4x4 TransformWorldToClipShadow;
    float4x4 TransformWorldToClipShadowInverse;
};

cbuffer ConstantsObject : register(b1)
{
    float4x4 TransformObjectToWorld;
};

Texture2D TextureShadowMap : register(t0);
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
    vout.Position = mul(TransformWorldToClip, mul(TransformObjectToWorld, vin.Position));
    vout.Normal = normalize(mul(TransformObjectToWorld, float4(vin.Normal, 0)).xyz);
    vout.Texcoord = vin.Texcoord;
    vout.WorldPosition = mul(TransformObjectToWorld, vin.Position).xyz;
    return vout;
}

float4 mainPS(VertexPS vin) : SV_Target
{
    float3 lightPosition = float3(4, 2, -4);
    float3 vectorLight = lightPosition - vin.WorldPosition;
    float lightDistance = length(vectorLight);
    vectorLight /= lightDistance;

    float4 worldInShadowSpaceHomogeneous = mul(TransformWorldToClipShadow, float4(vin.WorldPosition, 1));
    float4 worldInShadowSpaceHomogeneousWDIV = worldInShadowSpaceHomogeneous / worldInShadowSpaceHomogeneous.w;
    float spotlight = 1 - pow(sqrt(dot(worldInShadowSpaceHomogeneousWDIV.xy, worldInShadowSpaceHomogeneousWDIV.xy)), 10);

    float i = dot(vectorLight, vin.Normal) * spotlight - 0.002f * lightDistance * lightDistance;
    float4 color = float4(i, i, i, 1);

    // DEBUG : Map world points into the shadow map camera frame so we can see
    // what the shadow camera sees. 
    if (worldInShadowSpaceHomogeneous.w > 0 && worldInShadowSpaceHomogeneousWDIV.z < 1 &&
        worldInShadowSpaceHomogeneousWDIV.x > -1 && worldInShadowSpaceHomogeneousWDIV.x < 1 &&
        worldInShadowSpaceHomogeneousWDIV.y > -1 && worldInShadowSpaceHomogeneousWDIV.y < 1)
    {
      //color.b = 1;
    }

    // Map each world point into the shadow map buffer.
    // This will take clip space and map it to texture UV space.
    // The Y axis is upside down.
    float2 shadowuv = (worldInShadowSpaceHomogeneousWDIV.xy + 1) / 2;
    shadowuv.y = 1 - shadowuv.y;
    // Read the depth value out of the shadow map.
    float depth = TextureShadowMap.Sample(userSampler, shadowuv).x;
    // If the w adjusted depth of this fragment is beyond the shadow then we are shadowed.
    if (worldInShadowSpaceHomogeneousWDIV.z > (depth + 0.0001))
    {
      color.rgb = 0;
    }

    return color;
})SHADER";

  CComPtr<ID3D11VertexShader> shaderVertex;
  CComPtr<ID3D11InputLayout> inputLayout;
  {
    CComPtr<ID3DBlob> blobVS = CompileShader("vs_5_0", "mainVS", szShaderCode);
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

  ////////////////////////////////////////////////////////////////////////////////
  // Default wrapping sampler.
  CComPtr<ID3D11SamplerState> samplerState;
  TRYD3D(device->GetID3D11Device()->CreateSamplerState(
      &Make_D3D11_SAMPLER_DESC_DefaultBorder(), &samplerState.p));

  ////////////////////////////////////////////////////////////////////////
  // Constant slot 0 : World Constants
  // These constants persist through all draw calls in the scene for any
  // single pass. The shadow map and primary render will have *different*
  // versions of this since the depth map uses a different camera.

  CComPtr<ID3D11Buffer> constantsWorld =
      D3D11_Create_Buffer(device->GetID3D11Device(), D3D11_BIND_CONSTANT_BUFFER,
                          sizeof(ConstantsWorld));

  ////////////////////////////////////////////////////////////////////////
  // Render object factories.
  // Unlike other samples we can actually have immutable object properties
  // this time. Object state generally doesn't change within the scope of
  // a complete frame (e.g. objects don't move within one frame).
  // Other maps generate the buffers for all the meshes.

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
    mesh->copyTexcoords(reinterpret_cast<Vector3 *>(
                            bytesVertex.get() + offsetof(VertexVS, Texcoord)),
                        sizeof(VertexVS));
    return D3D11_Create_Buffer(device->GetID3D11Device(),
                               D3D11_BIND_VERTEX_BUFFER, sizeVertex,
                               bytesVertex.get());
  };

  const int SHADOW_MAP_WIDTH = 1024;
  const int SHADOW_MAP_HEIGHT = 1024;
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

  ////////////////////////////////////////////////////////////////////////
  // Scene and top level render function.
  // This lambda captures all the static resources above and yields the
  // single draw call which draws everything to the output target.

  const std::vector<Instance> &scene = Scene_Default();
  return [=](ID3D11Texture2D *textureBackbuffer,
             ID3D11DepthStencilView *dsvDepth,
             const Matrix44 &transformWorldToClip) {
    D3D11_TEXTURE2D_DESC descBackbuffer = {};
    textureBackbuffer->GetDesc(&descBackbuffer);
    CComPtr<ID3D11RenderTargetView> rtvBackbuffer =
        D3D11_Create_RTV_From_Texture2D(device->GetID3D11Device(),
                                        textureBackbuffer);
    ////////////////////////////////////////////////////////////////////////
    // Setup primitive assembly.
    // This is common to all draw calls within the frame. We clear the state
    // in advance to ensure we never receive any dirty state at the top of
    // the render call tree.

    device->GetID3D11DeviceContext()->ClearState();
    device->GetID3D11DeviceContext()->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    device->GetID3D11DeviceContext()->IASetInputLayout(inputLayout);
    device->GetID3D11DeviceContext()->PSSetSamplers(0, 1, &samplerState.p);

    ////////////////////////////////////////////////////////////////////////
    // This function draws everything, even if it can't be seen.
    // In reality you'd want some sort of broad/narrow phase scene culling and
    // try not to draw garbage that can't be seen by the camera.

    std::function<void()> DRAWEVERYTHING = [&]() {
      for (int instanceIndex = 0; instanceIndex < scene.size();
           ++instanceIndex) {
        const Instance &instance = scene[instanceIndex];
        ////////////////////////////////////////////////////////////////////////
        // Setup the material; specific shaders and shader parameters.
        device->GetID3D11DeviceContext()->VSSetShader(shaderVertex, nullptr, 0);
        device->GetID3D11DeviceContext()->PSSetShader(shaderPixel, nullptr, 0);
        ////////////////////////////////////////////////////////////////////////
        // Constant slot 1 : Object contants
        // These constants are immutable for the object.
        {
          auto constantsObject =
              factoryConstants(instance.TransformObjectToWorld.get());
          device->GetID3D11DeviceContext()->VSSetConstantBuffers(
              1, 1, &constantsObject.p);
        }
        ////////////////////////////////////////////////////////////////////////
        // Setup geometry for draw.
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
        CreateMatrixLookAt(Vector3{4, 2, -4}, Vector3{0, 0, 0},
                           Vector3{0, 1, 0}) *
        CreateProjection(0.01f, 10.0f, 90 * (Pi<float> / 180),
                         90 * (Pi<float> / 180));

    ////////////////////////////////////////////////////////////////////////
    // Pass 1 - Draw the entire world from the viewpoint of the camera.
    // We only draw a depth image here, the primary render target is not
    // set. We're only interested in depth samples for the shadow depth test.

    // Set all the global constants for the render.
    // Note that we're placing a single shadow map transform in here because
    // there's only one light source. In the real world you may or may not
    // want to split the constants into world and light constants. In
    // practice you usually don't have enough lights to warrant that split
    // and the world constants can just be replicated into the lights.
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
        dsvDepthShadow, D3D11_CLEAR_DEPTH, 1, 0);
    device->GetID3D11DeviceContext()->RSSetViewports(
        1, &Make_D3D11_VIEWPORT(SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT));
    ID3D11RenderTargetView *rtvNULL = {};
    device->GetID3D11DeviceContext()->OMSetRenderTargets(1, &rtvNULL,
                                                         dsvDepthShadow);
    DRAWEVERYTHING();

    ////////////////////////////////////////////////////////////////////////
    // Pass 2 - Draw the entire world again from the view of the primary
    // camera and with the shadow map enabled.

    {
      ConstantsWorld data = {};
      data.TransformWorldToClip = transformWorldToClip;
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
        dsvDepth, D3D11_CLEAR_DEPTH, 1, 0);
    device->GetID3D11DeviceContext()->RSSetViewports(
        1, &Make_D3D11_VIEWPORT(descBackbuffer.Width, descBackbuffer.Height));
    device->GetID3D11DeviceContext()->OMSetRenderTargets(1, &rtvBackbuffer.p,
                                                         dsvDepth);
    device->GetID3D11DeviceContext()->PSSetShaderResources(0, 1,
                                                           &srvDepthShadow.p);
    DRAWEVERYTHING();

    ////////////////////////////////////////////////////////////////////////
    // Clean up the DC state and flush everything to GPU
    device->GetID3D11DeviceContext()->ClearState();
    device->GetID3D11DeviceContext()->Flush();
  };
}
