#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_Object.h"
#include "Core_Window.h"
#include "Sample.h"
#include <Windows.h>
#include <functional>
#include <memory>

class WindowImpl : public Window
{
private:
    HWND m_hWindow;
    std::shared_ptr<Sample> m_pSample;
public:
    WindowImpl()
    {
        ////////////////////////////////////////////////////////////////////////////////
        // Create a window class.
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
        ////////////////////////////////////////////////////////////////////////////////
        // Create a window of this class.
        {
            RECT rect = { 64, 64, 64 + RENDERTARGET_WIDTH, 64 + RENDERTARGET_HEIGHT };
            AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
            m_hWindow = CreateWindow(L"Protoshop", L"Protoshop", WS_OVERLAPPEDWINDOW, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, nullptr, this);
        }
        ShowWindow(m_hWindow, SW_SHOW);
        ////////////////////////////////////////////////////////////////////////////////
        // Start a timer to drive frames.
        SetTimer(m_hWindow, 0, 100, [](HWND hWnd, UINT, UINT_PTR, DWORD)
            {
                InvalidateRect(hWnd, nullptr, FALSE);
            });
    }
    ~WindowImpl()
    {
        KillTimer(m_hWindow, 0);
        DestroyWindow(m_hWindow);
    }
    HWND GetWindowHandle() override
    {
        return m_hWindow;
    }
    void SetSample(std::shared_ptr<Sample> pSample) override
    {
        m_pSample = pSample;
    }
    virtual void OnPaint()
    {
        if (m_pSample != nullptr)
        {
            m_pSample->Render();
        }
    }
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
        if (uMsg == WM_PAINT)
        {
            window->OnPaint();
            ValidateRect(hWnd, nullptr);
            return 0;
        }
        if (uMsg == WM_CLOSE)
        {
            PostQuitMessage(0);
            return 0;
        }
        if (uMsg == WM_MOUSEMOVE)
        {
            MouseListener* mouse = dynamic_cast<MouseListener*>(window->m_pSample.get());
            if (mouse != nullptr) mouse->MouseMove(LOWORD(lParam), HIWORD(lParam));
            return 0;
        }
        if (uMsg == WM_LBUTTONDOWN)
        {
            MouseListener* mouse = dynamic_cast<MouseListener*>(window->m_pSample.get());
            if (mouse != nullptr) mouse->MouseDown(LOWORD(lParam), HIWORD(lParam));
            return 0;
        }
        if (uMsg == WM_LBUTTONUP)
        {
            MouseListener* mouse = dynamic_cast<MouseListener*>(window->m_pSample.get());
            if (mouse != nullptr) mouse->MouseUp(LOWORD(lParam), HIWORD(lParam));
            return 0;
        }
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
};

std::shared_ptr<Window> CreateNewWindow()
{
    return std::shared_ptr<Window>(new WindowImpl());
}