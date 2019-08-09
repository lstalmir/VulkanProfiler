#pragma once
#include "profiler_layer_functions/VkDispatch.h"
#include <vulkan/vulkan.h>

namespace Profiler
{
    /***********************************************************************************\

    Structure:
        ProfilerCallbacks

    Description:
        VkDevice functions used by the Profiler instances.

    \***********************************************************************************/
    struct ProfilerCallbacks
    {
        PFN_vkGetPhysicalDeviceQueueFamilyProperties pfnGetPhysicalDeviceQueueFamilyProperties;
        PFN_vkCreateQueryPool                        pfnCreateQueryPool;
        PFN_vkDestroyQueryPool                       pfnDestroyQueryPool;
        PFN_vkCreateCommandPool                      pfnCreateCommandPool;
        PFN_vkDestroyCommandPool                     pfnDestroyCommandPool;
        PFN_vkAllocateCommandBuffers                 pfnAllocateCommandBuffers;
        PFN_vkFreeCommandBuffers                     pfnFreeCommandBuffers;
        PFN_vkBeginCommandBuffer                     pfnBeginCommandBuffer;
        PFN_vkEndCommandBuffer                       pfnEndCommandBuffer;
        PFN_vkCmdWriteTimestamp                      pfnCmdWriteTimestamp;
        PFN_vkQueueSubmit                            pfnQueueSubmit;
    };
}
