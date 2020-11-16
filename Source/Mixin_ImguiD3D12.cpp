#include "Core_D3D.h"
#include "Core_D3D12Util.h"
#include "Mixin_ImguiD3D12.h"
#include "examples/imgui_impl_dx12.h"

Mixin_ImguiD3D12::Mixin_ImguiD3D12(std::shared_ptr<Direct3D12Device> pDevice) :
    m_pDeviceImgui(pDevice)
{
    m_pDescriptorHeapCBVSRVUAVImGui = D3D12_Create_DescriptorHeap_CBVSRVUAV(pDevice->m_pDevice, 16);
    ImGui_ImplDX12_Init(pDevice->m_pDevice, 1, DXGI_FORMAT_B8G8R8A8_UNORM, m_pDescriptorHeapCBVSRVUAVImGui, m_pDescriptorHeapCBVSRVUAVImGui->GetCPUDescriptorHandleForHeapStart(), m_pDescriptorHeapCBVSRVUAVImGui->GetGPUDescriptorHandleForHeapStart());
}

Mixin_ImguiD3D12::~Mixin_ImguiD3D12()
{
    ImGui::SetCurrentContext(pImgui);
    ImGui_ImplDX12_Shutdown();
}

void Mixin_ImguiD3D12::RenderImgui(ID3D12GraphicsCommandList5* pD3D12GraphicsCommandList)
{
    ImGui::SetCurrentContext(pImgui);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = RENDERTARGET_WIDTH;
    io.DisplaySize.y = RENDERTARGET_HEIGHT;
    ImGui_ImplDX12_NewFrame();
    ImGui::NewFrame();
    BuildImguiUI();
    ImGui::Render();
    pD3D12GraphicsCommandList->SetDescriptorHeaps(1, &m_pDescriptorHeapCBVSRVUAVImGui.p);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pD3D12GraphicsCommandList);
}