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
#include "Scene_InstanceTable.h"
#include "Scene_Mesh.h"
#include <array>
#include <atlbase.h>
#include <functional>
#include <vector>

//#define USE_OPENVR

#ifdef USE_OPENVR
#include <openvr.h>

Matrix44 ConvertMatrix(vr::HmdMatrix34_t &m) {
  Matrix44 o = {};
  // clang-format off
  o.M11 = m.m[0][0]; o.M12 = m.m[1][0]; o.M13 = m.m[2][0]; o.M14 = 0;
  o.M21 = m.m[0][1]; o.M22 = m.m[1][1]; o.M23 = m.m[2][1]; o.M24 = 0;
  o.M31 = m.m[0][2]; o.M32 = m.m[1][2]; o.M33 = m.m[2][2]; o.M34 = 0;
  o.M41 = m.m[0][3]; o.M42 = m.m[1][3]; o.M43 = m.m[2][3]; o.M44 = 1;
  // clang-format on
  return o;
}

Matrix44 ConvertMatrix(vr::HmdMatrix44_t &m) {
  Matrix44 o = {};
  // clang-format off
  o.M11 = m.m[0][0]; o.M12 = m.m[1][0]; o.M13 = m.m[2][0]; o.M14 = m.m[3][0];
  o.M21 = m.m[0][1]; o.M22 = m.m[1][1]; o.M23 = m.m[2][1]; o.M24 = m.m[3][1];
  o.M31 = m.m[0][2]; o.M32 = m.m[1][2]; o.M33 = m.m[2][2]; o.M34 = m.m[3][2];
  o.M41 = m.m[0][3]; o.M42 = m.m[1][3]; o.M43 = m.m[2][3]; o.M44 = m.m[3][3];
  // clang-format on
  return o;
}
#endif // USE_OPENVR

std::function<void(ID3D11Texture2D *)>
CreateSample_D3D11Scene(std::shared_ptr<Direct3D11Device> device) {
  struct Constants {
    Matrix44 TransformWorldToClip;
    Matrix44 TransformObjectToWorld;
  };
  struct VertexFormat {
    Vector3 Position;
    Vector3 Normal;
  };
  ////////////////////////////////////////////////////////////////////////////////
  // Create all the shaders that we might need.
  const char *szShaderCode = R"SHADER(
cbuffer Constants
{
    float4x4 TransformWorldToClip;
    float4x4 TransformObjectToWorld;
};

struct VertexVS
{
    float4 Position : SV_Position;
    float3 Normal : NORMAL;
};

struct VertexPS
{
    float4 Position : SV_Position;
    float3 Normal : NORMAL;
    float3 WorldPosition : POSITION1;
};

VertexPS mainVS(VertexVS vin)
{
    VertexPS vout;
    vout.Position = mul(TransformWorldToClip, vin.Position);
    vout.Normal = normalize(mul(TransformObjectToWorld, float4(vin.Normal, 0)).xyz);
    vout.WorldPosition = mul(TransformObjectToWorld, vin.Position).xyz;
    return vout;
}

float4 mainPS(VertexPS vin) : SV_Target
{
    float3 p = vin.WorldPosition;
    float3 n = vin.Normal;
    float3 l = normalize(float3(4, 1, -4) - p);
    float illum = dot(n, l);
    return float4(illum, illum, illum, 1);
})SHADER";
  CComPtr<ID3D11VertexShader> shaderVertex;
  CComPtr<ID3DBlob> blobShaderVertex =
      CompileShader("vs_5_0", "mainVS", szShaderCode);
  TRYD3D(device->GetID3D11Device()->CreateVertexShader(
      blobShaderVertex->GetBufferPointer(), blobShaderVertex->GetBufferSize(),
      nullptr, &shaderVertex));
  CComPtr<ID3D11PixelShader> shaderPixel;
  {
    ID3DBlob *blob = CompileShader("ps_5_0", "mainPS", szShaderCode);
    TRYD3D(device->GetID3D11Device()->CreatePixelShader(
        blob->GetBufferPointer(), blob->GetBufferSize(), nullptr,
        &shaderPixel));
  }
  ////////////////////////////////////////////////////////////////////////////////
  // Create the input vertex layout.
  CComPtr<ID3D11InputLayout> inputLayout;
  {
    std::array<D3D11_INPUT_ELEMENT_DESC, 2> desc = {};
    desc[0].SemanticName = "SV_Position";
    desc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    desc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    desc[0].AlignedByteOffset = 0;
    desc[1].SemanticName = "NORMAL";
    desc[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    desc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    desc[1].AlignedByteOffset = sizeof(Vector3);
    TRYD3D(device->GetID3D11Device()->CreateInputLayout(
        &desc[0], 2, blobShaderVertex->GetBufferPointer(),
        blobShaderVertex->GetBufferSize(), &inputLayout));
  }
  ////////////////////////////////////////////////////////////////////////////////
  // Choose the scene.
  std::shared_ptr<InstanceTable> theScene(InstanceTable::Default());
  ////////////////////////////////////////////////////////////////////////////////
  // Build all vertex/index buffer resources for the scene.
  std::vector<CComPtr<ID3D11Buffer>> bufferMeshVertex;
  std::vector<CComPtr<ID3D11Buffer>> bufferMeshIndex;
  for (int meshIndex = 0; meshIndex < theScene->Meshes.size(); ++meshIndex) {

    {
      // Create the vertex buffer.
      int sizeVertex =
          sizeof(VertexFormat) * theScene->Meshes[meshIndex]->getVertexCount();
      std::unique_ptr<int8_t[]> bytesVertex(new int8_t[sizeVertex]);
      theScene->Meshes[meshIndex]->copyVertices(
          reinterpret_cast<Vector3 *>(bytesVertex.get()), sizeof(VertexFormat));
      theScene->Meshes[meshIndex]->copyNormals(
          reinterpret_cast<Vector3 *>(bytesVertex.get() + sizeof(Vector3)),
          sizeof(VertexFormat));
      bufferMeshVertex.push_back(D3D11_Create_Buffer(
          device->GetID3D11Device(), D3D11_BIND_VERTEX_BUFFER, sizeVertex,
          bytesVertex.get()));
    }
    {
      // Create the index buffer.
      int sizeIndices =
          sizeof(int32_t) * theScene->Meshes[meshIndex]->getIndexCount();
      std::unique_ptr<int8_t[]> bytesIndex(new int8_t[sizeIndices]);
      theScene->Meshes[meshIndex]->copyIndices(
          reinterpret_cast<uint32_t *>(bytesIndex.get()), sizeof(uint32_t));
      bufferMeshIndex.push_back(D3D11_Create_Buffer(
          device->GetID3D11Device(), D3D11_BIND_INDEX_BUFFER, sizeIndices,
          bytesIndex.get()));
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
  std::function<void(ID3D11Texture2D *, ID3D11Texture2D *, const Matrix44 &)>
      fnRenderInto = [=](ID3D11Texture2D *textureBackbuffer,
                         ID3D11Texture2D *textureDepth,
                         const Matrix44 &transform) {
        D3D11_TEXTURE2D_DESC descBackbuffer = {};
        textureBackbuffer->GetDesc(&descBackbuffer);
        CComPtr<ID3D11RenderTargetView> rtvBackbuffer =
            D3D11_Create_RTV_From_Texture2D(device->GetID3D11Device(),
                                            textureBackbuffer);
        CComPtr<ID3D11DepthStencilView> dsvDepth;
        {
          D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
          desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
          desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
          TRYD3D(device->GetID3D11Device()->CreateDepthStencilView(
              textureDepth, &desc, &dsvDepth));
        }
        ////////////////////////////////////////////////////////////////////////////////
        // Build all instance constant buffers for the scene.
        std::vector<CComPtr<ID3D11Buffer>> bufferInstanceConstants;
        for (int instanceIndex = 0; instanceIndex < theScene->Instances.size();
             ++instanceIndex) {
          const Instance &instance = theScene->Instances[instanceIndex];
          // Create the constant buffer.
          {
            Constants constants = {};
            constants.TransformWorldToClip =
                instance.TransformObjectToWorld * transform;
            constants.TransformObjectToWorld = instance.TransformObjectToWorld;
            bufferInstanceConstants.push_back(D3D11_Create_Buffer(
                device->GetID3D11Device(), D3D11_BIND_CONSTANT_BUFFER,
                sizeof(Constants), &constants));
          }
        }
        ////////////////////////////////////////////////////////////////////////
        // Render everything.
        device->GetID3D11DeviceContext()->ClearState();
        device->GetID3D11DeviceContext()->ClearRenderTargetView(
            rtvBackbuffer, &std::array<FLOAT, 4>{0.1f, 0.1f, 0.1f, 1.0f}[0]);
        device->GetID3D11DeviceContext()->ClearDepthStencilView(
            dsvDepth, D3D10_CLEAR_DEPTH, 1.0f, 0);
        device->GetID3D11DeviceContext()->RSSetViewports(
            1,
            &Make_D3D11_VIEWPORT(descBackbuffer.Width, descBackbuffer.Height));
        device->GetID3D11DeviceContext()->OMSetRenderTargets(
            1, &rtvBackbuffer.p, dsvDepth);
        device->GetID3D11DeviceContext()->VSSetShader(shaderVertex, nullptr, 0);
        device->GetID3D11DeviceContext()->PSSetShader(shaderPixel, nullptr, 0);
        device->GetID3D11DeviceContext()->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        device->GetID3D11DeviceContext()->IASetInputLayout(inputLayout);
        for (int instanceIndex = 0; instanceIndex < theScene->Instances.size();
             ++instanceIndex) {
          const Instance &instance = theScene->Instances[instanceIndex];
          device->GetID3D11DeviceContext()->VSSetConstantBuffers(
              0, 1, &bufferInstanceConstants[instanceIndex].p);
          {
            const UINT vertexStride[] = {sizeof(VertexFormat)};
            const UINT vertexOffset[] = {0};
            device->GetID3D11DeviceContext()->IASetVertexBuffers(
                0, 1, &bufferMeshVertex[instance.GeometryIndex].p, vertexStride,
                vertexOffset);
          }
          device->GetID3D11DeviceContext()->IASetIndexBuffer(
              bufferMeshIndex[instance.GeometryIndex].p, DXGI_FORMAT_R32_UINT,
              0);
          device->GetID3D11DeviceContext()->DrawIndexed(
              theScene->Meshes[instance.GeometryIndex]->getIndexCount(), 0, 0);
        }
        device->GetID3D11DeviceContext()->ClearState();
        device->GetID3D11DeviceContext()->Flush();
      };

#ifdef USE_OPENVR
  {
    ////////////////////////////////////////////////////////////////////////////////
    // Initialize the VR system.
    vr::IVRSystem *vrsystem = vr::VR_Init(
        nullptr, vr::EVRApplicationType::VRApplication_Scene, nullptr);
    vr::IVRCompositor *vrcompositor = vr::VRCompositor();
    uint32_t viewwidth, viewheight;
    vrsystem->GetRecommendedRenderTargetSize(&viewwidth, &viewheight);
    CComPtr<ID3D11Texture2D> textureVR;
    {
      D3D11_TEXTURE2D_DESC desc = {};
      desc.Width = viewwidth;
      desc.Height = viewheight;
      desc.MipLevels = 1;
      desc.ArraySize = 1;
      desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
      desc.SampleDesc.Count = 1;
      desc.Usage = D3D11_USAGE_DEFAULT;
      desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
      TRYD3D(device->GetID3D11Device()->CreateTexture2D(&desc, nullptr,
                                                        &textureVR.p));
    }
    CComPtr<ID3D11Texture2D> textureDepth;
    {
      D3D11_TEXTURE2D_DESC desc = {};
      desc.Width = viewwidth;
      desc.Height = viewheight;
      desc.ArraySize = 1;
      desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
      desc.SampleDesc.Count = 1;
      desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
      TRYD3D(device->GetID3D11Device()->CreateTexture2D(&desc, nullptr,
                                                        &textureDepth));
    }
    std::array<vr::TrackedDevicePose_t, vr::k_unMaxTrackedDeviceCount>
        poseSystem = {};
    std::array<vr::TrackedDevicePose_t, vr::k_unMaxTrackedDeviceCount>
        poseGame = {};
    while (true) {
      if (vr::VRCompositorError_None !=
          vrcompositor->WaitGetPoses(&poseSystem[0], poseSystem.size(),
                                     &poseGame[0], poseGame.size()))
        continue;
      if (!poseSystem[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
        continue;

      Matrix44 world = CreateMatrixScale(Vector3{0.5f, 0.5f, -0.5f});
      Matrix44 head =
          Invert(ConvertMatrix(poseSystem[vr::k_unTrackedDeviceIndex_Hmd]
                                   .mDeviceToAbsoluteTracking));
      Matrix44 transformHeadToLeftEye =
          Invert(ConvertMatrix(vrsystem->GetEyeToHeadTransform(vr::Eye_Left)));
      Matrix44 transformHeadToRightEye =
          Invert(ConvertMatrix(vrsystem->GetEyeToHeadTransform(vr::Eye_Right)));
      Matrix44 projectionLeftEye = ConvertMatrix(
          vrsystem->GetProjectionMatrix(vr::Eye_Left, 0.001f, 100.0f));
      Matrix44 projectionRightEye = ConvertMatrix(
          vrsystem->GetProjectionMatrix(vr::Eye_Right, 0.001f, 100.0f));

      fnRenderInto(textureVR, textureDepth,
                   world * head * transformHeadToLeftEye * projectionLeftEye);
      {
        vr::Texture_t texture = {};
        texture.handle = textureVR.p;
        texture.eColorSpace = vr::ColorSpace_Auto;
        texture.eType = vr::TextureType_DirectX;
        vrcompositor->Submit(vr::Eye_Left, &texture);
      }
      fnRenderInto(textureVR, textureDepth,
                   world * head * transformHeadToRightEye * projectionRightEye);
      {
        vr::Texture_t texture = {};
        texture.handle = textureVR.p;
        texture.eColorSpace = vr::ColorSpace_Auto;
        texture.eType = vr::TextureType_DirectX;
        vrcompositor->Submit(vr::Eye_Right, &texture);
      }
    }
  }
#else
  return [=](ID3D11Texture2D *textureBackbuffer) {
    D3D11_TEXTURE2D_DESC descBackbuffer = {};
    textureBackbuffer->GetDesc(&descBackbuffer);
    ////////////////////////////////////////////////////////////////////////////////
    // Create an appropriately sized depth buffer and its view.
    CComPtr<ID3D11Texture2D> textureDepth;
    {
      D3D11_TEXTURE2D_DESC desc = {};
      desc.Width = descBackbuffer.Width;
      desc.Height = descBackbuffer.Height;
      desc.ArraySize = 1;
      desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
      desc.SampleDesc.Count = 1;
      desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
      TRYD3D(device->GetID3D11Device()->CreateTexture2D(&desc, nullptr,
                                                        &textureDepth));
    }
    fnRenderInto(textureBackbuffer, textureDepth,
                 GetTransformSource()->GetTransformWorldToClip());
  };
#endif // USE_OPENVR
}