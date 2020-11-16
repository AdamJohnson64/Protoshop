#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include "Core_DXRUtil.h"
#include "Sample_DXRBase.h"
#include <array>
#include <atlbase.h>
#include <d3d12.h>

Sample_DXRBase::Sample_DXRBase(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D12Device> pDevice) :
    m_pSwapChain(pSwapChain),
    m_pDevice(pDevice)
{
    m_pRootSignature = DXR_Create_Signature_1UAV1SRV(pDevice->m_pDevice);
    m_pResourceTargetUAV = DXR_Create_Output_UAV(pDevice->m_pDevice);
    m_pDescriptorHeapCBVSRVUAV = D3D12_Create_DescriptorHeap_CBVSRVUAV(pDevice->m_pDevice, 256);
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
    m_pDevice->m_pDevice->CreateRenderTargetView(pD3D12Resource, &Make_D3D12_RENDER_TARGET_VIEW_DESC_SwapChainDefault(), m_pDevice->m_pDescriptorHeapRTV->GetCPUDescriptorHandleForHeapStart());
    ////////////////////////////////////////////////////////////////////////////////
    // Copy the raytracer output from UAV to the back-buffer.
    ////////////////////////////////////////////////////////////////////////////////
    RunOnGPU(m_pDevice.get(), [&](ID3D12GraphicsCommandList5* RaytraceCommandList) {
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