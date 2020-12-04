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
#include "Core_Util.h"
#include "Scene_Camera.h"
#include "Scene_InstanceTable.h"
#include "Scene_Mesh.h"
#include <atlbase.h>
#include <functional>
#include <memory>

std::function<void(ID3D12Resource *)>
CreateSample_D3D12Scene(std::shared_ptr<Direct3D12Device> device) {
  CComPtr<ID3D12RootSignature> rootSignature =
      D3D12_Create_Signature_1CBV(device->m_pDevice);
  CComPtr<ID3D12DescriptorHeap> descriptorHeapCBVSRVUAV =
      D3D12_Create_DescriptorHeap_CBVSRVUAV(device->m_pDevice, 256);
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
  std::shared_ptr<InstanceTable> theScene(InstanceTable::Default());
  std::vector<CComPtr<ID3D12Resource1>> resourceVertex;
  std::vector<CComPtr<ID3D12Resource1>> resourceIndex;
  for (int i = 0; i < theScene->Meshes.size(); ++i) {
    {
      int sizeVertex = sizeof(float[3]) * theScene->Meshes[i]->getVertexCount();
      std::unique_ptr<int8_t[]> vertices(new int8_t[sizeVertex]);
      theScene->Meshes[i]->copyVertices(
          reinterpret_cast<Vector3 *>(vertices.get()), sizeof(Vector3));
      resourceVertex.push_back(D3D12_Create_Buffer(
          device.get(), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON,
          sizeVertex, sizeVertex, vertices.get()));
    }
    {
      int sizeIndices = sizeof(int32_t) * theScene->Meshes[i]->getIndexCount();
      std::unique_ptr<int8_t[]> indices(new int8_t[sizeIndices]);
      theScene->Meshes[i]->copyIndices(
          reinterpret_cast<uint32_t *>(indices.get()), sizeof(uint32_t));
      resourceIndex.push_back(D3D12_Create_Buffer(
          device.get(), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON,
          sizeIndices, sizeIndices, indices.get()));
    }
  }
  return [=](ID3D12Resource *resourceBackbuffer) {
    std::vector<CComPtr<ID3D12Resource1>> resourceConstants;
    for (int i = 0; i < theScene->Instances.size(); ++i) {
      resourceConstants.push_back(
          D3D12_Create_Buffer(device.get(), D3D12_RESOURCE_FLAG_NONE,
                              D3D12_RESOURCE_STATE_COMMON, 256, 256,
                              &(theScene->Instances[i].TransformObjectToWorld *
                                GetCameraWorldToClip())));
      {
        D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
        desc.BufferLocation = resourceConstants[i]->GetGPUVirtualAddress();
        desc.SizeInBytes = 256;
        D3D12_CPU_DESCRIPTOR_HANDLE handle =
            descriptorHeapCBVSRVUAV->GetCPUDescriptorHandleForHeapStart();
        handle.ptr =
            handle.ptr + device->m_pDevice->GetDescriptorHandleIncrementSize(
                             D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) *
                             i;
        device->m_pDevice->CreateConstantBufferView(&desc, handle);
      }
    }
    device->m_pDevice->CreateRenderTargetView(
        resourceBackbuffer,
        &Make_D3D12_RENDER_TARGET_VIEW_DESC_SwapChainDefault(),
        device->m_pDescriptorHeapRTV->GetCPUDescriptorHandleForHeapStart());
    D3D12_Run_Synchronously(device.get(), [&](ID3D12GraphicsCommandList5
                                                  *commandList) {
      commandList->SetGraphicsRootSignature(rootSignature);
      ID3D12DescriptorHeap *descriptorHeaps[] = {descriptorHeapCBVSRVUAV};
      commandList->SetDescriptorHeaps(1, descriptorHeaps);
      // Put the RTV into render target state and clear it before use.
      commandList->ResourceBarrier(
          1, &Make_D3D12_RESOURCE_BARRIER(resourceBackbuffer,
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
          1, &Make_D3D12_VIEWPORT(RENDERTARGET_WIDTH, RENDERTARGET_HEIGHT));
      commandList->RSSetScissorRects(
          1, &Make_D3D12_RECT(RENDERTARGET_WIDTH, RENDERTARGET_HEIGHT));
      commandList->OMSetRenderTargets(
          1,
          &device->m_pDescriptorHeapRTV->GetCPUDescriptorHandleForHeapStart(),
          FALSE, nullptr);
      for (int i = 0; i < theScene->Instances.size(); ++i) {
        D3D12_GPU_DESCRIPTOR_HANDLE handle =
            descriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart();
        handle.ptr =
            handle.ptr + device->m_pDevice->GetDescriptorHandleIncrementSize(
                             D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) *
                             i;
        commandList->SetGraphicsRootDescriptorTable(0, handle);
        int32_t meshIndex = theScene->Instances[i].GeometryIndex;
        {
          D3D12_VERTEX_BUFFER_VIEW desc = {};
          desc.BufferLocation =
              resourceVertex[meshIndex]->GetGPUVirtualAddress();
          desc.SizeInBytes =
              sizeof(float[3]) * theScene->Meshes[meshIndex]->getVertexCount();
          desc.StrideInBytes = sizeof(float[3]);
          commandList->IASetVertexBuffers(0, 1, &desc);
        }
        {
          D3D12_INDEX_BUFFER_VIEW desc = {};
          desc.BufferLocation =
              resourceIndex[meshIndex]->GetGPUVirtualAddress();
          desc.SizeInBytes =
              sizeof(int32_t) * theScene->Meshes[meshIndex]->getIndexCount();
          desc.Format = DXGI_FORMAT_R32_UINT;
          commandList->IASetIndexBuffer(&desc);
        }
        commandList->DrawIndexedInstanced(
            theScene->Meshes[meshIndex]->getIndexCount(), 1, 0, 0, 0);
      }
      // Transition the render target into presentation state for display.
      commandList->ResourceBarrier(
          1, &Make_D3D12_RESOURCE_BARRIER(resourceBackbuffer,
                                          D3D12_RESOURCE_STATE_RENDER_TARGET,
                                          D3D12_RESOURCE_STATE_PRESENT));
    });
  };
}