#pragma once

#if VULKAN_INSTALLED

#define VK_USE_PLATFORM_WIN32_KHR

#include <functional>
#include <memory>
#include <vulkan\vulkan.h>
#include <vulkan\vulkan.hpp>

class VKDevice {
public:
  VKDevice();

public:
  vk::UniqueInstance m_vkInstance;
  vk::UniqueDevice m_vkDevice;
  vk::Queue m_vkQueueGraphics;
  uint32_t m_memoryHeapHost;
  uint32_t m_memoryHeapDevice;
  uint32_t m_memoryTypeDevice;
  uint32_t m_memoryTypeShared;
  uint32_t m_queueGraphics;
  vk::UniqueCommandPool m_vkCommandPoolGraphics;
  vk::UniqueDescriptorPool m_vkDescriptorPool;
  vk::UniqueDescriptorSetLayout m_vkDescriptorSetLayoutUAV;
  std::vector<vk::UniqueDescriptorSet> m_vkDescriptorSetUAV;
  vk::UniqueDescriptorSetLayout m_vkDescriptorSetLayoutSRV;
  std::vector<vk::UniqueDescriptorSet> m_vkDescriptorSetSRV;
  vk::UniqueDescriptorSetLayout m_vkDescriptorSetLayoutSMP;
  std::vector<vk::UniqueDescriptorSet> m_vkDescriptorSetSMP;
  vk::UniquePipelineCache m_vkPipelineCache;
  vk::UniquePipelineLayout m_vkPipelineLayoutEmpty;
  vk::UniqueRenderPass m_vkRenderPass;
  vk::UniqueSampler m_vkSampler;
};

void VK_Run_Synchronously(VKDevice *device,
                          std::function<void(VkCommandBuffer)> fn);

std::shared_ptr<VKDevice> CreateVKDevice();

#endif // VULKAN_INSTALLED