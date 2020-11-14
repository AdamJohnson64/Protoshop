///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 ImGui
///////////////////////////////////////////////////////////////////////////////
// This sample demonstrates how to use the popular ImGui immediate-mode UI
// library in the context of a Direct3D 11 application.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Mixin_ImguiD3D11.h"
#include "Sample_D3D11Base.h"
#include <memory>

class Sample_D3D11Imgui : public Sample_D3D11Base, public Mixin_ImguiD3D11
{
public:
    Sample_D3D11Imgui(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D11Device> pDevice) :
        Sample_D3D11Base(pSwapChain, pDevice),
        Mixin_ImguiD3D11(pDevice)
    {
    }
    void RenderSample() override
    {
        RenderImgui();
    }
    void BuildImguiUI() override
    {
        ImGui::Text("Dear ImGui running on Direct3D 11.");
    }
};

std::shared_ptr<Sample> CreateSample_D3D11Imgui(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D11Device> pDevice)
{
    return std::shared_ptr<Sample>(new Sample_D3D11Imgui(pSwapChain, pDevice));
}