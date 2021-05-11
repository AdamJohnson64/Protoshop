#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include "Core_DXGI.h"
#include "Core_Object.h"
#include "Core_Util.h"
#include "Core_VK.h"
#include <atlbase.h>
#include <d3d12.h>
#include <functional>
#include <memory>
#include <vector>
#include <vulkan\vulkan.h>

std::function<void(VKDevice *, vk::Image)> CreateSample_VKBasic() {

  return [=](VKDevice *deviceVK, vk::Image imageBackbuffer) {
    // Perform a clear of the Vulkan image via a dispatched command buffer.
    VK_Run_Synchronously(deviceVK, [&](vk::CommandBuffer m_vkCommandBuffer) {
      {
        std::vector<vk::ImageMemoryBarrier> imb = {vk::ImageMemoryBarrier(
            (vk::AccessFlagBits)0, (vk::AccessFlagBits)0,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, imageBackbuffer,
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0,
                                      1))};
        m_vkCommandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                                          vk::PipelineStageFlagBits::eTransfer,
                                          (vk::DependencyFlagBits)0, {}, {},
                                          imb);
      }
      {
        static float colorStrobe = 0;
        std::vector<vk::ImageSubresourceRange> isr = {vk::ImageSubresourceRange(
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)};
        m_vkCommandBuffer.clearColorImage(
            imageBackbuffer, vk::ImageLayout::eGeneral,
            vk::ClearColorValue(std::array<float, 4>{colorStrobe, 0, 0, 1}),
            isr);
        colorStrobe += 0.1f;
        if (colorStrobe > 1)
          colorStrobe -= 1;
      }
    });
  };
}