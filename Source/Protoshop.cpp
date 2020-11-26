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
        std::shared_ptr<Direct3D11Device> deviceD3D11 = CreateDirect3D11Device();
        std::shared_ptr<Window> D3D11Basic = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_D3D11Basic(CreateDXGISwapChain(deviceD3D11, hwnd), deviceD3D11); });
        std::shared_ptr<Window> D3D11ComputeCanvas = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_D3D11ComputeCanvas(CreateDXGISwapChain(deviceD3D11, hwnd), deviceD3D11); });
        std::shared_ptr<Window> D3D11DrawingContext = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_D3D11DrawingContext(CreateDXGISwapChain(deviceD3D11, hwnd), deviceD3D11); });
        std::shared_ptr<Window> D3D11Mesh = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_D3D11Mesh(CreateDXGISwapChain(deviceD3D11, hwnd), deviceD3D11); });
        std::shared_ptr<Window> D3D11RayMarch = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_D3D11RayMarch(CreateDXGISwapChain(deviceD3D11, hwnd), deviceD3D11); });
        std::shared_ptr<Window> D3D11Tessellation = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_D3D11Tessellation(CreateDXGISwapChain(deviceD3D11, hwnd), deviceD3D11); });
        //std::shared_ptr<Window> D3D11Voxel = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_D3D11Voxel(CreateDXGISwapChain(pDevice11, hwnd), pDevice11); });
        //std::shared_ptr<Object> D3DShaderToy = CreateSample_D3D11ShaderToy(pDevice11);
        ////////////////////////////////////////////////////////////////////////////////
        // Direct3D 12 Samples
        std::shared_ptr<Direct3D12Device> deviceD3D12 = CreateDirect3D12Device();
        std::shared_ptr<Window> D3D12Basic = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_D3D12Basic(CreateDXGISwapChain(deviceD3D12, hwnd), deviceD3D12); });
        std::shared_ptr<Window> D3D12Mesh = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_D3D12Mesh(CreateDXGISwapChain(deviceD3D12, hwnd), deviceD3D12); });
        std::shared_ptr<Window> DXRAmbientOcclusion = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_DXRAmbientOcclusion(CreateDXGISwapChain(deviceD3D12, hwnd), deviceD3D12); });
        std::shared_ptr<Window> DXRBasic = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_DXRBasic(CreateDXGISwapChain(deviceD3D12, hwnd), deviceD3D12); });
        std::shared_ptr<Window> DXRMesh = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_DXRMesh(CreateDXGISwapChain(deviceD3D12, hwnd), deviceD3D12); });
        std::shared_ptr<Window> DXRPathTrace = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_DXRPathTrace(CreateDXGISwapChain(deviceD3D12, hwnd), deviceD3D12); });
        std::shared_ptr<Window> DXRScene = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_DXRScene(CreateDXGISwapChain(deviceD3D12, hwnd), deviceD3D12); });
        std::shared_ptr<Window> DXRTexture = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_DXRTexture(CreateDXGISwapChain(deviceD3D12, hwnd), deviceD3D12); });
        std::shared_ptr<Window> DXRWhitted = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_DXRWhitted(CreateDXGISwapChain(deviceD3D12, hwnd), deviceD3D12); });
        ////////////////////////////////////////////////////////////////////////////////
        // OpenGL Samples
        std::shared_ptr<OpenGLDevice> deviceGL = CreateOpenGLDevice();
        std::shared_ptr<Window> GLBasic = CreateSampleInWindow([&](std::shared_ptr<Window> wnd) { return CreateSample_OpenGLBasic(deviceGL, wnd); });
#if VULKAN_INSTALLED
        ////////////////////////////////////////////////////////////////////////////////
        // Vulkan Samples
        std::shared_ptr<VKDevice> deviceVK = CreateVKDevice();
        std::shared_ptr<Window> VKBasic = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_VKBasic(CreateDXGISwapChain(deviceD3D12, hwnd), deviceVK, deviceD3D12); });
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