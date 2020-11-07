#include "Core_D3D.h"
#include "Mixin_ImguiD3D12.h"
#include "examples/imgui_impl_dx12.h"

Mixin_ImguiD3D12::Mixin_ImguiD3D12(std::shared_ptr<Direct3D12Device> pDevice) :
    m_pDeviceImgui(pDevice)
{
    ////////////////////////////////////////////////////////////////////////////////
    // Create a descriptor heap for private Imgui resources.
    {
        D3D12_DESCRIPTOR_HEAP_DESC descDescriptorHeap = {};
        descDescriptorHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        descDescriptorHeap.NumDescriptors = 32;
        descDescriptorHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        TRYD3D(pDevice->m_pDevice->CreateDescriptorHeap(&descDescriptorHeap, __uuidof(ID3D12DescriptorHeap), (void**)&pD3D12DescriptorHeapImgui));
        pD3D12DescriptorHeapImgui->SetName(L"D3D12DescriptorHeap (Imgui)");
    }
    ImGui_ImplDX12_Init(pDevice->m_pDevice, 1, DXGI_FORMAT_B8G8R8A8_UNORM, pD3D12DescriptorHeapImgui, pD3D12DescriptorHeapImgui->GetCPUDescriptorHandleForHeapStart(), pD3D12DescriptorHeapImgui->GetGPUDescriptorHandleForHeapStart());
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
    ID3D12DescriptorHeap* descriptorHeaps[] = { pD3D12DescriptorHeapImgui };
    pD3D12GraphicsCommandList->SetDescriptorHeaps(1, descriptorHeaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pD3D12GraphicsCommandList);
}