#if VULKAN_INSTALLED

#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include "Core_VK.h"
#include "Sample.h"
#include "Core_DXGI.h"
#include "Core_Window.h"

#include <d3d12.h>
#include <vulkan\vulkan.h>

#include <atlbase.h>
#include <functional>
#include <memory>
#include <vector>

class AutoCloseHandle
{
public:
    void operator()(HANDLE h)
    {
        if (h != INVALID_HANDLE_VALUE)
        {
            CloseHandle(h);
        }
    }
};

class Sample_VKBasic : public Sample
{
    std::shared_ptr<DXGISwapChain> m_pSwapChain;
    std::shared_ptr<VKDevice> m_pDeviceVK;
    std::shared_ptr<Direct3D12Device> m_pDeviceD3D12;
public:
    Sample_VKBasic(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<VKDevice> pDeviceVK, std::shared_ptr<Direct3D12Device> pDeviceD3D12) :
        m_pSwapChain(pSwapChain),
        m_pDeviceVK(pDeviceVK),
        m_pDeviceD3D12(pDeviceD3D12)
    {
    }
    void Render() override
    {
        // We can't directly use the image from DXGI, probably because it's a D3D9Ex image.
        // Create a shared D3D12 image to use as our render target and copy it back later.
        CComPtr<ID3D12Resource1> pD3D12Backbuffer;
        {
            D3D12_CLEAR_VALUE descClear = {};
            descClear.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            descClear.DepthStencil.Depth = 1.0f;
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
                TRYD3D(m_pDeviceD3D12->m_pDevice->CreateCommittedResource1(&descHeapProperties, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES | D3D12_HEAP_FLAG_SHARED, &descResource, D3D12_RESOURCE_STATE_COMMON, &descClear, nullptr, __uuidof(ID3D12Resource1), (void**)&pD3D12Backbuffer.p));
                pD3D12Backbuffer->SetName(L"D3D12Resource (Backbuffer)");
            }
        }
        
        // Open a shared NT handle to the image (true HANDLE; must be closed later).
        std::unique_ptr<HANDLE, AutoCloseHandle> hD3D12Backbuffer;
        TRYD3D(m_pDeviceD3D12->m_pDevice->CreateSharedHandle(pD3D12Backbuffer, nullptr, GENERIC_ALL, nullptr, (HANDLE*)&hD3D12Backbuffer));
        
        // Create a Vulkan image to render into.
        vk::UniqueImage vkImage;
        {
            vk::StructureChain<vk::ImageCreateInfo, vk::ExternalMemoryImageCreateInfo> structureChain = {
                vk::ImageCreateInfo ({}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm, { RENDERTARGET_WIDTH, RENDERTARGET_HEIGHT, 1 }, 1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive, {}, vk::ImageLayout::eUndefined),
                vk::ExternalMemoryImageCreateInfo(vk::ExternalMemoryHandleTypeFlagBits::eD3D12Resource)
            };
            vkImage = m_pDeviceVK->m_vkDevice->createImageUnique(structureChain.get<vk::ImageCreateInfo>());
        }

        // Fake-Allocate the memory for our Vulkan image as an alias of the D3D12 image created above.
        vk::UniqueDeviceMemory vkDeviceMemory;
        {
            vk::MemoryRequirements mr = m_pDeviceVK->m_vkDevice->getImageMemoryRequirements(vkImage.get());
            vk::StructureChain<vk::MemoryAllocateInfo, vk::ImportMemoryWin32HandleInfoKHR> sc = {
                vk::MemoryAllocateInfo(mr.size, m_pDeviceVK->m_memoryTypeDevice),
                vk::ImportMemoryWin32HandleInfoKHR(vk::ExternalMemoryHandleTypeFlagBits::eD3D12Resource, hD3D12Backbuffer.get())
            };
            vkDeviceMemory = m_pDeviceVK->m_vkDevice->allocateMemoryUnique(sc.get<vk::MemoryAllocateInfo>());
        }

        // Connect the image with its memory.
        m_pDeviceVK->m_vkDevice->bindImageMemory(vkImage.get(), vkDeviceMemory.get(), 0);
        
        // Create an image view of the above Vulkan image.
        vk::UniqueImageView vkImageView;
        {
            vk::ImageViewCreateInfo ivci({}, vkImage.get(), vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Unorm, vk::ComponentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
            vkImageView = m_pDeviceVK->m_vkDevice->createImageViewUnique(ivci);
        }

        // Create a framebuffer from the above Vulkan image.
        vk::UniqueFramebuffer vkFramebuffer;
        {
            std::vector<vk::ImageView> attachments = { vkImageView.get() };
            vk::FramebufferCreateInfo cfb({}, m_pDeviceVK->m_vkRenderPass.get(), attachments, RENDERTARGET_WIDTH, RENDERTARGET_HEIGHT, 1);
            vkFramebuffer = m_pDeviceVK->m_vkDevice->createFramebufferUnique(cfb);
        }

        // Create a render pass.
        vk::UniqueRenderPass vkRenderPass;
        {
            std::vector<vk::AttachmentDescription> ad = { vk::AttachmentDescription({}, vk::Format::eR8G8B8A8Unorm, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR) };
            std::vector<vk::AttachmentReference> ar = { vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal) };
            std::vector<vk::SubpassDescription> spd = { vk::SubpassDescription({}, vk::PipelineBindPoint::eGraphics, {}, ar) };
            vk::RenderPassCreateInfo rpci((vk::RenderPassCreateFlagBits)0, ad, spd);
            vkRenderPass = m_pDeviceVK->m_vkDevice->createRenderPassUnique(rpci);
        }

        // Perform a clear of the Vulkan image via a dispatched command buffer.
        VKRunOnGPU(m_pDeviceVK.get(), [&] (vk::CommandBuffer m_vkCommandBuffer) {
            {
                std::vector<vk::ImageMemoryBarrier> imb = { vk::ImageMemoryBarrier((vk::AccessFlagBits)0, (vk::AccessFlagBits)0, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, vkImage.get(), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)) };
                m_vkCommandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, (vk::DependencyFlagBits)0, {}, {}, imb);
            }
            {
                static float colorStrobe = 0;
                std::vector<vk::ImageSubresourceRange> isr = { vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1) };
                m_vkCommandBuffer.clearColorImage(vkImage.get(), vk::ImageLayout::eGeneral, vk::ClearColorValue(std::array<float, 4> {colorStrobe, 0, 0, 1}), isr);
                colorStrobe += 0.1f;
                if (colorStrobe > 1) colorStrobe -= 1;
            }
        });

        {
            // Get the next buffer in the DXGI swap chain.
            CComPtr<ID3D12Resource> pD3D12SwapChainBuffer;
            TRYD3D(m_pSwapChain->GetIDXGISwapChain()->GetBuffer(m_pSwapChain->GetIDXGISwapChain()->GetCurrentBackBufferIndex(), __uuidof(ID3D12Resource), (void**)&pD3D12SwapChainBuffer));
            
            // Initialize a Render Target View (RTV) from this DXGI buffer.
            {
                D3D12_RENDER_TARGET_VIEW_DESC descRTV = {};
                descRTV.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
                descRTV.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                m_pDeviceD3D12->m_pDevice->CreateRenderTargetView(pD3D12SwapChainBuffer, &descRTV, m_pDeviceD3D12->m_pDescriptorHeapRTV->GetCPUDescriptorHandleForHeapStart());
            }

            // Perform a copy of our intermediate D3D12 image (written by Vulkan) to the DXGI buffer.
            RunOnGPU(m_pDeviceD3D12.get(), [&](ID3D12GraphicsCommandList5 *cmd)
                {
                    cmd->ResourceBarrier(1, &D3D12MakeResourceTransitionBarrier(pD3D12SwapChainBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
                    cmd->CopyResource(pD3D12SwapChainBuffer, pD3D12Backbuffer);
                    cmd->ResourceBarrier(1, &D3D12MakeResourceTransitionBarrier(pD3D12SwapChainBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT));
               });            
        }

        // Present the back buffer to the Windows compositor.
        TRYD3D(m_pSwapChain->GetIDXGISwapChain()->Present(0, 0));
    }
};

std::shared_ptr<Sample> CreateSample_VKBasic(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<VKDevice> pDevice, std::shared_ptr<Direct3D12Device> pDevice12)
{
    return std::shared_ptr<Sample>(new Sample_VKBasic(pSwapChain, pDevice, pDevice12));
}

#endif // VULKAN_INSTALLED