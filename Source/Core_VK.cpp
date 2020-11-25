#if VULKAN_INSTALLED

#include "Core_VK.h"

#pragma comment(lib, "vulkan-1.lib")

template <class _Container, class _Pr>
typename _Container::const_iterator find_iterator(const _Container& _Array, _Pr _Predicate)
{
    auto find = std::find_if(_Array.begin(), _Array.end(), _Predicate);
    if (find == _Array.end())
    {
        throw std::exception("Unable to find requested item.");
    }
    return find;
}

template <class _Container, class _Pr>
uint32_t find_index(const _Container& _Array, _Pr _Predicate)
{
    return find_iterator(_Array, _Predicate) - _Array.begin();
}

template <class _Container, class _Pr>
typename _Container::value_type find_item(const _Container& _Array, _Pr _Predicate)
{
    return *find_iterator(_Array, _Predicate);
}

VKDevice::VKDevice()
{
    // Create a Vulkan instance.
    {
        vk::ApplicationInfo descApp("DrawingContextVulkan", 1, "Vulkan 1.0", 1, VK_API_VERSION_1_0);
        std::vector<const char*> layerNames { "VK_LAYER_KHRONOS_validation" };
        vk::InstanceCreateInfo descInst({}, &descApp, layerNames );
        m_vkInstance = vk::UniqueInstance(vk::createInstanceUnique(descInst));
    }

    // Find the first available discrete GPU.
    vk::PhysicalDevice physicalDevice = find_item(m_vkInstance->enumeratePhysicalDevices(), [&] (const vk::PhysicalDevice& t) {
        vk::PhysicalDeviceProperties p;
        t.getProperties(&p);
        return p.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
    });

    // Find the graphics queue index for this device.
    m_queueGraphics = find_index(physicalDevice.getQueueFamilyProperties(), [&] (const vk::QueueFamilyProperties& t) { return t.queueCount > 0 && t.queueFlags & vk::QueueFlagBits::eGraphics; });

    // Locate the host and device memory heaps and types.
    {
        auto list = physicalDevice.getMemoryProperties();
        m_memoryHeapDevice = find_index(list.memoryHeaps, [&] (const vk::MemoryHeap& t) { return t.flags == vk::MemoryHeapFlagBits::eDeviceLocal; });
        m_memoryHeapHost = find_index(list.memoryHeaps, [&] (const vk::MemoryHeap& t) { return t.flags == (vk::MemoryHeapFlagBits)0; });
        m_memoryTypeDevice = find_index(list.memoryTypes, [&] (const vk::MemoryType& t) { return t.heapIndex == m_memoryHeapDevice && (t.propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal); });
        m_memoryTypeShared = find_index(list.memoryTypes, [&] (const vk::MemoryType& t) { return t.heapIndex == m_memoryHeapHost && (t.propertyFlags & (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)); });
    }

    // Create the device.
    { 
        // Find all extensions; we're going to enable them all.
        std::vector<const char*> extensionNames;
        auto extensions = physicalDevice.enumerateDeviceExtensionProperties();
        for (const auto& ext : extensions)
        {
            extensionNames.push_back(ext.extensionName);
        }
        std::array<float, 1> priorities = { 1.0f };
        vk::DeviceCreateInfo dci({}, vk::DeviceQueueCreateInfo { {}, m_queueGraphics, priorities }, {}, extensionNames);
        m_vkDevice = physicalDevice.createDeviceUnique(dci);
    }

    // Create a graphics command queue.
    m_vkQueueGraphics = m_vkDevice->getQueue(m_queueGraphics, 0);

    // Create a graphics command buffer pool.
    m_vkCommandPoolGraphics = m_vkDevice->createCommandPoolUnique(vk::CommandPoolCreateInfo { {}, m_queueGraphics });

    // Create the descriptor pool and layouts(s).
    m_vkDescriptorPool = m_vkDevice->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo({}, 64, std::vector<vk::DescriptorPoolSize> { { vk::DescriptorType::eUniformBuffer, 64 }, { vk::DescriptorType::eSampledImage, 64 }, { vk::DescriptorType::eSampler, 64 }}));
    m_vkDescriptorSetLayoutUAV = m_vkDevice->createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo ({}, std::vector<vk::DescriptorSetLayoutBinding> { vk::DescriptorSetLayoutBinding({}, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex) }));
    m_vkDescriptorSetUAV = m_vkDevice->allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo (m_vkDescriptorPool.get(), std::vector<vk::DescriptorSetLayout> { m_vkDescriptorSetLayoutUAV.get() }));
    m_vkDescriptorSetLayoutSRV = m_vkDevice->createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo ({}, std::vector<vk::DescriptorSetLayoutBinding> { vk::DescriptorSetLayoutBinding({}, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment) }));
    m_vkDescriptorSetSRV = m_vkDevice->allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo (m_vkDescriptorPool.get(), std::vector<vk::DescriptorSetLayout> { m_vkDescriptorSetLayoutSRV.get() }));
    m_vkDescriptorSetLayoutSMP = m_vkDevice->createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo ({}, std::vector<vk::DescriptorSetLayoutBinding> { vk::DescriptorSetLayoutBinding({}, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eFragment) }));
    m_vkDescriptorSetSMP = m_vkDevice->allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo (m_vkDescriptorPool.get(), std::vector<vk::DescriptorSetLayout> { m_vkDescriptorSetLayoutSMP.get() }));

    // Create a pipeline cache.
    m_vkPipelineCache = m_vkDevice->createPipelineCacheUnique(vk::PipelineCacheCreateInfo({}));

    // Create a pipeline layout.
    m_vkPipelineLayoutEmpty = m_vkDevice->createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({}, std::vector<vk::DescriptorSetLayout> { m_vkDescriptorSetLayoutUAV.get(), m_vkDescriptorSetLayoutSRV.get(), m_vkDescriptorSetLayoutSMP.get() }));

    // Create a render pass.
    {
        std::vector<vk::AttachmentDescription> att { vk::AttachmentDescription({}, vk::Format::eR8G8B8A8Unorm, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR) };
        std::vector<vk::AttachmentReference> atr { std::vector<vk::AttachmentReference> { vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal) }};
        std::vector<vk::SubpassDescription> spd { vk::SubpassDescription({}, vk::PipelineBindPoint::eGraphics, {}, atr) };
        m_vkRenderPass = m_vkDevice->createRenderPassUnique(vk::RenderPassCreateInfo({}, att, spd));
    }

    // Create a sampler.
    m_vkSampler = m_vkDevice->createSamplerUnique(vk::SamplerCreateInfo { {}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eNearest, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat });
}

void VK_Run_Synchronously(VKDevice* device, std::function<void(VkCommandBuffer)> fn)
{
    std::vector<vk::UniqueCommandBuffer> m_vkCommandBuffer = device->m_vkDevice->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo { device->m_vkCommandPoolGraphics.get(), vk::CommandBufferLevel::ePrimary, 1 });
    // Create a command buffer and start filling it.
    m_vkCommandBuffer[0]->begin(vk::CommandBufferBeginInfo { vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    fn(m_vkCommandBuffer[0].get());
    // Finish up command buffer generation.
    m_vkCommandBuffer[0]->end();
    // Submit the command buffer to GPU.
    std::vector<vk::CommandBuffer> commandBuffers { m_vkCommandBuffer[0].get() };
    vk::SubmitInfo submitInfo({}, {}, commandBuffers, {});
    device->m_vkQueueGraphics.submit(std::vector<vk::SubmitInfo> { submitInfo }, {});
    // Wait until the whole queue has been executed and the GPU goes idle.
    device->m_vkQueueGraphics.waitIdle();
};

std::shared_ptr<VKDevice> CreateVKDevice()
{
    return std::shared_ptr<VKDevice>(new VKDevice());
}

#endif // VULKAN_INSTALLED