#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Sample.h"
#include <Windows.h>
#include <functional>
#include <memory>

class WindowImpl
{
public:
    WindowImpl()
    {
        static bool windowClassInitialized = false;
        if (!windowClassInitialized)
        {
            WNDCLASS wndclass = {};
            wndclass.lpfnWndProc = WindowProc;
            wndclass.cbWndExtra = sizeof(WindowImpl*);
            wndclass.lpszClassName = L"Protoshop";
            if (0 == RegisterClass(&wndclass)) throw FALSE;
            windowClassInitialized = true;
        }
        m_hWindow = CreateWindow(L"Protoshop", L"Protoshop", WS_OVERLAPPEDWINDOW, 16, 16, RENDERTARGET_WIDTH, RENDERTARGET_HEIGHT, nullptr, nullptr, nullptr, this);
        ShowWindow(m_hWindow, SW_SHOW);
    }
    virtual void OnPaint()
    {
        if (m_pSample != nullptr)
        {
            m_pSample->Render();
        }
    }
    void SetSample(std::shared_ptr<Sample> pSample)
    {
        m_pSample = pSample;
    }
    HWND m_hWindow;
private:
    static LRESULT WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        WindowImpl* window = reinterpret_cast<WindowImpl*>(GetWindowLongPtr(hWnd, 0));
        if (uMsg == WM_CREATE)
        {
            const CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
            SetWindowLongPtr(hWnd, 0, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            return 0;
        }
        if (uMsg == WM_CLOSE)
        {
            PostQuitMessage(0);
            return 0;
        }
        if (uMsg == WM_PAINT)
        {
            window->OnPaint();
            ValidateRect(hWnd, nullptr);
            return 0;
        }
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    std::shared_ptr<Sample> m_pSample;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    try
    {
        std::shared_ptr<Direct3D11Device> pDevice = CreateDirect3D11Device();

        std::shared_ptr<WindowImpl> pWindow1(new WindowImpl());
        std::shared_ptr<DXGISwapChain> pSwapChain1 = CreateDXGISwapChain(pDevice, pWindow1->m_hWindow);
        pWindow1->SetSample(CreateSample_ComputeCanvas(pSwapChain1, pDevice));

        std::shared_ptr<WindowImpl> pWindow2(new WindowImpl());
        std::shared_ptr<DXGISwapChain> pSwapChain2 = CreateDXGISwapChain(pDevice, pWindow2->m_hWindow);
        pWindow2->SetSample(CreateSample_DrawingContext(pSwapChain2, pDevice));

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