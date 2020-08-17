#pragma once
#include "vk_dispatch_tables.h"
#include "VkSurfaceKhr_object.h"
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
