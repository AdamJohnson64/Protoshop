#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include "Sample_DXRBase.h"
#include <array>
#include <atlbase.h>
#include <d3d12.h>

Sample_DXRBase::Sample_DXRBase(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D12Device> pDevice) :
    m_pSwapChain(pSwapChain),
    m_pDevice(pDevice)
{
    ////////////////////////////////////////////////////////////////////////////////
    // Create a LOCAL root signature.
    {
        uint32_t setupRange = 0;

        std::array<D3D12_DESCRIPTOR_RANGE, 8> descDescriptorRange;

        descDescriptorRange[setupRange].BaseShaderRegister = 0;
        descDescriptorRange[setupRange].NumDescriptors = 1;
        descDescriptorRange[setupRange].RegisterSpace = 0;
        descDescriptorRange[setupRange].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        descDescriptorRange[setupRange].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        ++setupRange;

        descDescriptorRange[setupRange].BaseShaderRegister = 0;
        descDescriptorRange[setupRange].NumDescriptors = 1;
        descDescriptorRange[setupRange].RegisterSpace = 0;
        descDescriptorRange[setupRange].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        descDescriptorRange[setupRange].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        ++setupRange;

        descDescriptorRange[setupRange].BaseShaderRegister = 0;
        descDescriptorRange[setupRange].NumDescriptors = 1;
        descDescriptorRange[setupRange].RegisterSpace = 0;
        descDescriptorRange[setupRange].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        descDescriptorRange[setupRange].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        ++setupRange;

        D3D12_DESCRIPTOR_RANGE descDescriptorRange2 = {};
        descDescriptorRange2.BaseShaderRegister = 1;
        descDescriptorRange2.NumDescriptors = 1;
        descDescriptorRange2.RegisterSpace = 0;
        descDescriptorRange2.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        descDescriptorRange2.OffsetInDescriptorsFromTableStart = 0;

        std::array<D3D12_ROOT_PARAMETER, 2> descRootParameter = {};
        descRootParameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        descRootParameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        descRootParameter[0].DescriptorTable.NumDescriptorRanges = setupRange;
        descRootParameter[0].DescriptorTable.pDescriptorRanges = &descDescriptorRange[0];

        descRootParameter[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        descRootParameter[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        descRootParameter[1].Constants.ShaderRegister = 1;
        descRootParameter[1].Constants.Num32BitValues = 4;

        D3D12_ROOT_SIGNATURE_DESC descSignature = {};
        descSignature.NumParameters = 2;
        descSignature.pParameters = &descRootParameter[0];
        descSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

        CComPtr<ID3DBlob> m_blob;
        CComPtr<ID3DBlob> m_blobError;
        TRYD3D(D3D12SerializeRootSignature(&descSignature, D3D_ROOT_SIGNATURE_VERSION_1_0, &m_blob, &m_blobError));
        TRYD3D(m_pDevice->m_pDevice->CreateRootSignature(0, m_blob->GetBufferPointer(), m_blob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&m_pRootSignature));
        m_pRootSignature->SetName(L"DXR Root Signature");
    }
    ////////////////////////////////////////////////////////////////////////////////
    // Create the unordered access buffer for raytracing output.
    {
        D3D12_HEAP_PROPERTIES descHeapProperties = {};
        descHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC descResource = {};
        descResource.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        descResource.Width = RENDERTARGET_WIDTH;
        descResource.Height = RENDERTARGET_HEIGHT;
        descResource.DepthOrArraySize = 1;
        descResource.MipLevels = 1;
        descResource.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        descResource.SampleDesc.Count = 1;
        descResource.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        TRYD3D(m_pDevice->m_pDevice->CreateCommittedResource(&descHeapProperties, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &descResource, D3D12_RESOURCE_STATE_COMMON, nullptr, __uuidof(ID3D12Resource1), (void**)&m_pResourceTargetUAV));
        m_pResourceTargetUAV->SetName(L"DXR Output Texture2D UAV");
    }
    ////////////////////////////////////////////////////////////////////////////////
    // Create a descriptor heap for the SRVs.
    {
        D3D12_DESCRIPTOR_HEAP_DESC descDescriptorHeap = {};
        descDescriptorHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        descDescriptorHeap.NumDescriptors = 256;
        descDescriptorHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        TRYD3D(m_pDevice->m_pDevice->CreateDescriptorHeap(&descDescriptorHeap, __uuidof(ID3D12DescriptorHeap), (void**)&m_pDescriptorHeapCBVSRVUAV));
        m_pDescriptorHeapCBVSRVUAV->SetName(L"D3D12DescriptorHeap (CBV/SRV/UAV)");
    }
}

void Sample_DXRBase::Render()
{
    RenderSample();
    ////////////////////////////////////////////////////////////////////////////////
    // Get the next available backbuffer.
    ////////////////////////////////////////////////////////////////////////////////
    CComPtr<ID3D12Resource> pD3D12Resource;
    TRYD3D(m_pSwapChain->GetIDXGISwapChain()->GetBuffer(m_pSwapChain->GetIDXGISwapChain()->GetCurrentBackBufferIndex(), __uuidof(ID3D12Resource), (void**)&pD3D12Resource));
    pD3D12Resource->SetName(L"D3D12Resource (Backbuffer)");
    {
        D3D12_RENDER_TARGET_VIEW_DESC descRTV = {};
        descRTV.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        descRTV.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        m_pDevice->m_pDevice->CreateRenderTargetView(pD3D12Resource, &descRTV, m_pDevice->m_pDescriptorHeapRTV->GetCPUDescriptorHandleForHeapStart());
    }
    ////////////////////////////////////////////////////////////////////////////////
    // Copy the raytracer output from UAV to the back-buffer.
    ////////////////////////////////////////////////////////////////////////////////
    RunOnGPU(m_pDevice, [&](ID3D12GraphicsCommandList5* RaytraceCommandList) {
        RaytraceCommandList->ResourceBarrier(1, &D3D12MakeResourceTransitionBarrier(m_pResourceTargetUAV, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE));
        RaytraceCommandList->ResourceBarrier(1, &D3D12MakeResourceTransitionBarrier(pD3D12Resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
        RaytraceCommandList->CopyResource(pD3D12Resource, m_pResourceTargetUAV);
        RaytraceCommandList->ResourceBarrier(1, &D3D12MakeResourceTransitionBarrier(m_pResourceTargetUAV, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON));
        // Note: We're going to detour through RenderTarget state since we may need to scribble some UI over the top.
        RaytraceCommandList->ResourceBarrier(1, &D3D12MakeResourceTransitionBarrier(pD3D12Resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET));
        // Set up Rasterizer Stage (RS) for the viewport and scissor.
        RaytraceCommandList->RSSetViewports(1, &D3D12MakeViewport(RENDERTARGET_WIDTH, RENDERTARGET_HEIGHT));
        RaytraceCommandList->RSSetScissorRects(1, &D3D12MakeRect(RENDERTARGET_WIDTH, RENDERTARGET_HEIGHT));
        // Set up the Output Merger (OM) to define the target to render into.
        RaytraceCommandList->OMSetRenderTargets(1, &m_pDevice->m_pDescriptorHeapRTV->GetCPUDescriptorHandleForHeapStart(), FALSE, nullptr);
        RenderPost(RaytraceCommandList);
        // Note: Now we can transition back.
        RaytraceCommandList->ResourceBarrier(1, &D3D12MakeResourceTransitionBarrier(pD3D12Resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON));
    });
    // Swap the backbuffer and send this to the desktop composer for display.
    TRYD3D(m_pSwapChain->GetIDXGISwapChain()->Present(0, 0));
}

void Sample_DXRBase::RenderPost(ID3D12GraphicsCommandList5* pCommandList)
{
}