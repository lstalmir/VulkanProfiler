#include "VkCommandBuffer_functions.h"
#include "Helpers.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        BeginCommandBuffer

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkCommandBuffer_Functions::BeginCommandBuffer(
        VkCommandBuffer commandBuffer,
        const VkCommandBufferBeginInfo* pBeginInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );

        // Profiler requires command buffer to already be in recording state
        VkResult result = dd.Device.Callbacks.BeginCommandBuffer(
            commandBuffer, pBeginInfo );

        if( result == VK_SUCCESS )
        {
            // Prepare command buffer for the profiling
            dd.Profiler.BeginCommandBuffer( commandBuffer, pBeginInfo );
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        EndCommandBuffer

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkCommandBuffer_Functions::EndCommandBuffer(
        VkCommandBuffer commandBuffer )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );

        dd.Profiler.EndCommandBuffer( commandBuffer );

        return dd.Device.Callbacks.EndCommandBuffer( commandBuffer );
    }

    /***********************************************************************************\

    Function:
        CmdBeginRenderPass

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdBeginRenderPass(
        VkCommandBuffer commandBuffer,
        const VkRenderPassBeginInfo* pBeginInfo,
        VkSubpassContents subpassContents )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );

        // Profile the render pass time
        dd.Profiler.BeginRenderPass( commandBuffer, pBeginInfo );

        // Begin the render pass
        dd.Device.Callbacks.CmdBeginRenderPass( commandBuffer, pBeginInfo, subpassContents );
    }

    /***********************************************************************************\

    Function:
        CmdEndRenderPass

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdEndRenderPass(
        VkCommandBuffer commandBuffer )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );

        // End the render pass
        dd.Device.Callbacks.CmdEndRenderPass( commandBuffer );

        // Profile the render pass time
        dd.Profiler.EndRenderPass( commandBuffer );
    }

    /***********************************************************************************\

    Function:
        CmdBindPipeline

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdBindPipeline(
        VkCommandBuffer commandBuffer,
        VkPipelineBindPoint bindPoint,
        VkPipeline pipeline )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );

        // Bind the pipeline
        dd.Device.Callbacks.CmdBindPipeline( commandBuffer, bindPoint, pipeline );

        // Profile the pipeline time
        dd.Profiler.BindPipeline( commandBuffer, pipeline );
    }

    /***********************************************************************************\

    Function:
        CmdPipelineBarrier

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdPipelineBarrier(
        VkCommandBuffer commandBuffer,
        VkPipelineStageFlags srcStageMask,
        VkPipelineStageFlags dstStageMask,
        VkDependencyFlags dependencyFlags,
        uint32_t memoryBarrierCount,
        const VkMemoryBarrier* pMemoryBarriers,
        uint32_t bufferMemoryBarrierCount,
        const VkBufferMemoryBarrier* pBufferMemoryBarriers,
        uint32_t imageMemoryBarrierCount,
        const VkImageMemoryBarrier* pImageMemoryBarriers )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );

        // Record barrier statistics
        dd.Profiler.OnPipelineBarrier( commandBuffer,
            memoryBarrierCount, pMemoryBarriers,
            bufferMemoryBarrierCount, pBufferMemoryBarriers,
            imageMemoryBarrierCount, pImageMemoryBarriers );

        // Insert the barrier
        dd.Device.Callbacks.CmdPipelineBarrier( commandBuffer,
            srcStageMask, dstStageMask, dependencyFlags,
            memoryBarrierCount, pMemoryBarriers,
            bufferMemoryBarrierCount, pBufferMemoryBarriers,
            imageMemoryBarrierCount, pImageMemoryBarriers );
    }

    /***********************************************************************************\

    Function:
        CmdDraw

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdDraw(
        VkCommandBuffer commandBuffer,
        uint32_t vertexCount,
        uint32_t instanceCount,
        uint32_t firstVertex,
        uint32_t firstInstance )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );

        dd.Profiler.PreDraw( commandBuffer );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDraw(
            commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance );

        dd.Profiler.PostDraw( commandBuffer );
    }

    /***********************************************************************************\

    Function:
        CmdDrawIndirect

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdDrawIndirect(
        VkCommandBuffer commandBuffer,
        VkBuffer buffer,
        VkDeviceSize offset,
        uint32_t drawCount,
        uint32_t stride )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );

        dd.Profiler.PreDrawIndirect( commandBuffer );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndirect(
            commandBuffer, buffer, offset, drawCount, stride );

        dd.Profiler.PostDrawIndirect( commandBuffer );
    }

    /***********************************************************************************\

    Function:
        CmdDrawIndexed

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdDrawIndexed(
        VkCommandBuffer commandBuffer,
        uint32_t indexCount,
        uint32_t instanceCount,
        uint32_t firstIndex,
        int32_t vertexOffset,
        uint32_t firstInstance )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );

        dd.Profiler.PreDraw( commandBuffer );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndexed(
            commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );

        dd.Profiler.PostDraw( commandBuffer );
    }

    /***********************************************************************************\

    Function:
        CmdDrawIndexedIndirect

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdDrawIndexedIndirect(
        VkCommandBuffer commandBuffer,
        VkBuffer buffer,
        VkDeviceSize offset,
        uint32_t drawCount,
        uint32_t stride )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );

        dd.Profiler.PreDrawIndirect( commandBuffer );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndexedIndirect(
            commandBuffer, buffer, offset, drawCount, stride );

        dd.Profiler.PostDrawIndirect( commandBuffer );
    }

    /***********************************************************************************\

    Function:
        CmdDispatch

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdDispatch(
        VkCommandBuffer commandBuffer,
        uint32_t x,
        uint32_t y,
        uint32_t z )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );

        dd.Profiler.PreDispatch( commandBuffer );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDispatch( commandBuffer, x, y, z );

        dd.Profiler.PostDispatch( commandBuffer );
    }

    /***********************************************************************************\

    Function:
        CmdDispatchIndirect

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdDispatchIndirect(
        VkCommandBuffer commandBuffer,
        VkBuffer buffer,
        VkDeviceSize offset )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );

        dd.Profiler.PreDispatchIndirect( commandBuffer );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDispatchIndirect( commandBuffer, buffer, offset );

        dd.Profiler.PostDispatchIndirect( commandBuffer );
    }

    /***********************************************************************************\

    Function:
        CmdCopyBuffer

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdCopyBuffer(
        VkCommandBuffer commandBuffer,
        VkBuffer srcBuffer,
        VkBuffer dstBuffer,
        uint32_t regionCount,
        const VkBufferCopy* pRegions )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );

        dd.Profiler.PreCopy( commandBuffer );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyBuffer(
            commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions );

        dd.Profiler.PostCopy( commandBuffer );
    }

    /***********************************************************************************\

    Function:
        CmdCopyBufferToImage

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdCopyBufferToImage(
        VkCommandBuffer commandBuffer,
        VkBuffer srcBuffer,
        VkImage dstImage,
        VkImageLayout dstImageLayout,
        uint32_t regionCount,
        const VkBufferImageCopy* pRegions )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );

        dd.Profiler.PreCopy( commandBuffer );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyBufferToImage(
            commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions );

        dd.Profiler.PostCopy( commandBuffer );
    }

    /***********************************************************************************\

    Function:
        CmdCopyImage

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdCopyImage(
        VkCommandBuffer commandBuffer,
        VkImage srcImage,
        VkImageLayout srcImageLayout,
        VkImage dstImage,
        VkImageLayout dstImageLayout,
        uint32_t regionCount,
        const VkImageCopy* pRegions )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );

        dd.Profiler.PreCopy( commandBuffer );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyImage(
            commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions );

        dd.Profiler.PostCopy( commandBuffer );
    }

    /***********************************************************************************\

    Function:
        CmdCopyImageToBuffer

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdCopyImageToBuffer(
        VkCommandBuffer commandBuffer,
        VkImage srcImage,
        VkImageLayout srcImageLayout,
        VkBuffer dstBuffer,
        uint32_t regionCount,
        const VkBufferImageCopy* pRegions )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );

        dd.Profiler.PreCopy( commandBuffer );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyImageToBuffer(
            commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions );

        dd.Profiler.PostCopy( commandBuffer );
    }

    /***********************************************************************************\

    Function:
        CmdClearAttachments

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdClearAttachments(
        VkCommandBuffer commandBuffer,
        uint32_t attachmentCount,
        const VkClearAttachment* pAttachments,
        uint32_t rectCount,
        const VkClearRect* pRects )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );

        dd.Profiler.PreClear( commandBuffer );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdClearAttachments(
            commandBuffer, attachmentCount, pAttachments, rectCount, pRects );

        dd.Profiler.PostClear( commandBuffer, attachmentCount );
    }

    /***********************************************************************************\

    Function:
        CmdClearColorImage

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdClearColorImage(
        VkCommandBuffer commandBuffer,
        VkImage image,
        VkImageLayout imageLayout,
        const VkClearColorValue* pColor,
        uint32_t rangeCount,
        const VkImageSubresourceRange* pRanges )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );

        dd.Profiler.PreClear( commandBuffer );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdClearColorImage(
            commandBuffer, image, imageLayout, pColor, rangeCount, pRanges );

        dd.Profiler.PostClear( commandBuffer, 1 );
    }

    /***********************************************************************************\

    Function:
        CmdClearDepthStencilImage

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdClearDepthStencilImage(
        VkCommandBuffer commandBuffer,
        VkImage image,
        VkImageLayout imageLayout,
        const VkClearDepthStencilValue* pDepthStencil,
        uint32_t rangeCount,
        const VkImageSubresourceRange* pRanges )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );

        dd.Profiler.PreClear( commandBuffer );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdClearDepthStencilImage(
            commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges );

        dd.Profiler.PostClear( commandBuffer, 1 );
    }
}
