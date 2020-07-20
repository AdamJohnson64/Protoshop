#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Sample.h"
#include "Scene_Camera.h"
#include "Scene_InstanceTable.h"
#include <atlbase.h>
#include <functional>
#include <memory>

class Sample_D3D12Mesh : public Sample
{
private:
    std::shared_ptr<DXGISwapChain> m_pSwapChain;
    std::shared_ptr<Direct3D12Device> m_pDevice;
    CComPtr<ID3D12PipelineState> m_pPipelineState;
public:
    Sample_D3D12Mesh(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D12Device> pDevice) :
        m_pSwapChain(pSwapChain),
        m_pDevice(pDevice)
    {
        ////////////////////////////////////////////////////////////////////////////////
        // Create a vertex shader.
        CComPtr<ID3DBlob> pD3DBlobCodeVS = CompileShader("vs_5_0", "main", R"SHADER(
cbuffer Constants
{
    float4x4 transform;
};

float4 main(float4 pos : SV_Position) : SV_Position
{
        return mul(pos, transform);
})SHADER");

        ////////////////////////////////////////////////////////////////////////////////
        // Create a pixel shader.
        ID3DBlob* pD3DBlobCodePS = CompileShader("ps_5_0", "main", R"SHADER(
float4 main() : SV_Target
{
        return float4(1, 1, 1, 1);
})SHADER");

        ////////////////////////////////////////////////////////////////////////////////
        // Create a pipeline with these shaders.
        {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipeline = {};
            descPipeline.pRootSignature = pDevice->GetID3D12RootSignature();
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
            TRYD3D(pDevice->GetID3D12Device()->CreateGraphicsPipelineState(&descPipeline, __uuidof(ID3D12PipelineState), (void**)&m_pPipelineState.p));
        }
    }
    void Render() override
    {
        std::shared_ptr<InstanceTable> scene(InstanceTable::Default());
        CComPtr<ID3D12Resource> pD3D12Resource;
        TRYD3D(m_pSwapChain->GetIDXGISwapChain()->GetBuffer(m_pSwapChain->GetIDXGISwapChain()->GetCurrentBackBufferIndex(), __uuidof(ID3D12Resource), (void**)&pD3D12Resource));
        pD3D12Resource->SetName(L"D3D12Resource (Backbuffer)");
        {
            D3D12_RENDER_TARGET_VIEW_DESC descRTV = {};
            descRTV.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            descRTV.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            m_pDevice->GetID3D12Device()->CreateRenderTargetView(pD3D12Resource, &descRTV, m_pDevice->GetID3D12DescriptorHeapRTV()->GetCPUDescriptorHandleForHeapStart());
        }
        std::vector<CComPtr<ID3D12Resource>> vertexBuffers;
        std::vector<CComPtr<ID3D12Resource>> indexBuffers;
        for (int i = 0; i < scene->Meshes.size(); ++i)
        {
            {
                int sizeVertex = sizeof(float[3]) * scene->Meshes[i]->getVertexCount();
                std::unique_ptr<int8_t[]> vertices(new int8_t[sizeVertex]);
                scene->Meshes[i]->copyVertices(reinterpret_cast<Vector3*>(vertices.get()), sizeof(Vector3));
                CComPtr<ID3D12Resource> vertexBuffer;
                vertexBuffer.p = D3D12CreateBuffer(m_pDevice, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, sizeVertex, sizeVertex, vertices.get());
                vertexBuffers.push_back(vertexBuffer.Detach());
            }
            {
                int sizeIndices = sizeof(int32_t) * scene->Meshes[i]->getIndexCount();
                std::unique_ptr<int8_t[]> indices(new int8_t[sizeIndices]);
                scene->Meshes[i]->copyIndices(reinterpret_cast<uint32_t*>(indices.get()), sizeof(uint32_t));
                CComPtr<ID3D12Resource> indexBuffer;
                indexBuffer.p = D3D12CreateBuffer(m_pDevice, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, sizeIndices, sizeIndices, indices.get());
                indexBuffers.push_back(indexBuffer.Detach());
            }
        }
        std::vector<CComPtr<ID3D12Resource>> constantBuffers;
        for (int i = 0; i < scene->Instances.size(); ++i)
        {
            {
                CComPtr<ID3D12Resource> constantBuffer;
                constantBuffer.p = D3D12CreateBuffer(m_pDevice, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, 256, 256, &Transpose(scene->Instances[i].Transform * GetCameraViewProjection()));
                constantBuffers.push_back(constantBuffer.Detach());
            }
            {
                D3D12_CONSTANT_BUFFER_VIEW_DESC descConstantBuffer = {};
                descConstantBuffer.BufferLocation = constantBuffers[i]->GetGPUVirtualAddress();
                descConstantBuffer.SizeInBytes = 256;
                D3D12_CPU_DESCRIPTOR_HANDLE handle = m_pDevice->GetID3D12DescriptorHeapCBVSRVUAV()->GetCPUDescriptorHandleForHeapStart();
                handle.ptr = handle.ptr + m_pDevice->GetID3D12Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * i;
                m_pDevice->GetID3D12Device()->CreateConstantBufferView(&descConstantBuffer, handle);
            }
        }
        RunOnGPU(m_pDevice, [&](ID3D12GraphicsCommandList5* pD3D12GraphicsCommandList)
        {
            pD3D12GraphicsCommandList->SetGraphicsRootSignature(m_pDevice->GetID3D12RootSignature());
            ID3D12DescriptorHeap* descriptorHeaps[] = { m_pDevice->GetID3D12DescriptorHeapCBVSRVUAV() };
            pD3D12GraphicsCommandList->SetDescriptorHeaps(1, descriptorHeaps);
            // Put the RTV into render target state and clear it before use.
            {
                D3D12_RESOURCE_BARRIER descBarrier = {};
                descBarrier.Transition.pResource = pD3D12Resource;
                descBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
                descBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
                pD3D12GraphicsCommandList->ResourceBarrier(1, &descBarrier);
            }
            {
                float color[4] = { 0.25f, 0.25f, 0.25f, 1 };
                pD3D12GraphicsCommandList->ClearRenderTargetView(m_pDevice->GetID3D12DescriptorHeapRTV()->GetCPUDescriptorHandleForHeapStart(), color, 0, nullptr);
            }
            pD3D12GraphicsCommandList->SetPipelineState(m_pPipelineState);
            pD3D12GraphicsCommandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            {
                D3D12_VIEWPORT descViewport = {};
                descViewport.Width = RENDERTARGET_WIDTH;
                descViewport.Height = RENDERTARGET_HEIGHT;
                descViewport.MaxDepth = 1.0f;
                pD3D12GraphicsCommandList->RSSetViewports(1, &descViewport);
            }
            {
                D3D12_RECT descRect = {};
                descRect.right = RENDERTARGET_WIDTH;
                descRect.bottom = RENDERTARGET_HEIGHT;
                pD3D12GraphicsCommandList->RSSetScissorRects(1, &descRect);
            }
            pD3D12GraphicsCommandList->OMSetRenderTargets(1, &m_pDevice->GetID3D12DescriptorHeapRTV()->GetCPUDescriptorHandleForHeapStart(), FALSE, nullptr);
            for (int i = 0; i < scene->Instances.size(); ++i)
            {
                D3D12_GPU_DESCRIPTOR_HANDLE handle = m_pDevice->GetID3D12DescriptorHeapCBVSRVUAV()->GetGPUDescriptorHandleForHeapStart();
                handle.ptr = handle.ptr + m_pDevice->GetID3D12Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * i;
                pD3D12GraphicsCommandList->SetGraphicsRootDescriptorTable(DESCRIPTOR_HEAP_CBV, handle);
                int32_t meshIndex = scene->Instances[i].GeometryIndex;
                {
                    D3D12_VERTEX_BUFFER_VIEW descVertexBufferView = {};
                    descVertexBufferView.BufferLocation = vertexBuffers[meshIndex]->GetGPUVirtualAddress();
                    descVertexBufferView.SizeInBytes = sizeof(float[3]) * scene->Meshes[meshIndex]->getVertexCount();
                    descVertexBufferView.StrideInBytes = sizeof(float[3]);
                    pD3D12GraphicsCommandList->IASetVertexBuffers(0, 1, &descVertexBufferView);
                }
                {
                    D3D12_INDEX_BUFFER_VIEW descIndexBufferView = {};
                    descIndexBufferView.BufferLocation = indexBuffers[meshIndex]->GetGPUVirtualAddress();
                    descIndexBufferView.SizeInBytes = sizeof(int32_t) * scene->Meshes[meshIndex]->getIndexCount();
                    descIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
                    pD3D12GraphicsCommandList->IASetIndexBuffer(&descIndexBufferView);
                }
                pD3D12GraphicsCommandList->DrawIndexedInstanced(scene->Meshes[meshIndex]->getIndexCount(), 1, 0, 0, 0);
            }
            // Transition the render target into presentation state for display.
            {
                D3D12_RESOURCE_BARRIER descBarrier = {};
                descBarrier.Transition.pResource = pD3D12Resource;
                descBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
                descBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
                pD3D12GraphicsCommandList->ResourceBarrier(1, &descBarrier);
            }
        });
        // Swap the backbuffer and send this to the desktop composer for display.
        TRYD3D(m_pSwapChain->GetIDXGISwapChain()->Present(0, 0));
    }
};

std::shared_ptr<Sample> CreateSample_D3D12Mesh(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D12Device> pDevice)
{
    return std::shared_ptr<Sample>(new Sample_D3D12Mesh(pSwapChain, pDevice));
}