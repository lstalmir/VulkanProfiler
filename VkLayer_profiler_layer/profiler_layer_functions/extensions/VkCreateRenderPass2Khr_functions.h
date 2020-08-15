#pragma once
#include "VkDevice_functions_base.h"

namespace Profiler
{
    struct VkCreateRenderPass2Khr_Functions : VkDevice_Functions_Base
    {
        // vkCreateRenderPass2KHR
        static VKAPI_ATTR VkResult VKAPI_CALL CreateRenderPass2KHR(
            VkDevice device,
            const VkRenderPassCreateInfo2KHR* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkRenderPass* pRenderPass );

        // vkCmdBeginRenderPass2KHR
        static VKAPI_ATTR void VKAPI_CALL CmdBeginRenderPass2KHR(
            VkCommandBuffer commandBuffer,
            const VkRenderPassBeginInfo* pBeginInfo,
            const VkSubpassBeginInfoKHR* pSubpassBeginInfo );

        // vkCmdEndRenderPass2KHR
        static VKAPI_ATTR void VKAPI_CALL CmdEndRenderPass2KHR(
            VkCommandBuffer commandBuffer,
            const VkSubpassEndInfoKHR* pSubpassEndInfo );

        // vkCmdNextSubpass2KHR
        static VKAPI_ATTR void VKAPI_CALL CmdNextSubpass2KHR(
            VkCommandBuffer commandBuffer,
            const VkSubpassBeginInfoKHR* pSubpassBeginInfo,
            const VkSubpassEndInfoKHR* pSubpassEndInfo );
    };
}
