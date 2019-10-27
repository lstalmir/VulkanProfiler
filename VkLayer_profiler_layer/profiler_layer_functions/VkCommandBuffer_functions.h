#pragma once
#include "VkDevice_functions_base.h"

namespace Profiler
{
    /***********************************************************************************\

    Structure:
        VkCommandBuffer_Functions

    Description:
        Set of VkCommandBuffer functions which are overloaded in this layer.

    \***********************************************************************************/
    struct VkCommandBuffer_Functions : VkDevice_Functions_Base
    {
        // vkBeginCommandBuffer
        static VKAPI_ATTR VkResult VKAPI_CALL BeginCommandBuffer(
            VkCommandBuffer commandBuffer,
            const VkCommandBufferBeginInfo* pBeginInfo );

        // vkEndCommandBuffer
        static VKAPI_ATTR VkResult VKAPI_CALL EndCommandBuffer(
            VkCommandBuffer commandBuffer );

        // vkCmdBeginRenderPass
        static VKAPI_ATTR void VKAPI_CALL CmdBeginRenderPass(
            VkCommandBuffer commandBuffer,
            const VkRenderPassBeginInfo* pBeginInfo,
            VkSubpassContents subpassContents );

        // vkCmdEndRenderPass
        static VKAPI_ATTR void VKAPI_CALL CmdEndRenderPass(
            VkCommandBuffer commandBuffer );

        // vkCmdDraw
        static VKAPI_ATTR void VKAPI_CALL CmdDraw(
            VkCommandBuffer commandBuffer,
            uint32_t vertexCount,
            uint32_t instanceCount,
            uint32_t firstVertex,
            uint32_t firstInstance );

        // vkCmdDrawIndexed
        static VKAPI_ATTR void VKAPI_CALL CmdDrawIndexed(
            VkCommandBuffer commandBuffer,
            uint32_t indexCount,
            uint32_t instanceCount,
            uint32_t firstIndex,
            int32_t vertexOffset,
            uint32_t firstInstance );
    };
}
