#pragma once
#include "VkInstance_functions_base.h"

namespace Profiler
{
    struct VkWaylandSurfaceKhr_Functions : VkInstance_Functions_Base
    {
        #ifdef VK_USE_PLATFORM_WAYLAND_KHR
        // vkCreateWin32SurfaceKHR
        static VKAPI_ATTR VkResult VKAPI_CALL CreateWaylandSurfaceKHR(
            VkInstance instance,
            const VkWaylandSurfaceCreateInfoKHR* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkSurfaceKHR* pSurface );
        #endif
    };
}
