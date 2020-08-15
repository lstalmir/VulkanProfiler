#pragma once
#include "VkInstance_functions_base.h"

namespace Profiler
{
    struct VkSurfaceKhr_Functions : VkInstance_Functions_Base
    {
        // vkDestroySurfaceKHR
        static VKAPI_ATTR void VKAPI_CALL DestroySurfaceKHR(
            VkInstance instance,
            VkSurfaceKHR surface,
            const VkAllocationCallbacks* pAllocator );
    };
}
