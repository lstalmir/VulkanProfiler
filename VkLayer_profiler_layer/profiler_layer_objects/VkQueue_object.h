#pragma once
#include <stdint.h>
#include <vk_layer.h>

namespace Profiler
{
    struct VkQueue_Object
    {
        VkQueue Handle;
        VkQueueFlags Flags;
        uint32_t Family;
        uint32_t Index;
    };
}
