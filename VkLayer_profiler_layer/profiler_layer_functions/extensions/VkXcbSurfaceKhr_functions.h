#pragma once
#include "VkInstance_functions_base.h"

namespace Profiler
{
    struct VkXcbSurfaceKhr_Functions : VkInstance_Functions_Base
    {
        #ifdef VK_USE_PLATFORM_XCB_KHR
        // vkCreateXcbSurfaceKHR
        static VKAPI_ATTR VkResult VKAPI_CALL CreateXcbSurfaceKHR(
            VkInstance instance,
            const VkXcbSurfaceCreateInfoKHR* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkSurfaceKHR* pSurface );
        #endif
    };
}
