#if VULKAN_INSTALLED

#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include "Core_DXGI.h"
#include "Core_ISample.h"
#include "Core_Object.h"
#include "Core_Util.h"
#include "Core_VK.h"
#include <atlbase.h>
#include <d3d12.h>
#include <functional>
#include <memory>
#include <vector>
#include <vulkan\vulkan.h>

class AutoCloseHandle {
public:
  void operator()(HANDLE h) {
    if (h != INVALID_HANDLE_VALUE) {
      CloseHandle(h);
    }
  }
};

class Sample_VKBasic : public Object, public ISample {
private:
  std::function<void()> m_fnRender;

public:
  Sample_VKBasic(std::shared_ptr<DXGISwapChain> swapchain,
                 std::shared_ptr<VKDevice> deviceVK,
                 std::shared_ptr<Direct3D12Device> m_pDeviceD3D12) {
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
      TRYD3D(m_pDeviceD3D12->m_pDevice->CreateCommittedResource1(
          &descHeapProperties,
          D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES |
              D3D12_HEAP_FLAG_SHARED,
          &descResource, D3D12_RESOURCE_STATE_COMMON, &descClear, nullptr,
          __uuidof(ID3D12Resource1), (void **)&resourceBackbuffer.p));
      resourceBackbuffer->SetName(L"D3D12Resource (Backbuffer)");
    }

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

    m_fnRender = [=]() {
      // Open a shared NT handle to the image (true HANDLE; must be closed
      // later).
      std::unique_ptr<HANDLE, AutoCloseHandle> handleBackbuffer;
      TRYD3D(m_pDeviceD3D12->m_pDevice->CreateSharedHandle(
          resourceBackbuffer, nullptr, GENERIC_ALL, nullptr,
          (HANDLE *)&handleBackbuffer));

      // Create a Vulkan image to render into.
      vk::UniqueImage vkImage;
      {
        vk::StructureChain<vk::ImageCreateInfo,
                           vk::ExternalMemoryImageCreateInfo>
            structureChain = {
                vk::ImageCreateInfo(
                    {}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm,
                    {RENDERTARGET_WIDTH, RENDERTARGET_HEIGHT, 1}, 1, 1,
                    vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
                    vk::ImageUsageFlagBits::eTransferDst |
                        vk::ImageUsageFlagBits::eColorAttachment,
                    vk::SharingMode::eExclusive, {},
                    vk::ImageLayout::eUndefined),
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
            {}, vkImage.get(), vk::ImageViewType::e2D,
            vk::Format::eR8G8B8A8Unorm,
            vk::ComponentMapping(
                vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
                vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA),
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0,
                                      1));
        vkImageView = deviceVK->m_vkDevice->createImageViewUnique(ivci);
      }

      // Create a framebuffer from the above Vulkan image.
      vk::UniqueFramebuffer vkFramebuffer;
      {
        std::vector<vk::ImageView> attachments = {vkImageView.get()};
        vk::FramebufferCreateInfo cfb({}, deviceVK->m_vkRenderPass.get(),
                                      attachments, RENDERTARGET_WIDTH,
                                      RENDERTARGET_HEIGHT, 1);
        vkFramebuffer = deviceVK->m_vkDevice->createFramebufferUnique(cfb);
      }

      // Perform a clear of the Vulkan image via a dispatched command buffer.
      VK_Run_Synchronously(
          deviceVK.get(), [&](vk::CommandBuffer m_vkCommandBuffer) {
            {
              std::vector<vk::ImageMemoryBarrier> imb = {vk::ImageMemoryBarrier(
                  (vk::AccessFlagBits)0, (vk::AccessFlagBits)0,
                  vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
                  VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                  vkImage.get(),
                  vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0,
                                            1, 0, 1))};
              m_vkCommandBuffer.pipelineBarrier(
                  vk::PipelineStageFlagBits::eTopOfPipe,
                  vk::PipelineStageFlagBits::eTransfer,
                  (vk::DependencyFlagBits)0, {}, {}, imb);
            }
            {
              static float colorStrobe = 0;
              std::vector<vk::ImageSubresourceRange> isr = {
                  vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0,
                                            1, 0, 1)};
              m_vkCommandBuffer.clearColorImage(
                  vkImage.get(), vk::ImageLayout::eGeneral,
                  vk::ClearColorValue(
                      std::array<float, 4>{colorStrobe, 0, 0, 1}),
                  isr);
              colorStrobe += 0.1f;
              if (colorStrobe > 1)
                colorStrobe -= 1;
            }
          });

      {
        // Get the next buffer in the DXGI swap chain.
        CComPtr<ID3D12Resource> pD3D12SwapChainBuffer;
        TRYD3D(swapchain->GetIDXGISwapChain()->GetBuffer(
            swapchain->GetIDXGISwapChain()->GetCurrentBackBufferIndex(),
            __uuidof(ID3D12Resource), (void **)&pD3D12SwapChainBuffer));

        // Initialize a Render Target View (RTV) from this DXGI buffer.
        m_pDeviceD3D12->m_pDevice->CreateRenderTargetView(
            pD3D12SwapChainBuffer,
            &Make_D3D12_RENDER_TARGET_VIEW_DESC_SwapChainDefault(),
            m_pDeviceD3D12->m_pDescriptorHeapRTV
                ->GetCPUDescriptorHandleForHeapStart());

        // Perform a copy of our intermediate D3D12 image (written by Vulkan) to
        // the DXGI buffer.
        D3D12_Run_Synchronously(
            m_pDeviceD3D12.get(), [&](ID3D12GraphicsCommandList5 *cmd) {
              cmd->ResourceBarrier(1, &Make_D3D12_RESOURCE_BARRIER(
                                          pD3D12SwapChainBuffer,
                                          D3D12_RESOURCE_STATE_COMMON,
                                          D3D12_RESOURCE_STATE_COPY_DEST));
              cmd->CopyResource(pD3D12SwapChainBuffer, resourceBackbuffer);
              cmd->ResourceBarrier(1, &Make_D3D12_RESOURCE_BARRIER(
                                          pD3D12SwapChainBuffer,
                                          D3D12_RESOURCE_STATE_COPY_DEST,
                                          D3D12_RESOURCE_STATE_PRESENT));
            });
      }

      // Present the back buffer to the Windows compositor.
      TRYD3D(swapchain->GetIDXGISwapChain()->Present(0, 0));
    };
  }
  void Render() override { m_fnRender(); }
};

std::shared_ptr<ISample>
CreateSample_VKBasic(std::shared_ptr<DXGISwapChain> swapchain,
                     std::shared_ptr<VKDevice> device,
                     std::shared_ptr<Direct3D12Device> pDevice12) {
  return std::shared_ptr<ISample>(
      new Sample_VKBasic(swapchain, device, pDevice12));
}

#endif // VULKAN_INSTALLED