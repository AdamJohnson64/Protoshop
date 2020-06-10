#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Framework_Sample.h"
#include <Windows.h>
#include <functional>
#include <memory>

std::function<void()> fnWindowProcPaint = nullptr;

LRESULT WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_CLOSE)
    {
        PostQuitMessage(0);
        return 0;
    }
    if (uMsg == WM_PAINT)
    {
        fnWindowProcPaint();
        ValidateRect(hWnd, nullptr);
        return 0;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    try
    {

    ////////////////////////////////////////////////////////////////////////////////
    // Create a window class.
    {
        WNDCLASS wndclass = {};
        wndclass.lpszClassName = L"HelloD3D11";
        wndclass.lpfnWndProc = WindowProc;
        if (0 == RegisterClass(&wndclass)) throw FALSE;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Create a window for display.
    HWND hWindow = CreateWindow(L"HelloD3D11", L"HelloD3D11", WS_OVERLAPPEDWINDOW, 16, 16, RENDERTARGET_WIDTH, RENDERTARGET_HEIGHT, nullptr, nullptr, nullptr, nullptr);
    ShowWindow(hWindow, nShowCmd);

    ////////////////////////////////////////////////////////////////////////////////
    // Initialize our graphics API.
    InitializeDirect3D11(hWindow);
    //std::unique_ptr<Sample> sample(CreateSample_ComputeCanvas());
    std::unique_ptr<Sample> sample(CreateSample_DrawingContext());

    ////////////////////////////////////////////////////////////////////////////////
    // Define the window paint function to use in WNDPROC.
    fnWindowProcPaint = [&]() {
        sample->Render();
    };

    ////////////////////////////////////////////////////////////////////////////////
    // Run the window message loop.
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