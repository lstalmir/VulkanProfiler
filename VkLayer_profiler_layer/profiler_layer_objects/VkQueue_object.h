#pragma once
#include <vector>
#include <unordered_map>
#include <vulkan/vk_layer.h>

namespace Profiler
{
    /***********************************************************************************\

    Class:
        VkQueue_Object

    Description:
        Extension to the VkQueue object, which holds additional information on the
        queue family, priority, its index and parent device object of the queue.

    \***********************************************************************************/
    struct VkQueue_Object
    {
        VkDevice Device;
        VkQueue  Queue;
        uint32_t Index;
        uint32_t FamilyIndex;
        float    Priority;
    };

    typedef std::unordered_map<VkQueue, VkQueue_Object> VkDeviceQueues_Object;
}
