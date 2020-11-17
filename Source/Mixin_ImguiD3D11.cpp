#include "Core_D3D.h"
#include "Mixin_ImguiD3D11.h"
#include <backends/imgui_impl_dx11.h>

Mixin_ImguiD3D11::Mixin_ImguiD3D11(std::shared_ptr<Direct3D11Device> pDevice) :
    Mixin_ImguiBase()
{
    ImGui::SetCurrentContext(pImgui);
    ImGui_ImplDX11_Init(pDevice->GetID3D11Device(), pDevice->GetID3D11DeviceContext());
}

Mixin_ImguiD3D11::~Mixin_ImguiD3D11()
{
    ImGui::SetCurrentContext(pImgui);
    ImGui_ImplDX11_Shutdown();
}

void Mixin_ImguiD3D11::RenderImgui()
{
    // Draw some UI with Dear ImGui.
    ImGui::SetCurrentContext(pImgui);
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDrawCursor = true;
    io.DisplaySize.x = RENDERTARGET_WIDTH;
    io.DisplaySize.y = RENDERTARGET_HEIGHT;
    ImGui_ImplDX11_NewFrame();
    ImGui::NewFrame();
    BuildImguiUI();
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}