#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Sample_D3D11Base.h"
#include "examples/imgui_impl_dx11.h"
#include <atlbase.h>
#include <memory>

class Sample_D3D11Imgui : public Sample_D3D11Base
{
private:
    ImGuiContext* pImgui;
public:
    Sample_D3D11Imgui(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D11Device> pDevice) :
        Sample_D3D11Base(pSwapChain, pDevice),
        pImgui(ImGui::CreateContext())
    {
        ImGui::SetCurrentContext(pImgui);
        ImGui_ImplDX11_Init(pDevice->GetID3D11Device(), pDevice->GetID3D11DeviceContext());
    }
    ~Sample_D3D11Imgui()
    {
        ImGui::SetCurrentContext(pImgui);
        ImGui_ImplDX11_Shutdown();
        ImGui::DestroyContext(pImgui);
    }
    void RenderSample() override
    {
        // Draw some UI with Dear ImGui.
        ImGui::SetCurrentContext(pImgui);
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize.x = RENDERTARGET_WIDTH;
        io.DisplaySize.y = RENDERTARGET_HEIGHT;
        ImGui_ImplDX11_NewFrame();
        ImGui::NewFrame();
        ImGui::Text("Dear ImGui running on Direct3D 11.");
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
};

std::shared_ptr<Sample> CreateSample_D3D11Imgui(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D11Device> pDevice)
{
    return std::shared_ptr<Sample>(new Sample_D3D11Imgui(pSwapChain, pDevice));
}