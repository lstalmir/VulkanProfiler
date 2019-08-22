#pragma once
#include "profiler_callbacks.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include <vulkan/vulkan.h>

namespace Profiler
{
    class ProfilerBuffer
    {
    public:
        ProfilerBuffer();
        
        VkResult Initialize(
            VkDevice_Object* pDevice,
            const VkBufferCreateInfo* pCreateInfo,
            VkMemoryPropertyFlags memoryPropertyFlags,
            ProfilerCallbacks callbacks );

        void Destroy();

        VkResult Map( void** ppMappedMemoryAddress );
        void Unmap();

        VkBuffer                Buffer;
        uint32_t                Size;

    protected:
        ProfilerCallbacks       m_Callbacks;
        VkDevice                m_Device;
        VkDeviceMemory          m_DeviceMemory;
    };
}
