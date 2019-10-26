#pragma once
#include <unordered_map>
#include <vulkan.h>

namespace Profiler
{
    /***********************************************************************************\

    Class:
        VkQueue_Object

    Description:
        Extension to the VkQueue object, which holds additional information on the
        queue family, priority and its index.

    \***********************************************************************************/
    struct VkQueue_Object
    {
        VkQueue  Queue;
        uint32_t Index;
        uint32_t FamilyIndex;
        float    Priority;
    };
}
