#include "Core_Window.h"
#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3D11Util.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
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

void SetCameraWorldToView(const Matrix44 &transformWorldToView) {
  TransformWorldToView = transformWorldToView;
}

class WindowPaintFunction : public Object {
public:
  HWND m_hWindow;
  std::function<void()> m_OnPaint;
  std::function<void(char)> m_OnKeyDown;

public:
  WindowPaintFunction() {
    ////////////////////////////////////////////////////////////////////////////////
    // Create a window class.
    static bool windowClassInitialized = false;
    if (!windowClassInitialized) {
      WNDCLASS wndclass = {};
      wndclass.lpfnWndProc = WindowProc;
      wndclass.cbWndExtra = sizeof(WindowPaintFunction *);
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

  ~WindowPaintFunction() {
    KillTimer(m_hWindow, 0);
    DestroyWindow(m_hWindow);
  }

private:
  static LRESULT WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                   LPARAM lParam) {
    WindowPaintFunction *window =
        reinterpret_cast<WindowPaintFunction *>(GetWindowLongPtr(hWnd, 0));
    if (uMsg == WM_CREATE) {
      const CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT *>(lParam);
      SetWindowLongPtr(hWnd, 0, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
      return 0;
    }
    if (uMsg == WM_PAINT) {
      window->m_OnPaint();
      ValidateRect(hWnd, nullptr);
      return 0;
    }
    if (uMsg == WM_CLOSE) {
      PostQuitMessage(0);
      return 0;
    }
    if (uMsg == WM_KEYDOWN) {
      if (window->m_OnKeyDown) {
        window->m_OnKeyDown(static_cast<char>(wParam));
      }
      return 0;
    }
    if (uMsg == WM_MOUSEMOVE) {
      int mouseXNow = static_cast<int16_t>(LOWORD(lParam));
      int mouseYNow = static_cast<int16_t>(HIWORD(lParam));
      if (g_MouseDown) {
        bool modifierShift = (wParam & MK_SHIFT) == MK_SHIFT;
        bool modifierCtrl = (wParam & MK_CONTROL) == MK_CONTROL;
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
      g_MouseX = mouseXNow;
      g_MouseY = mouseYNow;
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

class AutoCloseHandle {
public:
  void operator()(HANDLE h) {
    if (h != INVALID_HANDLE_VALUE) {
      CloseHandle(h);
    }
  }
};

std::shared_ptr<Object> CreateNewWindow(const SampleRequestD3D11 &request) {
  std::shared_ptr<WindowPaintFunction> window(new WindowPaintFunction());
  std::shared_ptr<DXGISwapChain> DXGISwapChain =
      CreateDXGISwapChain(request.Device, window->m_hWindow);
  // If the client requested render via D3D11 then we'll patch a swap chain in
  // here and thunk through to their provided render function.
  if (request.Render) {
    window->m_OnPaint = [=]() {
      CComPtr<ID3D11Texture2D> textureBackbuffer;
      TRYD3D(DXGISwapChain->GetIDXGISwapChain()->GetBuffer(
          0, __uuidof(ID3D11Texture2D), (void **)&textureBackbuffer.p));
      D3D11_TEXTURE2D_DESC descBackbuffer = {};
      textureBackbuffer->GetDesc(&descBackbuffer);
      CComPtr<ID3D11Texture2D> textureDepth;
      {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = descBackbuffer.Width;
        desc.Height = descBackbuffer.Height;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.SampleDesc.Count = 1;
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        TRYD3D(request.Device->GetID3D11Device()->CreateTexture2D(
            &desc, nullptr, &textureDepth));
      }
      CComPtr<ID3D11DepthStencilView> dsvDepth;
      {
        D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
        desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        TRYD3D(request.Device->GetID3D11Device()->CreateDepthStencilView(
            textureDepth, &desc, &dsvDepth));
      }
      SampleResourcesD3D11 sampleResources = {};
      sampleResources.BackBufferTexture = textureBackbuffer;
      sampleResources.DepthStencilView = dsvDepth;
      sampleResources.TransformWorldToClip =
          TransformWorldToView * TransformViewToClip;
      sampleResources.TransformWorldToView = TransformWorldToView;
      request.Render(sampleResources);
      DXGISwapChain->GetIDXGISwapChain()->Present(0, 0);
    };
  }
  if (request.KeyDown) {
    window->m_OnKeyDown = request.KeyDown;
  }
  return std::shared_ptr<Object>(window);
}

std::shared_ptr<Object>
CreateNewWindow(std::shared_ptr<Direct3D11Device> deviceD3D11,
                std::function<void(const SampleResourcesD3D11 &)> fnRender) {
  SampleRequestD3D11 request = {};
  request.Device = deviceD3D11;
  request.Render = fnRender;
  return CreateNewWindow(request);
}

std::shared_ptr<Object>
CreateNewWindow(std::shared_ptr<Direct3D11Device> deviceD3D11,
                std::shared_ptr<IImage> image) {
  // Create a compute shader.
  CComPtr<ID3D11ComputeShader> shaderCompute;
  {
    CComPtr<ID3DBlob> blobCS = CompileShader("cs_5_0", "main", R"SHADER(
RWTexture2D<float4> renderTarget;
Texture2D<float4> userImage;

[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    renderTarget[dispatchThreadId.xy] = userImage.Load(int3(dispatchThreadId.xy, 0));
})SHADER");
    TRYD3D(deviceD3D11->GetID3D11Device()->CreateComputeShader(
        blobCS->GetBufferPointer(), blobCS->GetBufferSize(), nullptr,
        &shaderCompute));
  }
  CComPtr<ID3D11ShaderResourceView> srvImage =
      D3D11_Create_SRV(deviceD3D11->GetID3D11DeviceContext(), image.get());
  return CreateNewWindow(
      deviceD3D11, [=](const SampleResourcesD3D11 &sampleResources) {
        D3D11_TEXTURE2D_DESC descBackbuffer = {};
        sampleResources.BackBufferTexture->GetDesc(&descBackbuffer);
        CComPtr<ID3D11UnorderedAccessView> uavBackbuffer =
            D3D11_Create_UAV_From_Texture2D(deviceD3D11->GetID3D11Device(),
                                            sampleResources.BackBufferTexture);
        deviceD3D11->GetID3D11DeviceContext()->ClearState();
        // Beginning of rendering.
        deviceD3D11->GetID3D11DeviceContext()->CSSetUnorderedAccessViews(
            0, 1, &uavBackbuffer.p, nullptr);
        deviceD3D11->GetID3D11DeviceContext()->CSSetShader(shaderCompute,
                                                           nullptr, 0);
        deviceD3D11->GetID3D11DeviceContext()->CSSetShaderResources(
            0, 1, &srvImage.p);
        deviceD3D11->GetID3D11DeviceContext()->Dispatch(
            descBackbuffer.Width, descBackbuffer.Height, 1);
        deviceD3D11->GetID3D11DeviceContext()->ClearState();
        deviceD3D11->GetID3D11DeviceContext()->Flush();
      });
}

std::shared_ptr<Object>
CreateNewWindow(std::shared_ptr<Direct3D11Device> deviceD3D11,
                std::function<void(SampleRequestD3D11 &)> client) {
  SampleRequestD3D11 request;
  request.Device = deviceD3D11;
  client(request);
  return CreateNewWindow(request);
}

std::shared_ptr<Object>
CreateNewWindow(std::shared_ptr<Direct3D12Device> deviceD3D12,
                std::function<void(const SampleResourcesD3D12RTV &)> fnRender) {
  std::shared_ptr<WindowPaintFunction> window(new WindowPaintFunction());
  std::shared_ptr<DXGISwapChain> DXGISwapChain =
      CreateDXGISwapChain(deviceD3D12, window->m_hWindow);
  window->m_OnPaint = [=]() {
    CComPtr<ID3D12Resource> resourceBackbuffer;
    TRYD3D(DXGISwapChain->GetIDXGISwapChain()->GetBuffer(
        DXGISwapChain->GetIDXGISwapChain()->GetCurrentBackBufferIndex(),
        __uuidof(ID3D12Resource), (void **)&resourceBackbuffer));
    SampleResourcesD3D12RTV sampleResources = {};
    sampleResources.TransformWorldToClip =
        TransformWorldToView * TransformViewToClip;
    sampleResources.TransformWorldToView = TransformWorldToView;
    sampleResources.BackBufferResource = resourceBackbuffer;
    fnRender(sampleResources);
    DXGISwapChain->GetIDXGISwapChain()->Present(0, 0);
  };
  return std::shared_ptr<Object>(window);
}

std::shared_ptr<Object>
CreateNewWindow(std::shared_ptr<Direct3D12Device> deviceD3D12,
                std::function<void(const SampleResourcesD3D12UAV &)> fnRender) {
  CComPtr<ID3D12Resource1> uavResource;
  uint32_t uavWidth;
  uint32_t uavHeight;
  std::function<void(const SampleResourcesD3D12RTV &)> fnCreateUAVAndRender =
      [=](const SampleResourcesD3D12RTV &rtvResources) mutable {
        // Create an interposing UAV surface.
        if (uavResource == nullptr || uavWidth != RENDERTARGET_WIDTH ||
            uavHeight != RENDERTARGET_HEIGHT) {
          uavResource.Release();
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
          descResource.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
          TRYD3D(deviceD3D12->m_pDevice->CreateCommittedResource(
              &descHeapProperties,
              D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &descResource,
              D3D12_RESOURCE_STATE_COMMON, nullptr, __uuidof(ID3D12Resource1),
              (void **)&uavResource.p));
          uavResource->SetName(L"Direct3D 12 Interposing UAV");
          uavWidth = RENDERTARGET_WIDTH;
          uavHeight = RENDERTARGET_HEIGHT;
        }
        SampleResourcesD3D12UAV uavResources = {};
        uavResources.TransformWorldToClip =
            TransformWorldToView * TransformViewToClip;
        uavResources.TransformWorldToView = TransformWorldToView;
        uavResources.BackBufferResource = uavResource;
        fnRender(uavResources);
        ////////////////////////////////////////////////////////////////////////////////
        // Copy the interposing UAV to the backbuffer texture.
        D3D12_Run_Synchronously(
            deviceD3D12.get(), [&](ID3D12GraphicsCommandList4 *commandList) {
              commandList->ResourceBarrier(
                  1, &Make_D3D12_RESOURCE_BARRIER(
                         uavResource, D3D12_RESOURCE_STATE_COMMON,
                         D3D12_RESOURCE_STATE_COPY_SOURCE));
              commandList->ResourceBarrier(
                  1,
                  &Make_D3D12_RESOURCE_BARRIER(rtvResources.BackBufferResource,
                                               D3D12_RESOURCE_STATE_COMMON,
                                               D3D12_RESOURCE_STATE_COPY_DEST));
              commandList->CopyResource(rtvResources.BackBufferResource,
                                        uavResource);
              commandList->ResourceBarrier(
                  1, &Make_D3D12_RESOURCE_BARRIER(
                         uavResource, D3D12_RESOURCE_STATE_COPY_SOURCE,
                         D3D12_RESOURCE_STATE_COMMON));
              commandList->ResourceBarrier(
                  1,
                  &Make_D3D12_RESOURCE_BARRIER(rtvResources.BackBufferResource,
                                               D3D12_RESOURCE_STATE_COPY_DEST,
                                               D3D12_RESOURCE_STATE_COMMON));
            });
      };
  return CreateNewWindow(deviceD3D12, fnCreateUAVAndRender);
}

std::shared_ptr<Object> CreateNewWindow(std::shared_ptr<OpenGLDevice> deviceOGL,
                                        std::function<void()> fnRender) {
  std::shared_ptr<WindowPaintFunction> window(new WindowPaintFunction());
  HDC hDC = GetDC(window->m_hWindow);
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
  int nPixelFormat = ChoosePixelFormat(hDC, &pixelFormatDescriptor);
  if (0 == nPixelFormat)
    throw FALSE;
  // Use this format (this may only be set ONCE).
  if (TRUE != SetPixelFormat(hDC, nPixelFormat, &pixelFormatDescriptor))
    throw FALSE;
  window->m_OnPaint = [=]() {
    if (TRUE != wglMakeCurrent(hDC, deviceOGL->GetHLGRC()))
      throw FALSE;
    // Establish a viewport as big as the window area.
    RECT rectClient = {};
    GetClientRect(window->m_hWindow, &rectClient);
    glViewport(0, 0, rectClient.right - rectClient.left,
               rectClient.bottom - rectClient.top);
    fnRender();
    if (TRUE != wglMakeCurrent(hDC, nullptr))
      throw FALSE;
  };
  return std::shared_ptr<Object>(window);
}

#if VULKAN_INSTALLED
std::shared_ptr<Object>
CreateNewWindow(std::shared_ptr<VKDevice> deviceVK,
                std::shared_ptr<Direct3D12Device> deviceD3D12,
                std::function<void(VKDevice *, vk::Image)> fnRender) {
  std::shared_ptr<WindowPaintFunction> window(new WindowPaintFunction());
  std::shared_ptr<DXGISwapChain> DXGISwapChain =
      CreateDXGISwapChain(deviceD3D12, window->m_hWindow);
  window->m_OnPaint = [=]() {
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
      TRYD3D(deviceD3D12->m_pDevice->CreateCommittedResource1(
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
    TRYD3D(deviceD3D12->m_pDevice->CreateSharedHandle(
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
      vkRenderPass = deviceVK->m_vkDevice->createRenderPassUnique(rpci);
    }
    // Create a Vulkan image to render into.
    vk::UniqueImage vkImage;
    {
      vk::StructureChain<vk::ImageCreateInfo, vk::ExternalMemoryImageCreateInfo>
          structureChain = {
              vk::ImageCreateInfo(
                  {}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm,
                  {static_cast<uint32_t>(RENDERTARGET_WIDTH),
                   static_cast<uint32_t>(RENDERTARGET_HEIGHT), 1},
                  1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
                  vk::ImageUsageFlagBits::eTransferDst |
                      vk::ImageUsageFlagBits::eColorAttachment,
                  vk::SharingMode::eExclusive, {}, vk::ImageLayout::eUndefined),
              vk::ExternalMemoryImageCreateInfo(
                  vk::ExternalMemoryHandleTypeFlagBits::eD3D12Resource)};
      vkImage = deviceVK->m_vkDevice->createImageUnique(
          structureChain.get<vk::ImageCreateInfo>());
    }

    // Fake-Allocate the memory for our Vulkan image as an alias of the D3D12
    // image created above.
    vk::UniqueDeviceMemory vkDeviceMemory;
    {
      vk::MemoryRequirements mr =
          deviceVK->m_vkDevice->getImageMemoryRequirements(vkImage.get());
      vk::StructureChain<vk::MemoryAllocateInfo,
                         vk::ImportMemoryWin32HandleInfoKHR>
          sc = {vk::MemoryAllocateInfo(mr.size, deviceVK->m_memoryTypeDevice),
                vk::ImportMemoryWin32HandleInfoKHR(
                    vk::ExternalMemoryHandleTypeFlagBits::eD3D12Resource,
                    handleBackbuffer.get())};
      vkDeviceMemory = deviceVK->m_vkDevice->allocateMemoryUnique(
          sc.get<vk::MemoryAllocateInfo>());
    }

    // Connect the image with its memory.
    deviceVK->m_vkDevice->bindImageMemory(vkImage.get(), vkDeviceMemory.get(),
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
      vkImageView = deviceVK->m_vkDevice->createImageViewUnique(ivci);
    }
    /*
    // Create a framebuffer from the above Vulkan image.
    vk::UniqueFramebuffer vkFramebuffer;
    {
      std::vector<vk::ImageView> attachments = {vkImageView.get()};
      vk::FramebufferCreateInfo cfb({}, deviceVK->m_vkRenderPass.get(),
                                    attachments, defaultWidth,
                                    defaultHeight, 1);
      vkFramebuffer = deviceVK->m_vkDevice->createFramebufferUnique(cfb);
    }
    */
    // Call the renderer.
    fnRender(deviceVK.get(), vkImage.get());
    // Present the back buffer to the Windows compositor.
    {
      // Get the next buffer in the DXGI swap chain.
      CComPtr<ID3D12Resource> pD3D12SwapChainBuffer;
      TRYD3D(DXGISwapChain->GetIDXGISwapChain()->GetBuffer(
          DXGISwapChain->GetIDXGISwapChain()->GetCurrentBackBufferIndex(),
          __uuidof(ID3D12Resource), (void **)&pD3D12SwapChainBuffer));

      // Initialize a Render Target View (RTV) from this DXGI buffer.
      deviceD3D12->m_pDevice->CreateRenderTargetView(
          pD3D12SwapChainBuffer,
          &Make_D3D12_RENDER_TARGET_VIEW_DESC_SwapChainDefault(),
          deviceD3D12->m_pDescriptorHeapRTV
              ->GetCPUDescriptorHandleForHeapStart());

      // Perform a copy of our intermediate D3D12 image (written by Vulkan) to
      // the DXGI buffer.
      D3D12_Run_Synchronously(
          deviceD3D12.get(), [&](ID3D12GraphicsCommandList5 *cmd) {
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
    TRYD3D(DXGISwapChain->GetIDXGISwapChain()->Present(0, 0));
  };
  return std::shared_ptr<Object>(window);
}
#endif