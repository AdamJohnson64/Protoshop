#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3D12.h"
#include "Core_DXGI.h"
#include "Core_Object.h"
#include "Core_OpenGL.h"
#include "Core_Window.h"
#include "Sample_Manifest.h"
#include <Windows.h>
#include <functional>
#include <memory>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    try
    {
        std::shared_ptr<Direct3D11Device> pDevice11 = CreateDirect3D11Device();
        std::shared_ptr<Direct3D12Device> pDevice12 = CreateDirect3D12Device();
        std::shared_ptr<OpenGLDevice> pDeviceGL = CreateOpenGLDevice();

        std::shared_ptr<Window> pWindow1(CreateNewWindow());
        pWindow1->SetSample(CreateSample_D3D11Basic(CreateDXGISwapChain(pDevice11, pWindow1->GetWindowHandle()), pDevice11));

        std::shared_ptr<Window> pWindow2(CreateNewWindow());
        pWindow2->SetSample(CreateSample_D3D11ComputeCanvas(CreateDXGISwapChain(pDevice11, pWindow2->GetWindowHandle()), pDevice11));

        std::shared_ptr<Window> pWindow3(CreateNewWindow());
        pWindow3->SetSample(CreateSample_D3D11DrawingContext(CreateDXGISwapChain(pDevice11, pWindow3->GetWindowHandle()), pDevice11));

        std::shared_ptr<Window> pWindow4(CreateNewWindow());
        pWindow4->SetSample(CreateSample_D3D11Imgui(CreateDXGISwapChain(pDevice11, pWindow4->GetWindowHandle()), pDevice11));

        std::shared_ptr<Window> pWindow5(CreateNewWindow());
        pWindow5->SetSample(CreateSample_D3D12Basic(CreateDXGISwapChain(pDevice12, pWindow5->GetWindowHandle()), pDevice12));

        std::shared_ptr<Window> pWindow6(CreateNewWindow());
        pWindow6->SetSample(CreateSample_D3D12Imgui(CreateDXGISwapChain(pDevice12, pWindow6->GetWindowHandle()), pDevice12));

        std::shared_ptr<Window> pWindow7(CreateNewWindow());
        pWindow7->SetSample(CreateSample_OpenGLBasic(pDeviceGL, pWindow7));

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