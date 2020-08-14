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

        // vkCmdNextSubpass
        static VKAPI_ATTR void VKAPI_CALL CmdNextSubpass(
            VkCommandBuffer commandBuffer,
            VkSubpassContents contents );

        // vkCmdBeginRenderPass2
        static VKAPI_ATTR void VKAPI_CALL CmdBeginRenderPass2(
            VkCommandBuffer commandBuffer,
            const VkRenderPassBeginInfo* pBeginInfo,
            const VkSubpassBeginInfo* pSubpassBeginInfo );

        // vkCmdEndRenderPass2
        static VKAPI_ATTR void VKAPI_CALL CmdEndRenderPass2(
            VkCommandBuffer commandBuffer,
            const VkSubpassEndInfo* pSubpassEndInfo );

        // vkCmdNextSubpass2
        static VKAPI_ATTR void VKAPI_CALL CmdNextSubpass2(
            VkCommandBuffer commandBuffer,
            const VkSubpassBeginInfo* pSubpassBeginInfo,
            const VkSubpassEndInfo* pSubpassEndInfo );

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

        // vkCmdBindPipeline
        static VKAPI_ATTR void VKAPI_CALL CmdBindPipeline(
            VkCommandBuffer commandBuffer,
            VkPipelineBindPoint bindPoint,
            VkPipeline pipeline );

        // vkCmdExecuteCommands
        static VKAPI_ATTR void VKAPI_CALL CmdExecuteCommands(
            VkCommandBuffer commandBuffer,
            uint32_t commandBufferCount,
            const VkCommandBuffer* pCommandBuffers );

        // vkPipelineBarrier
        static VKAPI_ATTR void VKAPI_CALL CmdPipelineBarrier(
            VkCommandBuffer commandBuffer,
            VkPipelineStageFlags srcStageMask,
            VkPipelineStageFlags dstStageMask,
            VkDependencyFlags dependencyFlags,
            uint32_t memoryBarrierCount,
            const VkMemoryBarrier* pMemoryBarriers,
            uint32_t bufferMemoryBarrierCount,
            const VkBufferMemoryBarrier* pBufferMemoryBarriers,
            uint32_t imageMemoryBarrierCount,
            const VkImageMemoryBarrier* pImageMemoryBarriers );

        // vkCmdDraw
        static VKAPI_ATTR void VKAPI_CALL CmdDraw(
            VkCommandBuffer commandBuffer,
            uint32_t vertexCount,
            uint32_t instanceCount,
            uint32_t firstVertex,
            uint32_t firstInstance );

        // vkCmdDrawIndirect
        static VKAPI_ATTR void VKAPI_CALL CmdDrawIndirect(
            VkCommandBuffer commandBuffer,
            VkBuffer buffer,
            VkDeviceSize offset,
            uint32_t drawCount,
            uint32_t stride );

        // vkCmdDrawIndexed
        static VKAPI_ATTR void VKAPI_CALL CmdDrawIndexed(
            VkCommandBuffer commandBuffer,
            uint32_t indexCount,
            uint32_t instanceCount,
            uint32_t firstIndex,
            int32_t vertexOffset,
            uint32_t firstInstance );

        // vkCmdDrawIndexedIndirect
        static VKAPI_ATTR void VKAPI_CALL CmdDrawIndexedIndirect(
            VkCommandBuffer commandBuffer,
            VkBuffer buffer,
            VkDeviceSize offset,
            uint32_t drawCount,
            uint32_t stride );

        // vkCmdDrawIndirectCount
        static VKAPI_ATTR void VKAPI_CALL CmdDrawIndirectCount(
            VkCommandBuffer commandBuffer,
            VkBuffer argsBuffer,
            VkDeviceSize argsOffset,
            VkBuffer countBuffer,
            VkDeviceSize countOffset,
            uint32_t maxDrawCount,
            uint32_t stride );

        // vkCmdDrawIndirectCount
        static VKAPI_ATTR void VKAPI_CALL CmdDrawIndexedIndirectCount(
            VkCommandBuffer commandBuffer,
            VkBuffer argsBuffer,
            VkDeviceSize argsOffset,
            VkBuffer countBuffer,
            VkDeviceSize countOffset,
            uint32_t maxDrawCount,
            uint32_t stride );

        // vkCmdDrawIndirectCountKHR
        static VKAPI_ATTR void VKAPI_CALL CmdDrawIndirectCountKHR(
            VkCommandBuffer commandBuffer,
            VkBuffer argsBuffer,
            VkDeviceSize argsOffset,
            VkBuffer countBuffer,
            VkDeviceSize countOffset,
            uint32_t maxDrawCount,
            uint32_t stride );

        // vkCmdDrawIndirectCountKHR
        static VKAPI_ATTR void VKAPI_CALL CmdDrawIndexedIndirectCountKHR(
            VkCommandBuffer commandBuffer,
            VkBuffer argsBuffer,
            VkDeviceSize argsOffset,
            VkBuffer countBuffer,
            VkDeviceSize countOffset,
            uint32_t maxDrawCount,
            uint32_t stride );

        // vkCmdDrawIndirectCountAMD
        static VKAPI_ATTR void VKAPI_CALL CmdDrawIndirectCountAMD(
            VkCommandBuffer commandBuffer,
            VkBuffer argsBuffer,
            VkDeviceSize argsOffset,
            VkBuffer countBuffer,
            VkDeviceSize countOffset,
            uint32_t maxDrawCount,
            uint32_t stride );

        // vkCmdDrawIndirectCountAMD
        static VKAPI_ATTR void VKAPI_CALL CmdDrawIndexedIndirectCountAMD(
            VkCommandBuffer commandBuffer,
            VkBuffer argsBuffer,
            VkDeviceSize argsOffset,
            VkBuffer countBuffer,
            VkDeviceSize countOffset,
            uint32_t maxDrawCount,
            uint32_t stride );

        // vkCmdDispatch
        static VKAPI_ATTR void VKAPI_CALL CmdDispatch(
            VkCommandBuffer commandBuffer,
            uint32_t x,
            uint32_t y,
            uint32_t z );

        // vkCmdDispatch
        static VKAPI_ATTR void VKAPI_CALL CmdDispatchIndirect(
            VkCommandBuffer commandBuffer,
            VkBuffer buffer,
            VkDeviceSize offset );

        // vkCmdCopyBuffer
        static VKAPI_ATTR void VKAPI_CALL CmdCopyBuffer(
            VkCommandBuffer commandBuffer,
            VkBuffer srcBuffer,
            VkBuffer dstBuffer,
            uint32_t regionCount,
            const VkBufferCopy* pRegions );

        // vkCmdCopyBufferToImage
        static VKAPI_ATTR void VKAPI_CALL CmdCopyBufferToImage(
            VkCommandBuffer commandBuffer,
            VkBuffer srcBuffer,
            VkImage dstImage,
            VkImageLayout dstImageLayout,
            uint32_t regionCount,
            const VkBufferImageCopy* pRegions );

        // vkCmdCopyImage
        static VKAPI_ATTR void VKAPI_CALL CmdCopyImage(
            VkCommandBuffer commandBuffer,
            VkImage srcImage,
            VkImageLayout srcImageLayout,
            VkImage dstImage,
            VkImageLayout dstImageLayout,
            uint32_t regionCount,
            const VkImageCopy* pRegions );

        // vkCmdCopyImageToBuffer
        static VKAPI_ATTR void VKAPI_CALL CmdCopyImageToBuffer(
            VkCommandBuffer commandBuffer,
            VkImage srcImage,
            VkImageLayout srcImageLayout,
            VkBuffer dstBuffer,
            uint32_t regionCount,
            const VkBufferImageCopy* pRegions );

        // vkCmdClearAttachments
        static VKAPI_ATTR void VKAPI_CALL CmdClearAttachments(
            VkCommandBuffer commandBuffer,
            uint32_t attachmentCount,
            const VkClearAttachment* pAttachments,
            uint32_t rectCount,
            const VkClearRect* pRects );

        // vkCmdClearColorImage
        static VKAPI_ATTR void VKAPI_CALL CmdClearColorImage(
            VkCommandBuffer commandBuffer,
            VkImage image,
            VkImageLayout imageLayout,
            const VkClearColorValue* pColor,
            uint32_t rangeCount,
            const VkImageSubresourceRange* pRanges );

        // vkCmdClearDepthStencilImage
        static VKAPI_ATTR void VKAPI_CALL CmdClearDepthStencilImage(
            VkCommandBuffer commandBuffer,
            VkImage image,
            VkImageLayout imageLayout,
            const VkClearDepthStencilValue* pDepthStencil,
            uint32_t rangeCount,
            const VkImageSubresourceRange* pRanges );

        // vkCmdResolveImage
        static VKAPI_ATTR void VKAPI_CALL CmdResolveImage(
            VkCommandBuffer commandBuffer,
            VkImage srcImage,
            VkImageLayout srcImageLayout,
            VkImage dstImage,
            VkImageLayout dstImageLayout,
            uint32_t regionCount,
            const VkImageResolve* pRegions );

        // vkCmdBlitImage
        static VKAPI_ATTR void VKAPI_CALL CmdBlitImage(
            VkCommandBuffer commandBuffer,
            VkImage srcImage,
            VkImageLayout srcImageLayout,
            VkImage dstImage,
            VkImageLayout dstImageLayout,
            uint32_t regionCount,
            const VkImageBlit* pRegions,
            VkFilter filter );

        // vkCmdFillBuffer
        static VKAPI_ATTR void VKAPI_CALL CmdFillBuffer(
            VkCommandBuffer commandBuffer,
            VkBuffer dstBuffer,
            VkDeviceSize dstOffset,
            VkDeviceSize size,
            uint32_t data );

        // vkCmdUpdateBuffer
        static VKAPI_ATTR void VKAPI_CALL CmdUpdateBuffer(
            VkCommandBuffer commandBuffer,
            VkBuffer dstBuffer,
            VkDeviceSize dstOffset,
            VkDeviceSize size,
            const void* pData );
    };
}
