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
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Profiler requires command buffer to already be in recording state
        VkResult result = dd.Device.Callbacks.BeginCommandBuffer(
            commandBuffer, pBeginInfo );

        if( result == VK_SUCCESS )
        {
            // Prepare command buffer for the profiling
            profiledCommandBuffer.Begin( pBeginInfo );
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
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.End();

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
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreBeginRenderPass( pBeginInfo );

        // Begin the render pass
        dd.Device.Callbacks.CmdBeginRenderPass( commandBuffer, pBeginInfo, subpassContents );

        profiledCommandBuffer.PostBeginRenderPass();
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
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreEndRenderPass();

        // End the render pass
        dd.Device.Callbacks.CmdEndRenderPass( commandBuffer );

        profiledCommandBuffer.PostEndRenderPass();
    }

    /***********************************************************************************\

    Function:
        CmdNextSubpass

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdNextSubpass(
        VkCommandBuffer commandBuffer,
        VkSubpassContents contents )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.NextSubpass( contents );
        
        // Begin next subpass
        dd.Device.Callbacks.CmdNextSubpass( commandBuffer, contents );
    }

    /***********************************************************************************\

    Function:
        CmdBeginRenderPass2

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdBeginRenderPass2(
        VkCommandBuffer commandBuffer,
        const VkRenderPassBeginInfo* pBeginInfo,
        const VkSubpassBeginInfo* pSubpassBeginInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreBeginRenderPass( pBeginInfo );

        // Begin the render pass
        dd.Device.Callbacks.CmdBeginRenderPass2( commandBuffer, pBeginInfo, pSubpassBeginInfo );

        profiledCommandBuffer.PostBeginRenderPass();
    }

    /***********************************************************************************\

    Function:
        CmdEndRenderPass2

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdEndRenderPass2(
        VkCommandBuffer commandBuffer,
        const VkSubpassEndInfo* pSubpassEndInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreEndRenderPass();

        // End the render pass
        dd.Device.Callbacks.CmdEndRenderPass2( commandBuffer, pSubpassEndInfo );

        profiledCommandBuffer.PostEndRenderPass();
    }

    /***********************************************************************************\

    Function:
        CmdNextSubpass2

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdNextSubpass2(
        VkCommandBuffer commandBuffer,
        const VkSubpassBeginInfo* pSubpassBeginInfo,
        const VkSubpassEndInfo* pSubpassEndInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.NextSubpass( pSubpassBeginInfo->contents );

        // Begin next subpass
        dd.Device.Callbacks.CmdNextSubpass2( commandBuffer, pSubpassBeginInfo, pSubpassEndInfo );
    }

    /***********************************************************************************\

    Function:
        CmdBeginRenderPass2KHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdBeginRenderPass2KHR(
        VkCommandBuffer commandBuffer,
        const VkRenderPassBeginInfo* pBeginInfo,
        const VkSubpassBeginInfoKHR* pSubpassBeginInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreBeginRenderPass( pBeginInfo );

        // Begin the render pass
        dd.Device.Callbacks.CmdBeginRenderPass2KHR( commandBuffer, pBeginInfo, pSubpassBeginInfo );

        profiledCommandBuffer.PostBeginRenderPass();
    }

    /***********************************************************************************\

    Function:
        CmdEndRenderPass2KHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdEndRenderPass2KHR(
        VkCommandBuffer commandBuffer,
        const VkSubpassEndInfoKHR* pSubpassEndInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreEndRenderPass();

        // End the render pass
        dd.Device.Callbacks.CmdEndRenderPass2KHR( commandBuffer, pSubpassEndInfo );

        profiledCommandBuffer.PostEndRenderPass();
    }

    /***********************************************************************************\

    Function:
        CmdNextSubpass2KHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdNextSubpass2KHR(
        VkCommandBuffer commandBuffer,
        const VkSubpassBeginInfoKHR* pSubpassBeginInfo,
        const VkSubpassEndInfoKHR* pSubpassEndInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.NextSubpass( pSubpassBeginInfo->contents );

        // Begin next subpass
        dd.Device.Callbacks.CmdNextSubpass2KHR( commandBuffer, pSubpassBeginInfo, pSubpassEndInfo );
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
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );
        auto& profiledPipeline = dd.Profiler.GetPipeline( pipeline );

        // Bind the pipeline
        dd.Device.Callbacks.CmdBindPipeline( commandBuffer, bindPoint, pipeline );

        // Profile the pipeline time
        profiledCommandBuffer.BindPipeline( profiledPipeline );
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
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreDraw();

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDraw(
            commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance );

        profiledCommandBuffer.PostDraw();
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
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreDrawIndirect();

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndirect(
            commandBuffer, buffer, offset, drawCount, stride );

        profiledCommandBuffer.PostDrawIndirect();
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
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreDraw();

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndexed(
            commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );

        profiledCommandBuffer.PostDraw();
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
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreDrawIndirect();

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndexedIndirect(
            commandBuffer, buffer, offset, drawCount, stride );

        profiledCommandBuffer.PostDrawIndirect();
    }

    /***********************************************************************************\

    Function:
        CmdDrawIndirectCount

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdDrawIndirectCount(
        VkCommandBuffer commandBuffer,
        VkBuffer argsBuffer,
        VkDeviceSize argsOffset,
        VkBuffer countBuffer,
        VkDeviceSize countOffset,
        uint32_t maxDrawCount,
        uint32_t stride )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreDrawIndirect();

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndirectCount(
            commandBuffer, argsBuffer, argsOffset, countBuffer, countOffset, maxDrawCount, stride );

        profiledCommandBuffer.PostDrawIndirect();
    }

    /***********************************************************************************\

    Function:
        CmdDrawIndexedIndirectCount

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdDrawIndexedIndirectCount(
        VkCommandBuffer commandBuffer,
        VkBuffer argsBuffer,
        VkDeviceSize argsOffset,
        VkBuffer countBuffer,
        VkDeviceSize countOffset,
        uint32_t maxDrawCount,
        uint32_t stride )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreDrawIndirect();

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndexedIndirectCount(
            commandBuffer, argsBuffer, argsOffset, countBuffer, countOffset, maxDrawCount, stride );

        profiledCommandBuffer.PostDrawIndirect();
    }

    /***********************************************************************************\

    Function:
        CmdDrawIndirectCountKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdDrawIndirectCountKHR(
        VkCommandBuffer commandBuffer,
        VkBuffer argsBuffer,
        VkDeviceSize argsOffset,
        VkBuffer countBuffer,
        VkDeviceSize countOffset,
        uint32_t maxDrawCount,
        uint32_t stride )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreDrawIndirect();

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndirectCountKHR(
            commandBuffer, argsBuffer, argsOffset, countBuffer, countOffset, maxDrawCount, stride );

        profiledCommandBuffer.PostDrawIndirect();
    }

    /***********************************************************************************\

    Function:
        CmdDrawIndexedIndirectCountKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdDrawIndexedIndirectCountKHR(
        VkCommandBuffer commandBuffer,
        VkBuffer argsBuffer,
        VkDeviceSize argsOffset,
        VkBuffer countBuffer,
        VkDeviceSize countOffset,
        uint32_t maxDrawCount,
        uint32_t stride )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreDrawIndirect();

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndexedIndirectCountKHR(
            commandBuffer, argsBuffer, argsOffset, countBuffer, countOffset, maxDrawCount, stride );

        profiledCommandBuffer.PostDrawIndirect();
    }

    /***********************************************************************************\

    Function:
        CmdDrawIndirectCountAMD

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdDrawIndirectCountAMD(
        VkCommandBuffer commandBuffer,
        VkBuffer argsBuffer,
        VkDeviceSize argsOffset,
        VkBuffer countBuffer,
        VkDeviceSize countOffset,
        uint32_t maxDrawCount,
        uint32_t stride )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreDrawIndirect();

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndirectCountAMD(
            commandBuffer, argsBuffer, argsOffset, countBuffer, countOffset, maxDrawCount, stride );

        profiledCommandBuffer.PostDrawIndirect();
    }

    /***********************************************************************************\

    Function:
        CmdDrawIndexedIndirectCountAMD

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdDrawIndexedIndirectCountAMD(
        VkCommandBuffer commandBuffer,
        VkBuffer argsBuffer,
        VkDeviceSize argsOffset,
        VkBuffer countBuffer,
        VkDeviceSize countOffset,
        uint32_t maxDrawCount,
        uint32_t stride )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreDrawIndirect();

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndexedIndirectCountAMD(
            commandBuffer, argsBuffer, argsOffset, countBuffer, countOffset, maxDrawCount, stride );

        profiledCommandBuffer.PostDrawIndirect();
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
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreDispatch();

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDispatch( commandBuffer, x, y, z );

        profiledCommandBuffer.PostDispatch();
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
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreDispatchIndirect();

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDispatchIndirect( commandBuffer, buffer, offset );

        profiledCommandBuffer.PostDispatchIndirect();
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
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreCopy();

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyBuffer(
            commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions );

        profiledCommandBuffer.PostCopy();
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
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreCopy();

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyBufferToImage(
            commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions );

        profiledCommandBuffer.PostCopy();
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
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreCopy();

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyImage(
            commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions );

        profiledCommandBuffer.PostCopy();
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
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreCopy();

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyImageToBuffer(
            commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions );

        profiledCommandBuffer.PostCopy();
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
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreClear();

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdClearAttachments(
            commandBuffer, attachmentCount, pAttachments, rectCount, pRects );

        profiledCommandBuffer.PostClear( attachmentCount );
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
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreClear();

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdClearColorImage(
            commandBuffer, image, imageLayout, pColor, rangeCount, pRanges );

        profiledCommandBuffer.PostClear( 1 );
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
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreClear();

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdClearDepthStencilImage(
            commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges );

        profiledCommandBuffer.PostClear( 1 );
    }
}
