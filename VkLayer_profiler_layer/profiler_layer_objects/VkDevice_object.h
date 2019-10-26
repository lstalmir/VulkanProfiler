#pragma once
#include "VkInstance_object.h"
#include "VkQueue_object.h"
#include <vulkan.h>

namespace Profiler
{
    /***********************************************************************************\

    Class:
        VkDevice_Object

    Description:
        Extension to the VkDevice object, which holds additional information on the
        queues associated with the device and parent physical device object.

    \***********************************************************************************/
    struct VkDevice_Object
    {
        VkDevice                    Device;
        VkPhysicalDevice            PhysicalDevice;
        VkDeviceQueues_Object       Queues;
        PFN_vkGetDeviceProcAddr     pfnGetDeviceProcAddr;
        PFN_vkGetInstanceProcAddr   pfnGetInstanceProcAddr;
    };
}
