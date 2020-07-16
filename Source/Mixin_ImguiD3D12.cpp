#include "Core_D3D.h"
#include "Mixin_ImguiD3D12.h"
#include "examples/imgui_impl_dx12.h"

Mixin_ImguiD3D12::Mixin_ImguiD3D12(std::shared_ptr<Direct3D12Device> pDevice)
{
    ID3D12DescriptorHeap* pHeap = pDevice->GetID3D12DescriptorHeapCBVSRVUAV();
    ImGui_ImplDX12_Init(pDevice->GetID3D12Device(), 1, DXGI_FORMAT_B8G8R8A8_UNORM, pHeap, pHeap->GetCPUDescriptorHandleForHeapStart(), pHeap->GetGPUDescriptorHandleForHeapStart());
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
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pD3D12GraphicsCommandList);
}