#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3D11Util.h"
#include "Core_D3D12.h"
#include "Core_ISample.h"
#include "Core_IWindow.h"
#include "Core_Math.h"
#include "Core_Object.h"
#include "Core_Util.h"
#include "Scene_Camera.h"
#include <Windows.h>
#include <d3d11.h>
#include <functional>
#include <memory>

static bool mouseDown = false;
static int mouseX = 0;
static int mouseY = 0;

static Vector3 cameraPos = {0, 1, -5};
static Quaternion cameraRot = {0, 0, 0, 1};

class WindowSimple : public Object, public IWindow {
protected:
  HWND m_hWindow;
  std::shared_ptr<ISample> m_pSample;

public:
  WindowSimple() {
    ////////////////////////////////////////////////////////////////////////////////
    // Create a window class.
    static bool windowClassInitialized = false;
    if (!windowClassInitialized) {
      WNDCLASS wndclass = {};
      wndclass.lpfnWndProc = WindowProc;
      wndclass.cbWndExtra = sizeof(WindowSimple *);
      wndclass.lpszClassName = L"Protoshop";
      if (0 == RegisterClass(&wndclass))
        throw FALSE;
      windowClassInitialized = true;
    }
    ////////////////////////////////////////////////////////////////////////////////
    // Create a window of this class.
    {
      RECT rect = {64, 64, 64 + RENDERTARGET_WIDTH, 64 + RENDERTARGET_HEIGHT};
      AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
      m_hWindow =
          CreateWindow(L"Protoshop", L"Protoshop", WS_OVERLAPPEDWINDOW,
                       rect.left, rect.top, rect.right - rect.left,
                       rect.bottom - rect.top, nullptr, nullptr, nullptr, this);
    }
    ShowWindow(m_hWindow, SW_SHOW);
    ////////////////////////////////////////////////////////////////////////////////
    // Start a timer to drive frames.
    SetTimer(m_hWindow, 0, 1000 / 30, [](HWND hWnd, UINT, UINT_PTR, DWORD) {
      InvalidateRect(hWnd, nullptr, FALSE);
    });
  }
  ~WindowSimple() {
    KillTimer(m_hWindow, 0);
    DestroyWindow(m_hWindow);
  }
  HWND GetWindowHandle() override { return m_hWindow; }
  void SetSample(std::shared_ptr<ISample> pSample) override {
    m_pSample = pSample;
  }
  virtual void OnPaint() {
    if (m_pSample != nullptr) {
      m_pSample->Render();
    }
  }

private:
  static LRESULT WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                   LPARAM lParam) {
    WindowSimple *window =
        reinterpret_cast<WindowSimple *>(GetWindowLongPtr(hWnd, 0));
    if (uMsg == WM_CREATE) {
      const CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT *>(lParam);
      SetWindowLongPtr(hWnd, 0, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
      return 0;
    }
    if (uMsg == WM_PAINT) {
      window->OnPaint();
      ValidateRect(hWnd, nullptr);
      return 0;
    }
    if (uMsg == WM_CLOSE) {
      PostQuitMessage(0);
      return 0;
    }
    if (uMsg == WM_MOUSEMOVE) {
      if (mouseDown) {
        bool modifierShift = (wParam & MK_SHIFT) == MK_SHIFT;
        bool modifierCtrl = (wParam & MK_CONTROL) == MK_CONTROL;
        int mouseXNow = LOWORD(lParam);
        int mouseYNow = HIWORD(lParam);
        int mouseDeltaX = mouseXNow - mouseX;
        int mouseDeltaY = mouseYNow - mouseY;
        Matrix44 transformWorldToView = CreateMatrixRotation(cameraRot);
        // Camera Truck
        if (!modifierShift && !modifierCtrl) {
          cameraPos.X += transformWorldToView.M11 * -mouseDeltaX * 0.01f;
          cameraPos.Y += transformWorldToView.M12 * -mouseDeltaX * 0.01f;
          cameraPos.Z += transformWorldToView.M13 * -mouseDeltaX * 0.01f;
          cameraPos.X += transformWorldToView.M21 * mouseDeltaY * 0.01f;
          cameraPos.Y += transformWorldToView.M22 * mouseDeltaY * 0.01f;
          cameraPos.Z += transformWorldToView.M23 * mouseDeltaY * 0.01f;
        }
        // Camera Pan/Tilt
        if (!modifierShift && modifierCtrl) {
          if (mouseDeltaX != 0)
            cameraRot =
                Multiply<float>(CreateQuaternionRotation<float>(
                                    {0, 1, 0}, mouseDeltaX / (2 * Pi<float>)),
                                cameraRot);
          if (mouseDeltaY != 0)
            cameraRot = Multiply<float>(
                cameraRot, CreateQuaternionRotation<float>(
                               {1, 0, 0}, mouseDeltaY / (2 * Pi<float>)));
        }
        // Camera Dolly
        if (modifierShift && modifierCtrl) {
          cameraPos.X += transformWorldToView.M31 * -mouseDeltaY * 0.01f;
          cameraPos.Y += transformWorldToView.M32 * -mouseDeltaY * 0.01f;
          cameraPos.Z += transformWorldToView.M33 * -mouseDeltaY * 0.01f;
        }
        transformWorldToView.M41 = cameraPos.X;
        transformWorldToView.M42 = cameraPos.Y;
        transformWorldToView.M43 = cameraPos.Z;
        SetCameraWorldToView(Invert(transformWorldToView));
        InvalidateRect(hWnd, nullptr, FALSE);
      }
      mouseX = LOWORD(lParam);
      mouseY = HIWORD(lParam);
      return 0;
    }
    if (uMsg == WM_LBUTTONDOWN) {
      SetCapture(hWnd);
      mouseDown = true;
      return 0;
    }
    if (uMsg == WM_LBUTTONUP) {
      ReleaseCapture();
      mouseDown = false;
      return 0;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }
};

class WindowAndSwapChain : public WindowSimple {
protected:
  std::shared_ptr<Direct3D11Device> m_Device;
  std::shared_ptr<DXGISwapChain> m_SwapChain;

public:
  WindowAndSwapChain(std::shared_ptr<Direct3D11Device> device) {
    m_Device = device;
    m_SwapChain = CreateDXGISwapChain(device, m_hWindow);
  }
};

class WindowAndSwapChain12 : public WindowSimple {
protected:
  std::shared_ptr<Direct3D12Device> m_Device;
  std::shared_ptr<DXGISwapChain> m_SwapChain;

public:
  WindowAndSwapChain12(std::shared_ptr<Direct3D12Device> device) {
    m_Device = device;
    m_SwapChain = CreateDXGISwapChain(device, m_hWindow);
  }
};

class WindowWithSwapChain : public WindowAndSwapChain {
  std::function<void(IDXGISwapChain *)> m_fnRender;

public:
  WindowWithSwapChain(std::shared_ptr<Direct3D11Device> device,
                      std::function<void(IDXGISwapChain *)> fnRender)
      : WindowAndSwapChain(device) {
    m_fnRender = fnRender;
  }
  void OnPaint() override {
    m_fnRender(m_SwapChain->GetIDXGISwapChain());
    m_SwapChain->GetIDXGISwapChain()->Present(0, 0);
  }
};

class WindowWithRTV : public WindowAndSwapChain {
  std::function<void(ID3D11RenderTargetView *)> m_fnRender;

public:
  WindowWithRTV(std::shared_ptr<Direct3D11Device> device,
                std::function<void(ID3D11RenderTargetView *)> fnRender)
      : WindowAndSwapChain(device) {
    m_fnRender = fnRender;
  }
  void OnPaint() override {
    // Get the backbuffer and create a render target from it.
    CComPtr<ID3D11RenderTargetView> rtvBackbuffer =
        D3D11_Create_RTV_From_SwapChain(m_Device->GetID3D11Device(),
                                        m_SwapChain->GetIDXGISwapChain());
    m_fnRender(rtvBackbuffer);
    m_SwapChain->GetIDXGISwapChain()->Present(0, 0);
  }
};

class WindowWithUAV : public WindowAndSwapChain {
  std::function<void(ID3D11UnorderedAccessView *)> m_fnRender;

public:
  WindowWithUAV(std::shared_ptr<Direct3D11Device> device,
                std::function<void(ID3D11UnorderedAccessView *)> fnRender)
      : WindowAndSwapChain(device) {
    m_fnRender = fnRender;
  }
  void OnPaint() override {
    // Get the backbuffer and create a render target from it.
    CComPtr<ID3D11UnorderedAccessView> uavBackbuffer =
        D3D11_Create_UAV_From_SwapChain(m_Device->GetID3D11Device(),
                                        m_SwapChain->GetIDXGISwapChain());
    m_fnRender(uavBackbuffer);
    m_SwapChain->GetIDXGISwapChain()->Present(0, 0);
  }
};

class WindowWithRTV12 : public WindowAndSwapChain12 {
  std::function<void(ID3D12Resource *)> m_fnRender;

public:
  WindowWithRTV12(std::shared_ptr<Direct3D12Device> device,
                  std::function<void(ID3D12Resource *)> fnRender)
      : WindowAndSwapChain12(device) {
    m_fnRender = fnRender;
  }
  void OnPaint() override {
    CComPtr<ID3D12Resource> resourceBackbuffer;
    TRYD3D(m_SwapChain->GetIDXGISwapChain()->GetBuffer(
        m_SwapChain->GetIDXGISwapChain()->GetCurrentBackBufferIndex(),
        __uuidof(ID3D12Resource), (void **)&resourceBackbuffer));
    m_fnRender(resourceBackbuffer);
    m_SwapChain->GetIDXGISwapChain()->Present(0, 0);
  }
};

std::shared_ptr<IWindow> CreateNewWindow() {
  return std::shared_ptr<IWindow>(new WindowSimple());
}

std::shared_ptr<Object>
CreateNewWindow(std::shared_ptr<Direct3D11Device> device,
                std::function<void(IDXGISwapChain *)> fnRender) {
  return std::shared_ptr<Object>(new WindowWithSwapChain(device, fnRender));
}

std::shared_ptr<Object>
CreateNewWindow(std::shared_ptr<Direct3D11Device> device,
                std::function<void(ID3D11RenderTargetView *)> fnRender) {
  return std::shared_ptr<Object>(new WindowWithRTV(device, fnRender));
}

std::shared_ptr<Object>
CreateNewWindow(std::shared_ptr<Direct3D11Device> device,
                std::function<void(ID3D11UnorderedAccessView *)> fnRender) {
  return std::shared_ptr<Object>(new WindowWithUAV(device, fnRender));
}

std::shared_ptr<Object>
CreateNewWindow(std::shared_ptr<Direct3D12Device> device,
                std::function<void(ID3D12Resource *rt)> fnRender) {
  return std::shared_ptr<Object>(new WindowWithRTV12(device, fnRender));
}