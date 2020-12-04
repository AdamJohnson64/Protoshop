#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3D12.h"
#include "Core_DXGI.h"
#include "Core_IWindow.h"
#include "Core_Object.h"
#include "Core_OpenGL.h"
#include "Core_VK.h"
#include "Sample_Manifest.h"
#include <Windows.h>
#include <functional>
#include <memory>

std::shared_ptr<IWindow> CreateSampleInWindow(
    std::function<std::shared_ptr<ISample>(std::shared_ptr<IWindow>)>
        createsample) {
  std::shared_ptr<IWindow> window(CreateNewWindow());
  window->SetSample(createsample(window));
  return window;
}

std::shared_ptr<IWindow> CreateSampleInWindow(
    std::function<std::shared_ptr<ISample>(HWND)> createsample) {
  return CreateSampleInWindow([&](std::shared_ptr<IWindow> w) {
    return createsample(w->GetWindowHandle());
  });
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd) {
  try {
    // clang-format off
    ////////////////////////////////////////////////////////////////////////////////
    // Direct3D 11 Samples
    std::shared_ptr<Direct3D11Device> deviceD3D11 = CreateDirect3D11Device();
    std::shared_ptr<Object> D3D11Basic = CreateNewWindow(deviceD3D11, CreateSample_D3D11Basic(deviceD3D11));
    std::shared_ptr<Object> D3D11ComputeCanvas = CreateNewWindow(deviceD3D11, CreateSample_D3D11ComputeCanvas(deviceD3D11));
    std::shared_ptr<Object> D3D11DrawingContext = CreateNewWindow(deviceD3D11, CreateSample_D3D11DrawingContext(deviceD3D11));
    //std::shared_ptr<Object> D3D11LightProbe = CreateNewWindow(deviceD3D11, CreateSample_D3D11LightProbe(deviceD3D11));
    std::shared_ptr<Object> D3D11Mesh = CreateNewWindow(deviceD3D11, CreateSample_D3D11Mesh(deviceD3D11));
    std::shared_ptr<Object> D3D11NormalMap = CreateNewWindow(deviceD3D11, CreateSample_D3D11NormalMap(deviceD3D11));
    std::shared_ptr<Object> D3D11ParallaxMap = CreateNewWindow(deviceD3D11, CreateSample_D3D11ParallaxMap(deviceD3D11));
    std::shared_ptr<Object> D3D11RayMarch = CreateNewWindow(deviceD3D11, CreateSample_D3D11RayMarch(deviceD3D11));
    std::shared_ptr<Object> D3D11Scene = CreateNewWindow(deviceD3D11, CreateSample_D3D11Scene(deviceD3D11));
    std::shared_ptr<Object> D3D11ShowTexture = CreateNewWindow(deviceD3D11, CreateSample_D3D11ShowTexture(deviceD3D11));
    std::shared_ptr<Object> D3D11Tessellation = CreateNewWindow(deviceD3D11, CreateSample_D3D11Tessellation(deviceD3D11));
    //std::shared_ptr<Object> D3D11Voxel = CreateNewWindow(deviceD3D11, CreateSample_D3D11Voxel(deviceD3D11));
    //std::shared_ptr<Object> D3DShaderToy = CreateSample_D3D11ShaderToy(deviceD3D11);
    ////////////////////////////////////////////////////////////////////////////////
    // Direct3D 12 Samples
    std::shared_ptr<Direct3D12Device> deviceD3D12 = CreateDirect3D12Device();
    std::shared_ptr<IWindow> D3D12Basic = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_D3D12Basic(CreateDXGISwapChain(deviceD3D12, hwnd), deviceD3D12); });
    std::shared_ptr<IWindow> D3D12Scene = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_D3D12Scene(CreateDXGISwapChain(deviceD3D12, hwnd), deviceD3D12); });
    std::shared_ptr<IWindow> DXRAmbientOcclusion = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_DXRAmbientOcclusion(CreateDXGISwapChain(deviceD3D12, hwnd), deviceD3D12); });
    std::shared_ptr<IWindow> DXRBasic = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_DXRBasic(CreateDXGISwapChain(deviceD3D12, hwnd), deviceD3D12); });
    std::shared_ptr<IWindow> DXRMesh = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_DXRMesh(CreateDXGISwapChain(deviceD3D12, hwnd), deviceD3D12); });
    std::shared_ptr<IWindow> DXRPathTrace = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_DXRPathTrace(CreateDXGISwapChain(deviceD3D12, hwnd), deviceD3D12); });
    std::shared_ptr<IWindow> DXRScene = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_DXRScene(CreateDXGISwapChain(deviceD3D12, hwnd), deviceD3D12); });
    std::shared_ptr<IWindow> DXRTexture = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_DXRTexture(CreateDXGISwapChain(deviceD3D12, hwnd), deviceD3D12); });
    std::shared_ptr<IWindow> DXRWhitted = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_DXRWhitted(CreateDXGISwapChain(deviceD3D12, hwnd), deviceD3D12); });
    ////////////////////////////////////////////////////////////////////////////////
    // OpenGL Samples
    std::shared_ptr<OpenGLDevice> deviceGL = CreateOpenGLDevice();
    std::shared_ptr<IWindow> GLBasic = CreateSampleInWindow([&](std::shared_ptr<IWindow> wnd) { return CreateSample_OpenGLBasic(deviceGL, wnd); });
#if VULKAN_INSTALLED
    ////////////////////////////////////////////////////////////////////////////////
    // Vulkan Samples
    std::shared_ptr<VKDevice> deviceVK = CreateVKDevice();
    std::shared_ptr<IWindow> VKBasic = CreateSampleInWindow([&](HWND hwnd) { return CreateSample_VKBasic(CreateDXGISwapChain(deviceD3D12, hwnd), deviceVK, deviceD3D12); });
#endif // VULKAN_INSTALLED
    // clang-format on
    {
      MSG Msg = {};
      while (GetMessage(&Msg, nullptr, 0, 0)) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
      }
    }
    return 0;
  } catch (const std::exception &ex) {
    OutputDebugStringA(ex.what());
    return -1;
  } catch (...) {
    return -1;
  }
}