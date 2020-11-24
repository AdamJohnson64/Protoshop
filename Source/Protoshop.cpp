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

std::shared_ptr<Window> CreateSampleInWindow(std::function<std::shared_ptr<Sample>(std::shared_ptr<Window>)> createsample)
{
    std::shared_ptr<Window> window(CreateNewWindow());
    window->SetSample(createsample(window));
    return window;
}

std::shared_ptr<Window> CreateSampleInWindow(std::function<std::shared_ptr<Sample>(HWND)> createsample)
{
    return CreateSampleInWindow([&](std::shared_ptr<Window> w) { return createsample(w->GetWindowHandle()); });
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    try
    {
        ////////////////////////////////////////////////////////////////////////////////
        // Direct3D 11 Samples
        std::shared_ptr<Direct3D11Device> pDevice11 = CreateDirect3D11Device();
        std::shared_ptr<Window> pWindowD3D11Basic = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_D3D11Basic(CreateDXGISwapChain(pDevice11, hwnd), pDevice11); });
        std::shared_ptr<Window> pWindowD3D11ComputeCanvas = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_D3D11ComputeCanvas(CreateDXGISwapChain(pDevice11, hwnd), pDevice11); });
        std::shared_ptr<Window> pWindowD3D11DrawingContext = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_D3D11DrawingContext(CreateDXGISwapChain(pDevice11, hwnd), pDevice11); });
        std::shared_ptr<Window> pWindowD3D11RayMarch = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_D3D11RayMarch(CreateDXGISwapChain(pDevice11, hwnd), pDevice11); });
        std::shared_ptr<Window> pWindowD3D11Scene = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_D3D11Scene(CreateDXGISwapChain(pDevice11, hwnd), pDevice11); });
        std::shared_ptr<Window> pWindowD3D11Tessellation = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_D3D11Tessellation(CreateDXGISwapChain(pDevice11, hwnd), pDevice11); });
        //std::shared_ptr<Window> pWindowD3D11Voxel = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_D3D11Voxel(CreateDXGISwapChain(pDevice11, hwnd), pDevice11); });
        //std::shared_ptr<Object> pWindowD3DShaderToy = CreateSample_D3D11ShaderToy(pDevice11);
        ////////////////////////////////////////////////////////////////////////////////
        // Direct3D 12 Samples
        std::shared_ptr<Direct3D12Device> pDevice12 = CreateDirect3D12Device();
        std::shared_ptr<Window> pWindowD3D12Basic = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_D3D12Basic(CreateDXGISwapChain(pDevice12, hwnd), pDevice12); });
        std::shared_ptr<Window> pWindowD3D12Mesh = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_D3D12Mesh(CreateDXGISwapChain(pDevice12, hwnd), pDevice12); });
        std::shared_ptr<Window> pWindowDXRAmbientOcclusion = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_DXRAmbientOcclusion(CreateDXGISwapChain(pDevice12, hwnd), pDevice12); });
        std::shared_ptr<Window> pWindowDXRBasic = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_DXRBasic(CreateDXGISwapChain(pDevice12, hwnd), pDevice12); });
        std::shared_ptr<Window> pWindowDXRMesh = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_DXRMesh(CreateDXGISwapChain(pDevice12, hwnd), pDevice12); });
        std::shared_ptr<Window> pWindowDXRPathTrace = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_DXRPathTrace(CreateDXGISwapChain(pDevice12, hwnd), pDevice12); });
        std::shared_ptr<Window> pWindowDXRTexture = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_DXRTexture(CreateDXGISwapChain(pDevice12, hwnd), pDevice12); });
        std::shared_ptr<Window> pWindowDXRWhitted = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_DXRWhitted(CreateDXGISwapChain(pDevice12, hwnd), pDevice12); });
        ////////////////////////////////////////////////////////////////////////////////
        // OpenGL Samples
        std::shared_ptr<OpenGLDevice> pDeviceGL = CreateOpenGLDevice();
        std::shared_ptr<Window> pWindowGLBasic = CreateSampleInWindow([&](std::shared_ptr<Window> wnd) { return CreateSample_OpenGLBasic(pDeviceGL, wnd); });
#if VULKAN_INSTALLED
        ////////////////////////////////////////////////////////////////////////////////
        // Vulkan Samples
        std::shared_ptr<VKDevice> pDeviceVK = CreateVKDevice();
        std::shared_ptr<Window> pWindowVKBasic = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_VKBasic(CreateDXGISwapChain(pDevice12, hwnd), pDeviceVK, pDevice12); });
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