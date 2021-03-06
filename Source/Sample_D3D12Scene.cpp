///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 12 Scene
///////////////////////////////////////////////////////////////////////////////
// This sample demonstrates how to render a Direct3D 12 scene from an abstract
// scene description. Nobody would actually implement rendering in this manner
// without some sort of static buffer and resource management, and we would
// expect to see some buffer recycling for pipelined frames.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_Math.h"
#include "Core_Util.h"
#include "MutableMap.h"
#include "SampleResources.h"
#include "Scene_IMesh.h"
#include "Scene_InstanceTable.h"
#include <atlbase.h>
#include <functional>
#include <memory>

std::function<void(const SampleResourcesD3D12RTV &)>
CreateSample_D3D12Scene(std::shared_ptr<Direct3D12Device> device,
                        const std::vector<Instance> &scene) {
  CComPtr<ID3D12RootSignature> rootSignature =
      D3D12_Create_Signature_1CBV(device->m_pDevice);
  CComPtr<ID3D12DescriptorHeap> descriptorHeapCBVSRVUAV =
      D3D12_Create_DescriptorHeap_CBVSRVUAV(device->m_pDevice, 4096);
  ////////////////////////////////////////////////////////////////////////////////
  // Create a vertex shader.
  CComPtr<ID3DBlob> pD3DBlobCodeVS = CompileShader("vs_5_0", "main", R"SHADER(
cbuffer Constants
{
    float4x4 TransformWorldToClip;
};

float4 main(float4 pos : SV_Position) : SV_Position
{
        return mul(TransformWorldToClip, pos);
})SHADER");

  ////////////////////////////////////////////////////////////////////////////////
  // Create a pixel shader.
  ID3DBlob *pD3DBlobCodePS = CompileShader("ps_5_0", "main", R"SHADER(
float4 main() : SV_Target
{
        return float4(1, 1, 1, 1);
})SHADER");

  ////////////////////////////////////////////////////////////////////////////////
  // Create a pipeline with these shaders.
  CComPtr<ID3D12PipelineState> pipelineState;
  {
    D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipeline = {};
    descPipeline.pRootSignature = rootSignature;
    descPipeline.VS.pShaderBytecode = pD3DBlobCodeVS->GetBufferPointer();
    descPipeline.VS.BytecodeLength = pD3DBlobCodeVS->GetBufferSize();
    descPipeline.PS.pShaderBytecode = pD3DBlobCodePS->GetBufferPointer();
    descPipeline.PS.BytecodeLength = pD3DBlobCodePS->GetBufferSize();
    descPipeline.BlendState.RenderTarget->RenderTargetWriteMask = 0xF;
    descPipeline.SampleMask = 0xFFFFFFFF;
    descPipeline.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    descPipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    D3D12_INPUT_ELEMENT_DESC descElement = {};
    descElement.SemanticName = "SV_Position";
    descElement.Format = DXGI_FORMAT_R32G32B32_FLOAT;
    descPipeline.InputLayout.pInputElementDescs = &descElement;
    descPipeline.InputLayout.NumElements = 1;
    descPipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    descPipeline.NumRenderTargets = 1;
    descPipeline.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
    descPipeline.SampleDesc.Count = 1;
    TRYD3D(device->m_pDevice->CreateGraphicsPipelineState(
        &descPipeline, __uuidof(ID3D12PipelineState),
        (void **)&pipelineState.p));
  }

  MutableMap<const IMesh *, CComPtr<ID3D12Resource1>> factoryVertex;
  factoryVertex.fnGenerator = [=](const IMesh *mesh) {
    int sizeVertex = sizeof(float[3]) * mesh->getVertexCount();
    std::unique_ptr<int8_t[]> vertices(new int8_t[sizeVertex]);
    mesh->copyVertices(reinterpret_cast<Vector3 *>(vertices.get()),
                       sizeof(Vector3));
    return D3D12_Create_Buffer(device.get(), D3D12_RESOURCE_FLAG_NONE,
                               D3D12_RESOURCE_STATE_COMMON, sizeVertex,
                               sizeVertex, vertices.get());
  };

  MutableMap<const IMesh *, CComPtr<ID3D12Resource1>> factoryIndex;
  factoryIndex.fnGenerator = [=](const IMesh *mesh) {
    int sizeIndices = sizeof(int32_t) * mesh->getIndexCount();
    std::unique_ptr<int8_t[]> indices(new int8_t[sizeIndices]);
    mesh->copyIndices(reinterpret_cast<uint32_t *>(indices.get()),
                      sizeof(uint32_t));
    return D3D12_Create_Buffer(device.get(), D3D12_RESOURCE_FLAG_NONE,
                               D3D12_RESOURCE_STATE_COMMON, sizeIndices,
                               sizeIndices, indices.get());
  };

  return [=](const SampleResourcesD3D12RTV &sampleResources) {
    D3D12_RESOURCE_DESC descBackbuffer =
        sampleResources.BackBufferResource->GetDesc();
    device->m_pDevice->CreateRenderTargetView(
        sampleResources.BackBufferResource,
        &Make_D3D12_RENDER_TARGET_VIEW_DESC_SwapChainDefault(),
        device->m_pDescriptorHeapRTV->GetCPUDescriptorHandleForHeapStart());

    MutableMap<const Matrix44 *, CComPtr<ID3D12Resource1>> factoryConstants;
    factoryConstants.fnGenerator = [=](const Matrix44 *transform) {
      return D3D12_Create_Buffer(
          device.get(), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON,
          256, 256, &(*transform * sampleResources.TransformWorldToClip));
    };

    D3D12_Run_Synchronously(device.get(), [&](ID3D12GraphicsCommandList5
                                                  *commandList) {
      commandList->SetGraphicsRootSignature(rootSignature);
      ID3D12DescriptorHeap *descriptorHeaps[] = {descriptorHeapCBVSRVUAV};
      commandList->SetDescriptorHeaps(1, descriptorHeaps);
      // Put the RTV into render target state and clear it before use.
      commandList->ResourceBarrier(
          1, &Make_D3D12_RESOURCE_BARRIER(sampleResources.BackBufferResource,
                                          D3D12_RESOURCE_STATE_COMMON,
                                          D3D12_RESOURCE_STATE_RENDER_TARGET));
      {
        float color[4] = {0.25f, 0.25f, 0.25f, 1};
        commandList->ClearRenderTargetView(
            device->m_pDescriptorHeapRTV->GetCPUDescriptorHandleForHeapStart(),
            color, 0, nullptr);
      }
      commandList->SetPipelineState(pipelineState);
      commandList->IASetPrimitiveTopology(
          D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      commandList->RSSetViewports(
          1, &Make_D3D12_VIEWPORT(descBackbuffer.Width, descBackbuffer.Height));
      commandList->RSSetScissorRects(
          1, &Make_D3D12_RECT(descBackbuffer.Width, descBackbuffer.Height));
      commandList->OMSetRenderTargets(
          1,
          &device->m_pDescriptorHeapRTV->GetCPUDescriptorHandleForHeapStart(),
          FALSE, nullptr);
      for (int i = 0; i < scene.size(); ++i) {
        const Instance &instance = scene[i];
        {
          D3D12_CPU_DESCRIPTOR_HANDLE handle =
              descriptorHeapCBVSRVUAV->GetCPUDescriptorHandleForHeapStart();
          handle.ptr =
              handle.ptr + device->m_pDevice->GetDescriptorHandleIncrementSize(
                               D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) *
                               i;
          device->m_pDevice->CreateConstantBufferView(
              &Make_D3D12_CONSTANT_BUFFER_VIEW_DESC(
                  factoryConstants(instance.TransformObjectToWorld.get()), 256),
              handle);
        }
        D3D12_GPU_DESCRIPTOR_HANDLE handle =
            descriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart();
        handle.ptr =
            handle.ptr + device->m_pDevice->GetDescriptorHandleIncrementSize(
                             D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) *
                             i;
        commandList->SetGraphicsRootDescriptorTable(0, handle);
        {
          D3D12_VERTEX_BUFFER_VIEW desc = {};
          desc.BufferLocation =
              factoryVertex(instance.Mesh.get())->GetGPUVirtualAddress();
          desc.SizeInBytes = sizeof(float[3]) * scene[i].Mesh->getVertexCount();
          desc.StrideInBytes = sizeof(float[3]);
          commandList->IASetVertexBuffers(0, 1, &desc);
        }
        {
          D3D12_INDEX_BUFFER_VIEW desc = {};
          desc.BufferLocation =
              factoryIndex(instance.Mesh.get())->GetGPUVirtualAddress();
          desc.SizeInBytes = sizeof(int32_t) * scene[i].Mesh->getIndexCount();
          desc.Format = DXGI_FORMAT_R32_UINT;
          commandList->IASetIndexBuffer(&desc);
        }
        commandList->DrawIndexedInstanced(instance.Mesh->getIndexCount(), 1, 0,
                                          0, 0);
      }
      // Transition the render target into presentation state for display.
      commandList->ResourceBarrier(
          1, &Make_D3D12_RESOURCE_BARRIER(sampleResources.BackBufferResource,
                                          D3D12_RESOURCE_STATE_RENDER_TARGET,
                                          D3D12_RESOURCE_STATE_PRESENT));
    });
  };
}