#pragma once
#include "VkSurfaceKhr_object.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace Profiler
{
    struct VkSwapchainKhr_Object
    {
        VkSwapchainKHR Handle;

        VkSurfaceKhr_Object* pSurface;

        // Images created by the layer
        std::vector<VkImage> Images;
    };
}
