#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3D12.h"
#include "Core_DXGI.h"
#include "Core_Object.h"
#include "Core_OpenGL.h"
#include "Core_VK.h"
#include "Core_Window.h"
#include "Sample_Manifest.h"
#include <Windows.h>
#include <functional>
#include <memory>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    try
    {
        ////////////////////////////////////////////////////////////////////////////////
        // Direct3D 11 Samples
        std::shared_ptr<Direct3D11Device> pDevice11 = CreateDirect3D11Device();

        std::shared_ptr<Window> pWindowD3D11Basic(CreateNewWindow());
        pWindowD3D11Basic->SetSample(CreateSample_D3D11Basic(CreateDXGISwapChain(pDevice11, pWindowD3D11Basic->GetWindowHandle()), pDevice11));
        
        std::shared_ptr<Window> pWindowD3D11ComputeCanvas(CreateNewWindow());
        pWindowD3D11ComputeCanvas->SetSample(CreateSample_D3D11ComputeCanvas(CreateDXGISwapChain(pDevice11, pWindowD3D11ComputeCanvas->GetWindowHandle()), pDevice11));
        
        std::shared_ptr<Window> pWindowD3D11DrawingContext(CreateNewWindow());
        pWindowD3D11DrawingContext->SetSample(CreateSample_D3D11DrawingContext(CreateDXGISwapChain(pDevice11, pWindowD3D11DrawingContext->GetWindowHandle()), pDevice11));
        
        std::shared_ptr<Window> pWindowD3D11Imgui(CreateNewWindow());
        pWindowD3D11Imgui->SetSample(CreateSample_D3D11Imgui(CreateDXGISwapChain(pDevice11, pWindowD3D11Imgui->GetWindowHandle()), pDevice11));
        
        std::shared_ptr<Window> pWindowD3D11Scene(CreateNewWindow());
        pWindowD3D11Scene->SetSample(CreateSample_D3D11Scene(CreateDXGISwapChain(pDevice11, pWindowD3D11Scene->GetWindowHandle()), pDevice11));
        
        std::shared_ptr<Window> pWindowD3D11Tessellation(CreateNewWindow());
        pWindowD3D11Tessellation->SetSample(CreateSample_D3D11Tessellation(CreateDXGISwapChain(pDevice11, pWindowD3D11Tessellation->GetWindowHandle()), pDevice11));

        ////////////////////////////////////////////////////////////////////////////////
        // Direct3D 12 Samples
        std::shared_ptr<Direct3D12Device> pDevice12 = CreateDirect3D12Device();

        std::shared_ptr<Window> pWindowD3D12Basic(CreateNewWindow());
        pWindowD3D12Basic->SetSample(CreateSample_D3D12Basic(CreateDXGISwapChain(pDevice12, pWindowD3D12Basic->GetWindowHandle()), pDevice12));

        // BUG: The stock ImGui DX12 adaptor doesn't support multiple simultaneous contexts (yet).
        //std::shared_ptr<Window> pWindowD3D12Imgui(CreateNewWindow());
        //pWindowD3D12Imgui->SetSample(CreateSample_D3D12Imgui(CreateDXGISwapChain(pDevice12, pWindowD3D12Imgui->GetWindowHandle()), pDevice12));

        std::shared_ptr<Window> pWindowD3D12Mesh(CreateNewWindow());
        pWindowD3D12Mesh->SetSample(CreateSample_D3D12Mesh(CreateDXGISwapChain(pDevice12, pWindowD3D12Mesh->GetWindowHandle()), pDevice12));
        
        std::shared_ptr<Window> pWindowDXRAmbientOcclusion(CreateNewWindow());
        pWindowDXRAmbientOcclusion->SetSample(CreateSample_DXRAmbientOcclusion(CreateDXGISwapChain(pDevice12, pWindowDXRAmbientOcclusion->GetWindowHandle()), pDevice12));
        
        std::shared_ptr<Window> pWindowDXRBasic(CreateNewWindow());
        pWindowDXRBasic->SetSample(CreateSample_DXRBasic(CreateDXGISwapChain(pDevice12, pWindowDXRBasic->GetWindowHandle()), pDevice12));
        
        std::shared_ptr<Window> pWindowDXRMesh(CreateNewWindow());
        pWindowDXRMesh->SetSample(CreateSample_DXRMesh(CreateDXGISwapChain(pDevice12, pWindowDXRMesh->GetWindowHandle()), pDevice12));
        
        std::shared_ptr<Window> pWindowDXRPathTrace(CreateNewWindow());
        pWindowDXRPathTrace->SetSample(CreateSample_DXRPathTrace(CreateDXGISwapChain(pDevice12, pWindowDXRPathTrace->GetWindowHandle()), pDevice12));

        std::shared_ptr<Window> pWindowDXRTexture(CreateNewWindow());
        pWindowDXRTexture->SetSample(CreateSample_DXRTexture(CreateDXGISwapChain(pDevice12, pWindowDXRTexture->GetWindowHandle()), pDevice12));

        std::shared_ptr<Window> pWindowDXRWhitted(CreateNewWindow());
        pWindowDXRWhitted->SetSample(CreateSample_DXRWhitted(CreateDXGISwapChain(pDevice12, pWindowDXRWhitted->GetWindowHandle()), pDevice12));
        
        ////////////////////////////////////////////////////////////////////////////////
        // OpenGL Samples

        std::shared_ptr<OpenGLDevice> pDeviceGL = CreateOpenGLDevice();

        std::shared_ptr<Window> pWindowGLBasic(CreateNewWindow());
        pWindowGLBasic->SetSample(CreateSample_OpenGLBasic(pDeviceGL, pWindowGLBasic));

#if VULKAN_INSTALLED
        ////////////////////////////////////////////////////////////////////////////////
        // Vulkan Samples

        std::shared_ptr<VKDevice> pDeviceVK = CreateVKDevice();

        std::shared_ptr<Window> pWindowVKBasic(CreateNewWindow());
        pWindowVKBasic->SetSample(CreateSample_VKBasic(CreateDXGISwapChain(pDevice12, pWindowVKBasic->GetWindowHandle()), pDeviceVK, pDevice12));
#endif // VULKAN_INSTALLED
        {
            MSG Msg = {};
            while (GetMessage(&Msg, nullptr, 0, 0))
            {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
        }
        return 0;
    }
    catch (const std::exception& ex)
    {
        OutputDebugStringA(ex.what());
        return -1;
    }
    catch (...)
    {
        return -1;
    }
}