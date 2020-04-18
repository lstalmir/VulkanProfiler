#pragma once
#include "VkSurfaceKHR_object.h"
#include <vk_layer.h>
#include <vk_dispatch_table_helper.h>
#include <unordered_map>

namespace Profiler
{
    struct VkInstance_Object
    {
        VkInstance Handle;

        // Dispatch tables
        VkLayerInstanceDispatchTable Callbacks;

        VkApplicationInfo ApplicationInfo;

        // Surfaces created with this instance
        std::unordered_map<VkSurfaceKHR, VkSurfaceKHR_Object> Surfaces;
    };
}
