#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3D12.h"
#include "Core_DXGI.h"
#include "Core_Object.h"
#include "Core_OpenGL.h"
#include "Core_OpenVR.h"
#include "Core_VK.h"
#include "Core_Window.h"
#include "Sample_Manifest.h"
#include <Windows.h>
#include <functional>
#include <memory>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd) {
  try {
    // clang-format off
    ////////////////////////////////////////////////////////////////////////////////
    // Direct3D 11 Samples
    std::shared_ptr<Direct3D11Device> deviceD3D11 = CreateDirect3D11Device();
    //CreateNewOpenVRSession(deviceD3D11, CreateSample_D3D11Scene(deviceD3D11, Scene_Sponza()));
    std::shared_ptr<Object> D3D11Basic = CreateNewWindow(deviceD3D11, CreateSample_D3D11Basic(deviceD3D11));
    std::shared_ptr<Object> D3D11ComputeCanvas = CreateNewWindow(deviceD3D11, CreateSample_D3D11ComputeCanvas(deviceD3D11));
    std::shared_ptr<Object> D3D11DrawingContext = CreateNewWindow(deviceD3D11, CreateSample_D3D11DrawingContext(deviceD3D11));
    //std::shared_ptr<Object> D3D11LightProbe = CreateNewWindow(deviceD3D11, CreateSample_D3D11LightProbe(deviceD3D11));
    std::shared_ptr<Object> D3D11LightProbeCross = CreateNewWindow(deviceD3D11, CreateSample_D3D11LightProbeCross(deviceD3D11));
    std::shared_ptr<Object> D3D11Mesh = CreateNewWindow(deviceD3D11, CreateSample_D3D11Mesh(deviceD3D11));
    std::shared_ptr<Object> D3D11NormalMap = CreateNewWindow(deviceD3D11, CreateSample_D3D11NormalMap(deviceD3D11));
    std::shared_ptr<Object> D3D11ParallaxMap = CreateNewWindow(deviceD3D11, CreateSample_D3D11ParallaxMap(deviceD3D11));
    std::shared_ptr<Object> D3D11RayMarch = CreateNewWindow(deviceD3D11, CreateSample_D3D11RayMarch(deviceD3D11));
    std::shared_ptr<Object> D3D11SceneDefault = CreateNewWindow(deviceD3D11, CreateSample_D3D11Scene(deviceD3D11, Scene_Default()));
    std::shared_ptr<Object> D3D11SceneSponza = CreateNewWindow(deviceD3D11, CreateSample_D3D11Scene(deviceD3D11, Scene_Sponza()));
    std::shared_ptr<Object> D3D11ShowTexture = CreateNewWindow(deviceD3D11, CreateSample_D3D11ShowTexture(deviceD3D11));
    std::shared_ptr<Object> D3D11Tessellation = CreateNewWindow(deviceD3D11, CreateSample_D3D11Tessellation(deviceD3D11));
    //std::shared_ptr<Object> D3D11Voxel = CreateNewWindow(deviceD3D11, CreateSample_D3D11Voxel(deviceD3D11));
    //std::shared_ptr<Object> D3DShaderToy = CreateSample_D3D11ShaderToy(deviceD3D11);
    ////////////////////////////////////////////////////////////////////////////////
    // Direct3D 12 Samples
    std::shared_ptr<Direct3D12Device> deviceD3D12 = CreateDirect3D12Device();
    std::shared_ptr<Object> D3D12Basic = CreateNewWindowRTV(deviceD3D12, CreateSample_D3D12Basic(deviceD3D12));
    std::shared_ptr<Object> D3D12SceneDefault = CreateNewWindowRTV(deviceD3D12, CreateSample_D3D12Scene(deviceD3D12, Scene_Default()));
    //std::shared_ptr<Object> D3D12SceneSponza = CreateNewWindowRTV(deviceD3D12, CreateSample_D3D12Scene(deviceD3D12, Scene_Sponza()));
    std::shared_ptr<Object> DXRAmbientOcclusion = CreateNewWindowUAV(deviceD3D12, CreateSample_DXRAmbientOcclusion(deviceD3D12));
    std::shared_ptr<Object> DXRBasic = CreateNewWindowUAV(deviceD3D12, CreateSample_DXRBasic(deviceD3D12));
    std::shared_ptr<Object> DXRMesh = CreateNewWindowUAV(deviceD3D12, CreateSample_DXRMesh(deviceD3D12));
    std::shared_ptr<Object> DXRPathTrace = CreateNewWindowUAV(deviceD3D12, CreateSample_DXRPathTrace(deviceD3D12));
    std::shared_ptr<Object> DXRSceneDefault = CreateNewWindowUAV(deviceD3D12, CreateSample_DXRScene(deviceD3D12, Scene_Default()));
    std::shared_ptr<Object> DXRSceneSponza = CreateNewWindowUAV(deviceD3D12, CreateSample_DXRScene(deviceD3D12, Scene_Sponza()));
    std::shared_ptr<Object> DXRTexture = CreateNewWindowUAV(deviceD3D12, CreateSample_DXRTexture(deviceD3D12));
    std::shared_ptr<Object> DXRWhitted = CreateNewWindowUAV(deviceD3D12, CreateSample_DXRWhitted(deviceD3D12));
    ////////////////////////////////////////////////////////////////////////////////
    // OpenGL Samples
    std::shared_ptr<OpenGLDevice> deviceGL = CreateOpenGLDevice();
    std::shared_ptr<Object> GLBasic = CreateNewWindow(deviceGL, CreateSample_OpenGLBasic(deviceGL));
#if VULKAN_INSTALLED
    ////////////////////////////////////////////////////////////////////////////////
    // Vulkan Samples
    std::shared_ptr<VKDevice> deviceVK = CreateVKDevice();
    std::shared_ptr<Object> VKBasic = CreateNewWindow(deviceVK, deviceD3D12, CreateSample_VKBasic());
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