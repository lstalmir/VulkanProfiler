#pragma once
#include "vk_dispatch_tables.h"
#include <stdint.h>

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
