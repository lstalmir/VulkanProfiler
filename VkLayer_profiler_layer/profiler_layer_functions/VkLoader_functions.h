#pragma once
#include <vulkan/vk_layer.h>

namespace Profiler
{
    class VkLoader_Functions
    {
    public:
        // vkSetInstanceLoaderData
        static VKAPI_ATTR VkResult VKAPI_CALL SetInstanceLoaderData(
            VkInstance instance,
            void* pObject );

        // vkSetDeviceLoaderData
        static VKAPI_ATTR VkResult VKAPI_CALL SetDeviceLoaderData(
            VkDevice device,
            void* pObject );

        // vkEnumerateInstanceVersion
        static VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceVersion(
            uint32_t* pVersion );
    };
}
