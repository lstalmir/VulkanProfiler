#pragma once
#include "VkInstance_functions_base.h"

namespace Profiler
{
    struct VkXlibSurfaceKhr_Functions : VkInstance_Functions_Base
    {
        #ifdef VK_USE_PLATFORM_XLIB_KHR
        // vkCreateXlibSurfaceKHR
        static VKAPI_ATTR VkResult VKAPI_CALL CreateXlibSurfaceKHR(
            VkInstance instance,
            const VkXlibSurfaceCreateInfoKHR* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkSurfaceKHR* pSurface );
        #endif
    };
}
