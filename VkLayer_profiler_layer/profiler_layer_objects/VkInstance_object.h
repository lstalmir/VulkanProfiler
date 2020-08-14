#pragma once
#include "VkSurfaceKhr_object.h"
#include <vulkan/vk_layer.h>
#include <vk_dispatch_table_helper.h>
#include <unordered_map>

namespace Profiler
{
    struct VkInstance_Object
    {
        VkInstance Handle;

        // Dispatch tables
        VkLayerInstanceDispatchTable Callbacks;
        PFN_vkSetInstanceLoaderData SetInstanceLoaderData;

        VkApplicationInfo ApplicationInfo;

        uint32_t LoaderVersion;

        // Surfaces created with this instance
        std::unordered_map<VkSurfaceKHR, VkSurfaceKhr_Object> Surfaces;
    };
}
