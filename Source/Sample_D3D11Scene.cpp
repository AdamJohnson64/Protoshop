///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Mesh
///////////////////////////////////////////////////////////////////////////////
// This sample demonstrates how to render a triangle using the simplest shader
// setups and a common camera.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D11Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_ISample.h"
#include "Core_Object.h"
#include "Core_Util.h"
#include "Scene_Camera.h"
#include "Scene_InstanceTable.h"
#include "Scene_Mesh.h"
#include <array>
#include <atlbase.h>
#include <functional>
#include <vector>

class Sample_D3D11Scene : public Object, public ISample {
private:
  std::function<void()> m_fnRender;

public:
  Sample_D3D11Scene(std::shared_ptr<DXGISwapChain> m_pSwapChain,
                    std::shared_ptr<Direct3D11Device> m_pDevice) {
    struct Constants {
      Matrix44 TransformWorldToClip;
      Matrix44 TransformObjectToWorld;
    };
    struct VertexFormat {
      Vector3 Position;
      Vector3 Normal;
    };
    ////////////////////////////////////////////////////////////////////////////////
    // Create an appropriately sized depth buffer and its view.
    CComPtr<ID3D11DepthStencilView> srvDepth;
    {
      CComPtr<ID3D11Texture2D> textureDepth;
      {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = RENDERTARGET_WIDTH;
        desc.Height = RENDERTARGET_HEIGHT;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.SampleDesc.Count = 1;
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        TRYD3D(m_pDevice->GetID3D11Device()->CreateTexture2D(&desc, nullptr,
                                                             &textureDepth));
      }
      {
        D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
        desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        TRYD3D(m_pDevice->GetID3D11Device()->CreateDepthStencilView(
            textureDepth, &desc, &srvDepth));
      }
    }
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
    TRYD3D(m_pDevice->GetID3D11Device()->CreateVertexShader(
        blobShaderVertex->GetBufferPointer(), blobShaderVertex->GetBufferSize(),
        nullptr, &shaderVertex));
    CComPtr<ID3D11PixelShader> shaderPixel;
    {
      ID3DBlob *blob = CompileShader("ps_5_0", "mainPS", szShaderCode);
      TRYD3D(m_pDevice->GetID3D11Device()->CreatePixelShader(
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
      TRYD3D(m_pDevice->GetID3D11Device()->CreateInputLayout(
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
        int sizeVertex = sizeof(VertexFormat) *
                         theScene->Meshes[meshIndex]->getVertexCount();
        std::unique_ptr<int8_t[]> bytesVertex(new int8_t[sizeVertex]);
        theScene->Meshes[meshIndex]->copyVertices(
            reinterpret_cast<Vector3 *>(bytesVertex.get()),
            sizeof(VertexFormat));
        theScene->Meshes[meshIndex]->copyNormals(
            reinterpret_cast<Vector3 *>(bytesVertex.get() + sizeof(Vector3)),
            sizeof(VertexFormat));
        bufferMeshVertex.push_back(D3D11_Create_Buffer(
            m_pDevice->GetID3D11Device(), D3D11_BIND_VERTEX_BUFFER, sizeVertex,
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
            m_pDevice->GetID3D11Device(), D3D11_BIND_INDEX_BUFFER, sizeIndices,
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
    m_fnRender = [=]() {
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
              instance.TransformObjectToWorld * GetCameraWorldToClip();
          constants.TransformObjectToWorld = instance.TransformObjectToWorld;
          bufferInstanceConstants.push_back(D3D11_Create_Buffer(
              m_pDevice->GetID3D11Device(), D3D11_BIND_CONSTANT_BUFFER,
              sizeof(Constants), &constants));
        }
      }
      ////////////////////////////////////////////////////////////////////////////////
      // Get the backbuffer and create a render target from it.
      CComPtr<ID3D11RenderTargetView> rtvBackbuffer =
          D3D11_Create_RTV_From_SwapChain(m_pDevice->GetID3D11Device(),
                                          m_pSwapChain->GetIDXGISwapChain());
      ////////////////////////////////////////////////////////////////////////////////
      // Beginning of rendering.
      m_pDevice->GetID3D11DeviceContext()->ClearState();
      m_pDevice->GetID3D11DeviceContext()->ClearRenderTargetView(
          rtvBackbuffer, &std::array<FLOAT, 4>{0.1f, 0.1f, 0.1f, 1.0f}[0]);
      m_pDevice->GetID3D11DeviceContext()->ClearDepthStencilView(
          srvDepth, D3D10_CLEAR_DEPTH, 1.0f, 0);
      m_pDevice->GetID3D11DeviceContext()->RSSetViewports(
          1, &Make_D3D11_VIEWPORT(RENDERTARGET_WIDTH, RENDERTARGET_HEIGHT));
      m_pDevice->GetID3D11DeviceContext()->OMSetRenderTargets(
          1, &rtvBackbuffer.p, srvDepth);
      m_pDevice->GetID3D11DeviceContext()->VSSetShader(shaderVertex, nullptr,
                                                       0);
      m_pDevice->GetID3D11DeviceContext()->PSSetShader(shaderPixel, nullptr, 0);
      m_pDevice->GetID3D11DeviceContext()->IASetPrimitiveTopology(
          D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      m_pDevice->GetID3D11DeviceContext()->IASetInputLayout(inputLayout);
      for (int instanceIndex = 0; instanceIndex < theScene->Instances.size();
           ++instanceIndex) {
        const Instance &instance = theScene->Instances[instanceIndex];
        m_pDevice->GetID3D11DeviceContext()->VSSetConstantBuffers(
            0, 1, &bufferInstanceConstants[instanceIndex].p);
        {
          const UINT vertexStride[] = {sizeof(VertexFormat)};
          const UINT vertexOffset[] = {0};
          m_pDevice->GetID3D11DeviceContext()->IASetVertexBuffers(
              0, 1, &bufferMeshVertex[instance.GeometryIndex].p, vertexStride,
              vertexOffset);
        }
        m_pDevice->GetID3D11DeviceContext()->IASetIndexBuffer(
            bufferMeshIndex[instance.GeometryIndex].p, DXGI_FORMAT_R32_UINT, 0);
        m_pDevice->GetID3D11DeviceContext()->DrawIndexed(
            theScene->Meshes[instance.GeometryIndex]->getIndexCount(), 0, 0);
      }
      m_pDevice->GetID3D11DeviceContext()->ClearState();
      m_pDevice->GetID3D11DeviceContext()->Flush();
      // End of rendering; send to display.
      m_pSwapChain->GetIDXGISwapChain()->Present(0, 0);
    };
  }
  void Render() override { m_fnRender(); }
};

std::shared_ptr<ISample>
CreateSample_D3D11Scene(std::shared_ptr<DXGISwapChain> swapchain,
                        std::shared_ptr<Direct3D11Device> device) {
  return std::shared_ptr<ISample>(new Sample_D3D11Scene(swapchain, device));
}