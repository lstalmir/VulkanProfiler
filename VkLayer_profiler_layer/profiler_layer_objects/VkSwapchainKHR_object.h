#pragma once
#include "VkSurfaceKHR_object.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace Profiler
{
    struct VkSwapchainKHR_Object
    {
        VkSwapchainKHR Handle;

        VkSurfaceKHR_Object* pSurface;

        // Images created by the layer
        std::vector<VkImage> Images;
    };
}
