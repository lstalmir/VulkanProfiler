#pragma once
#include "VkInstance_functions_base.h"

namespace Profiler
{
    struct VkWin32SurfaceKhr_Functions : VkInstance_Functions_Base
    {
        #ifdef VK_USE_PLATFORM_WIN32_KHR
        // vkCreateWin32SurfaceKHR
        static VKAPI_ATTR VkResult VKAPI_CALL CreateWin32SurfaceKHR(
            VkInstance instance,
            const VkWin32SurfaceCreateInfoKHR* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkSurfaceKHR* pSurface );
        #endif
    };
}
