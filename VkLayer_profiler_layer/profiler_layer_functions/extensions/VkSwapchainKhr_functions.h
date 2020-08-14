#pragma once
#include "profiler_layer_functions/core/VkDevice_functions_base.h"

namespace Profiler
{
    struct VkSwapchainKhr_Functions : VkDevice_Functions_Base
    {
        // vkCreateSwapchainKHR
        static VKAPI_ATTR VkResult VKAPI_CALL CreateSwapchainKHR(
            VkDevice device,
            const VkSwapchainCreateInfoKHR* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkSwapchainKHR* pSwapchain );

        // vkDestroySwapchainKHR
        static VKAPI_ATTR void VKAPI_CALL DestroySwapchainKHR(
            VkDevice device,
            VkSwapchainKHR swapchain,
            const VkAllocationCallbacks* pAllocator );

        // vkQueuePresentKHR
        static VKAPI_ATTR VkResult VKAPI_CALL QueuePresentKHR(
            VkQueue queue,
            const VkPresentInfoKHR* pPresentInfo );
    };
}
