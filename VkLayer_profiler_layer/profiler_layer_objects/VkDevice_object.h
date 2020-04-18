#pragma once
#include "VkInstance_object.h"
#include "VkQueue_object.h"
#include <vector>
#include <vk_layer.h>
#include <vk_dispatch_table_helper.h>

namespace Profiler
{
    struct VkDevice_Object
    {
        VkDevice Handle;

        VkInstance_Object* pInstance;
        VkPhysicalDevice PhysicalDevice;

        // Dispatch tables
        VkLayerDispatchTable Callbacks;

        std::vector<VkQueue_Object> Queues;
    };
}
