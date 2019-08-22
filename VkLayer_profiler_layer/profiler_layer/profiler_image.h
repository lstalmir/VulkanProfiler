#pragma once
#include "profiler_callbacks.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include <vulkan/vulkan.h>

namespace Profiler
{
    class ProfilerImage
    {
    public:
        ProfilerImage();

        VkResult Initialize(
            VkDevice_Object* pDevice,
            const VkImageCreateInfo* pCreateInfo,
            VkMemoryPropertyFlags memoryPropertyFlags,
            ProfilerCallbacks callbacks );

        void Destroy();

        void LayoutTransition( VkCommandBuffer commandBuffer, VkImageLayout newLayout );

        VkImage                 Image;
        VkImageLayout           Layout;

    protected:
        ProfilerCallbacks       m_Callbacks;
        VkDevice                m_Device;
        VkDeviceMemory          m_DeviceMemory;
    };
}
