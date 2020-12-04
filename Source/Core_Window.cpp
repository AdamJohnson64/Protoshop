#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3D11Util.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include "Core_DXGI.h"
#include "Core_ITransformSource.h"
#include "Core_Math.h"
#include "Core_Object.h"
#include "Core_OpenGL.h"
#include "Core_Util.h"
#include "Core_VK.h"
#include <Windows.h>
#include <d3d11.h>
#include <functional>
#include <memory>

static bool g_MouseDown = false;
static int g_MouseX = 0;
static int g_MouseY = 0;

static Vector3 g_CameraPos = {0, 1, -5};
static Quaternion g_CameraRot = {0, 0, 0, 1};

static Matrix44 TransformWorldToView = {
    // clang-format off
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, -1, 5, 1
    // clang-format on
};

static Matrix44 TransformViewToClip = CreateProjection<float>(
    0.01f, 100.0f, 45 * (Pi<float> / 180), 45 * (Pi<float> / 180));

class TransformSource : public ITransformSource {
public:
  virtual Matrix44 GetTransformWorldToClip() {
    return TransformWorldToView * TransformViewToClip;
  }
  virtual Matrix44 GetTransformWorldToView() { return TransformWorldToView; }
} g_TransformSource;

ITransformSource *GetTransformSource() { return &g_TransformSource; }

void SetCameraWorldToView(const Matrix44 &transformWorldToView) {
  TransformWorldToView = transformWorldToView;
}

class WindowBase : public Object {
protected:
  HWND m_hWindow;

public:
  WindowBase() {
    ////////////////////////////////////////////////////////////////////////////////
    // Create a window class.
    static bool windowClassInitialized = false;
    if (!windowClassInitialized) {
      WNDCLASS wndclass = {};
      wndclass.lpfnWndProc = WindowProc;
      wndclass.cbWndExtra = sizeof(WindowBase *);
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
  ~WindowBase() {
    KillTimer(m_hWindow, 0);
    DestroyWindow(m_hWindow);
  }
  virtual void OnPaint() = 0;

private:
  static LRESULT WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                   LPARAM lParam) {
    WindowBase *window =
        reinterpret_cast<WindowBase *>(GetWindowLongPtr(hWnd, 0));
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
      if (g_MouseDown) {
        bool modifierShift = (wParam & MK_SHIFT) == MK_SHIFT;
        bool modifierCtrl = (wParam & MK_CONTROL) == MK_CONTROL;
        int mouseXNow = LOWORD(lParam);
        int mouseYNow = HIWORD(lParam);
        int mouseDeltaX = mouseXNow - g_MouseX;
        int mouseDeltaY = mouseYNow - g_MouseY;
        Matrix44 transformWorldToView = CreateMatrixRotation(g_CameraRot);
        // Camera Truck
        if (!modifierShift && !modifierCtrl) {
          g_CameraPos.X += transformWorldToView.M11 * -mouseDeltaX * 0.01f;
          g_CameraPos.Y += transformWorldToView.M12 * -mouseDeltaX * 0.01f;
          g_CameraPos.Z += transformWorldToView.M13 * -mouseDeltaX * 0.01f;
          g_CameraPos.X += transformWorldToView.M21 * mouseDeltaY * 0.01f;
          g_CameraPos.Y += transformWorldToView.M22 * mouseDeltaY * 0.01f;
          g_CameraPos.Z += transformWorldToView.M23 * mouseDeltaY * 0.01f;
        }
        // Camera Pan/Tilt
        if (!modifierShift && modifierCtrl) {
          if (mouseDeltaX != 0)
            g_CameraRot =
                Multiply<float>(CreateQuaternionRotation<float>(
                                    {0, 1, 0}, mouseDeltaX / (2 * Pi<float>)),
                                g_CameraRot);
          if (mouseDeltaY != 0)
            g_CameraRot = Multiply<float>(
                g_CameraRot, CreateQuaternionRotation<float>(
                                 {1, 0, 0}, mouseDeltaY / (2 * Pi<float>)));
        }
        // Camera Dolly
        if (modifierShift && modifierCtrl) {
          g_CameraPos.X += transformWorldToView.M31 * -mouseDeltaY * 0.01f;
          g_CameraPos.Y += transformWorldToView.M32 * -mouseDeltaY * 0.01f;
          g_CameraPos.Z += transformWorldToView.M33 * -mouseDeltaY * 0.01f;
        }
        transformWorldToView.M41 = g_CameraPos.X;
        transformWorldToView.M42 = g_CameraPos.Y;
        transformWorldToView.M43 = g_CameraPos.Z;
        SetCameraWorldToView(Invert(transformWorldToView));
        InvalidateRect(hWnd, nullptr, FALSE);
      }
      g_MouseX = LOWORD(lParam);
      g_MouseY = HIWORD(lParam);
      return 0;
    }
    if (uMsg == WM_LBUTTONDOWN) {
      SetCapture(hWnd);
      g_MouseDown = true;
      return 0;
    }
    if (uMsg == WM_LBUTTONUP) {
      ReleaseCapture();
      g_MouseDown = false;
      return 0;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }
};

class WindowWithSwapChainD3D11 : public WindowBase {
protected:
  std::shared_ptr<Direct3D11Device> m_Direct3D11Device;
  std::shared_ptr<DXGISwapChain> m_DXGISwapChain;

public:
  WindowWithSwapChainD3D11(std::shared_ptr<Direct3D11Device> device) {
    m_Direct3D11Device = device;
    m_DXGISwapChain = CreateDXGISwapChain(device, m_hWindow);
  }
};

class WindowWithSwapChainD3D12 : public WindowBase {
protected:
  std::shared_ptr<Direct3D12Device> m_Direct3D12Device;
  std::shared_ptr<DXGISwapChain> m_DXGISwapChain;

public:
  WindowWithSwapChainD3D12(std::shared_ptr<Direct3D12Device> deviceD3D12) {
    m_Direct3D12Device = deviceD3D12;
    m_DXGISwapChain = CreateDXGISwapChain(deviceD3D12, m_hWindow);
  }
};

class WindowRenderToD3D11Texture2D : public WindowWithSwapChainD3D11 {
  std::function<void(ID3D11Texture2D *)> m_fnRender;

public:
  WindowRenderToD3D11Texture2D(std::shared_ptr<Direct3D11Device> device,
                               std::function<void(ID3D11Texture2D *)> fnRender)
      : WindowWithSwapChainD3D11(device) {
    m_fnRender = fnRender;
  }
  void OnPaint() override {
    CComPtr<ID3D11Texture2D> textureBackbuffer;
    TRYD3D(m_DXGISwapChain->GetIDXGISwapChain()->GetBuffer(
        0, __uuidof(ID3D11Texture2D), (void **)&textureBackbuffer.p));
    m_fnRender(textureBackbuffer);
    m_DXGISwapChain->GetIDXGISwapChain()->Present(0, 0);
  }
};

class WindowRenderToD3D12RTV : public WindowWithSwapChainD3D12 {
  std::function<void(ID3D12Resource *)> m_fnRender;

public:
  WindowRenderToD3D12RTV(std::shared_ptr<Direct3D12Device> device,
                         std::function<void(ID3D12Resource *)> fnRender)
      : WindowWithSwapChainD3D12(device) {
    m_fnRender = fnRender;
  }
  void OnPaint() override {
    CComPtr<ID3D12Resource> resourceBackbuffer;
    TRYD3D(m_DXGISwapChain->GetIDXGISwapChain()->GetBuffer(
        m_DXGISwapChain->GetIDXGISwapChain()->GetCurrentBackBufferIndex(),
        __uuidof(ID3D12Resource), (void **)&resourceBackbuffer));
    m_fnRender(resourceBackbuffer);
    m_DXGISwapChain->GetIDXGISwapChain()->Present(0, 0);
  }
};

class WindowRenderToOpenGL : public WindowBase {
  std::shared_ptr<OpenGLDevice> m_OpenGLDevice;
  std::function<void()> m_fnRender;
  HDC m_hDC;

public:
  WindowRenderToOpenGL(std::shared_ptr<OpenGLDevice> deviceOpenGL,
                       std::function<void()> fnRender)
      : m_OpenGLDevice(deviceOpenGL), m_fnRender(fnRender) {
    m_hDC = GetDC(m_hWindow);
    // Describe a 32-bit ARGB pixel format.
    PIXELFORMATDESCRIPTOR pixelFormatDescriptor = {};
    pixelFormatDescriptor.nSize = sizeof(pixelFormatDescriptor);
    pixelFormatDescriptor.nVersion = 1;
    pixelFormatDescriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    pixelFormatDescriptor.cColorBits = 32;
    pixelFormatDescriptor.iPixelType = PFD_TYPE_RGBA;
    pixelFormatDescriptor.iLayerType = PFD_MAIN_PLANE;
    // Choose a pixel format that is a close approximation of this suggested
    // format.
    int nPixelFormat = ChoosePixelFormat(m_hDC, &pixelFormatDescriptor);
    if (0 == nPixelFormat)
      throw FALSE;
    // Use this format (this may only be set ONCE).
    if (TRUE != SetPixelFormat(m_hDC, nPixelFormat, &pixelFormatDescriptor))
      throw FALSE;
  }
  ~WindowRenderToOpenGL() { ReleaseDC(m_hWindow, m_hDC); }
  void OnPaint() override {
    if (TRUE != wglMakeCurrent(m_hDC, m_OpenGLDevice->GetHLGRC()))
      throw FALSE;
    // Establish a viewport as big as the window area.
    RECT rectClient = {};
    GetClientRect(m_hWindow, &rectClient);
    glViewport(0, 0, rectClient.right - rectClient.left,
               rectClient.bottom - rectClient.top);
    m_fnRender();
    if (TRUE != wglMakeCurrent(m_hDC, nullptr))
      throw FALSE;
  }
};

class AutoCloseHandle {
public:
  void operator()(HANDLE h) {
    if (h != INVALID_HANDLE_VALUE) {
      CloseHandle(h);
    }
  }
};

class WindowRenderToVKImage : public WindowWithSwapChainD3D12 {
private:
  std::shared_ptr<VKDevice> m_VKDevice;
  std::function<void(VKDevice *, vk::Image)> m_fnRender;

public:
  WindowRenderToVKImage(std::shared_ptr<VKDevice> deviceVK,
                        std::shared_ptr<Direct3D12Device> deviceD3D12,
                        std::function<void(VKDevice *, vk::Image)> fnRender)
      : WindowWithSwapChainD3D12(deviceD3D12), m_VKDevice(deviceVK),
        m_fnRender(fnRender) {}
  void OnPaint() {
    // We can't directly use the image from DXGI, probably because it's a
    // D3D9Ex image. Create a shared D3D12 image to use as our render target
    // and copy it back later.
    CComPtr<ID3D12Resource1> resourceBackbuffer;
    {
      D3D12_HEAP_PROPERTIES descHeapProperties = {};
      descHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
      D3D12_RESOURCE_DESC descResource = {};
      descResource.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
      descResource.Width = RENDERTARGET_WIDTH;
      descResource.Height = RENDERTARGET_HEIGHT;
      descResource.DepthOrArraySize = 1;
      descResource.MipLevels = 1;
      descResource.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
      descResource.SampleDesc.Count = 1;
      descResource.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
      D3D12_CLEAR_VALUE descClear = {};
      descClear.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
      descClear.DepthStencil.Depth = 1.0f;
      TRYD3D(m_Direct3D12Device->m_pDevice->CreateCommittedResource1(
          &descHeapProperties,
          D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES |
              D3D12_HEAP_FLAG_SHARED,
          &descResource, D3D12_RESOURCE_STATE_COMMON, &descClear, nullptr,
          __uuidof(ID3D12Resource1), (void **)&resourceBackbuffer.p));
      resourceBackbuffer->SetName(L"D3D12Resource (Backbuffer)");
    }
    // Open a shared NT handle to the image (true HANDLE; must be closed
    // later).
    std::unique_ptr<HANDLE, AutoCloseHandle> handleBackbuffer;
    TRYD3D(m_Direct3D12Device->m_pDevice->CreateSharedHandle(
        resourceBackbuffer, nullptr, GENERIC_ALL, nullptr,
        (HANDLE *)&handleBackbuffer));

    // Create a render pass.
    vk::UniqueRenderPass vkRenderPass;
    {
      std::vector<vk::AttachmentDescription> ad = {vk::AttachmentDescription(
          {}, vk::Format::eR8G8B8A8Unorm, vk::SampleCountFlagBits::e1,
          vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
          vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
          vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR)};
      std::vector<vk::AttachmentReference> ar = {
          vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal)};
      std::vector<vk::SubpassDescription> spd = {
          vk::SubpassDescription({}, vk::PipelineBindPoint::eGraphics, {}, ar)};
      vk::RenderPassCreateInfo rpci((vk::RenderPassCreateFlagBits)0, ad, spd);
      vkRenderPass = m_VKDevice->m_vkDevice->createRenderPassUnique(rpci);
    }
    // Create a Vulkan image to render into.
    vk::UniqueImage vkImage;
    {
      vk::StructureChain<vk::ImageCreateInfo, vk::ExternalMemoryImageCreateInfo>
          structureChain = {
              vk::ImageCreateInfo(
                  {}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm,
                  {RENDERTARGET_WIDTH, RENDERTARGET_HEIGHT, 1}, 1, 1,
                  vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
                  vk::ImageUsageFlagBits::eTransferDst |
                      vk::ImageUsageFlagBits::eColorAttachment,
                  vk::SharingMode::eExclusive, {}, vk::ImageLayout::eUndefined),
              vk::ExternalMemoryImageCreateInfo(
                  vk::ExternalMemoryHandleTypeFlagBits::eD3D12Resource)};
      vkImage = m_VKDevice->m_vkDevice->createImageUnique(
          structureChain.get<vk::ImageCreateInfo>());
    }

    // Fake-Allocate the memory for our Vulkan image as an alias of the D3D12
    // image created above.
    vk::UniqueDeviceMemory vkDeviceMemory;
    {
      vk::MemoryRequirements mr =
          m_VKDevice->m_vkDevice->getImageMemoryRequirements(vkImage.get());
      vk::StructureChain<vk::MemoryAllocateInfo,
                         vk::ImportMemoryWin32HandleInfoKHR>
          sc = {vk::MemoryAllocateInfo(mr.size, m_VKDevice->m_memoryTypeDevice),
                vk::ImportMemoryWin32HandleInfoKHR(
                    vk::ExternalMemoryHandleTypeFlagBits::eD3D12Resource,
                    handleBackbuffer.get())};
      vkDeviceMemory = m_VKDevice->m_vkDevice->allocateMemoryUnique(
          sc.get<vk::MemoryAllocateInfo>());
    }

    // Connect the image with its memory.
    m_VKDevice->m_vkDevice->bindImageMemory(vkImage.get(), vkDeviceMemory.get(),
                                            0);

    // Create an image view of the above Vulkan image.
    vk::UniqueImageView vkImageView;
    {
      vk::ImageViewCreateInfo ivci(
          {}, vkImage.get(), vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Unorm,
          vk::ComponentMapping(
              vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
              vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA),
          vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0,
                                    1));
      vkImageView = m_VKDevice->m_vkDevice->createImageViewUnique(ivci);
    }
    /*
    // Create a framebuffer from the above Vulkan image.
    vk::UniqueFramebuffer vkFramebuffer;
    {
      std::vector<vk::ImageView> attachments = {vkImageView.get()};
      vk::FramebufferCreateInfo cfb({}, deviceVK->m_vkRenderPass.get(),
                                    attachments, RENDERTARGET_WIDTH,
                                    RENDERTARGET_HEIGHT, 1);
      vkFramebuffer = deviceVK->m_vkDevice->createFramebufferUnique(cfb);
    }
    */
    // Call the renderer.
    m_fnRender(m_VKDevice.get(), vkImage.get());
    // Present the back buffer to the Windows compositor.
    {
      // Get the next buffer in the DXGI swap chain.
      CComPtr<ID3D12Resource> pD3D12SwapChainBuffer;
      TRYD3D(m_DXGISwapChain->GetIDXGISwapChain()->GetBuffer(
          m_DXGISwapChain->GetIDXGISwapChain()->GetCurrentBackBufferIndex(),
          __uuidof(ID3D12Resource), (void **)&pD3D12SwapChainBuffer));

      // Initialize a Render Target View (RTV) from this DXGI buffer.
      m_Direct3D12Device->m_pDevice->CreateRenderTargetView(
          pD3D12SwapChainBuffer,
          &Make_D3D12_RENDER_TARGET_VIEW_DESC_SwapChainDefault(),
          m_Direct3D12Device->m_pDescriptorHeapRTV
              ->GetCPUDescriptorHandleForHeapStart());

      // Perform a copy of our intermediate D3D12 image (written by Vulkan) to
      // the DXGI buffer.
      D3D12_Run_Synchronously(
          m_Direct3D12Device.get(), [&](ID3D12GraphicsCommandList5 *cmd) {
            cmd->ResourceBarrier(1, &Make_D3D12_RESOURCE_BARRIER(
                                        pD3D12SwapChainBuffer,
                                        D3D12_RESOURCE_STATE_COMMON,
                                        D3D12_RESOURCE_STATE_COPY_DEST));
            cmd->CopyResource(pD3D12SwapChainBuffer, resourceBackbuffer);
            cmd->ResourceBarrier(
                1, &Make_D3D12_RESOURCE_BARRIER(pD3D12SwapChainBuffer,
                                                D3D12_RESOURCE_STATE_COPY_DEST,
                                                D3D12_RESOURCE_STATE_PRESENT));
          });
    }
    TRYD3D(m_DXGISwapChain->GetIDXGISwapChain()->Present(0, 0));
  }
};

std::shared_ptr<Object>
CreateNewWindow(std::shared_ptr<Direct3D11Device> deviceD3D11,
                std::function<void(ID3D11Texture2D *)> fnRender) {
  return std::shared_ptr<Object>(
      new WindowRenderToD3D11Texture2D(deviceD3D11, fnRender));
}

std::shared_ptr<Object>
CreateNewWindow(std::shared_ptr<Direct3D12Device> deviceD3D12,
                std::function<void(ID3D12Resource *rt)> fnRender) {
  return std::shared_ptr<Object>(
      new WindowRenderToD3D12RTV(deviceD3D12, fnRender));
}

std::shared_ptr<Object>
CreateNewWindow(std::shared_ptr<OpenGLDevice> deviceOpenGL,
                std::function<void()> fnRender) {
  return std::shared_ptr<Object>(
      new WindowRenderToOpenGL(deviceOpenGL, fnRender));
}

std::shared_ptr<Object>
CreateNewWindow(std::shared_ptr<VKDevice> deviceVK,
                std::shared_ptr<Direct3D12Device> deviceD3D12,
                std::function<void(VKDevice *, vk::Image)> fnRender) {
  return std::shared_ptr<Object>(
      new WindowRenderToVKImage(deviceVK, deviceD3D12, fnRender));
}