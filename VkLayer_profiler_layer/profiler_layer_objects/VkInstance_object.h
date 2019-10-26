#pragma once
#include <vulkan/vk_layer.h>

namespace Profiler
{
    struct VkInstance_Object
    {
        VkInstance                  Instance;
        PFN_vkGetInstanceProcAddr   pfnGetInstanceProcAddr;
    };
}
