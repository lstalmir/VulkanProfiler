#pragma once
#include <vk_layer.h>

namespace Profiler
{
    struct VkSurfaceKHR_Object
    {
        VkSurfaceKHR Handle;
        void* WindowHandle;
    };
}
