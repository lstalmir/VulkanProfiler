// Copyright (c) 2019-2021 Lukasz Stalmirski
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "VkCommandBuffer_functions.h"

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
        ResetCommandBuffer

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkCommandBuffer_Functions::ResetCommandBuffer(
        VkCommandBuffer commandBuffer,
        VkCommandBufferResetFlags flags )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.Reset( flags );

        return dd.Device.Callbacks.ResetCommandBuffer( commandBuffer, flags );
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

        profiledCommandBuffer.PreBeginRenderPass( pBeginInfo, subpassContents );

        // Begin the render pass
        dd.Device.Callbacks.CmdBeginRenderPass( commandBuffer, pBeginInfo, subpassContents );

        profiledCommandBuffer.PostBeginRenderPass( pBeginInfo, subpassContents );
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

        profiledCommandBuffer.PreBeginRenderPass( pBeginInfo, pSubpassBeginInfo->contents );

        // Begin the render pass
        dd.Device.Callbacks.CmdBeginRenderPass2( commandBuffer, pBeginInfo, pSubpassBeginInfo );

        profiledCommandBuffer.PostBeginRenderPass( pBeginInfo, pSubpassBeginInfo->contents );
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
        CmdExecuteCommands

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdExecuteCommands(
        VkCommandBuffer commandBuffer,
        uint32_t commandBufferCount,
        const VkCommandBuffer* pCommandBuffers )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Record secondary command buffers
        profiledCommandBuffer.ExecuteCommands( commandBufferCount, pCommandBuffers );

        // Execute commands
        dd.Device.Callbacks.CmdExecuteCommands( commandBuffer, commandBufferCount, pCommandBuffers );
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
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Record barrier statistics
        profiledCommandBuffer.PipelineBarrier(
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

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eDraw;
        drawcall.m_Payload.m_Draw.m_VertexCount = vertexCount;
        drawcall.m_Payload.m_Draw.m_InstanceCount = instanceCount;
        drawcall.m_Payload.m_Draw.m_FirstVertex = firstVertex;
        drawcall.m_Payload.m_Draw.m_FirstInstance = firstInstance;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDraw(
            commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance );

        profiledCommandBuffer.PostCommand( drawcall );
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

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eDrawIndirect;
        drawcall.m_Payload.m_DrawIndirect.m_Buffer = buffer;
        drawcall.m_Payload.m_DrawIndirect.m_Offset = offset;
        drawcall.m_Payload.m_DrawIndirect.m_DrawCount = drawCount;
        drawcall.m_Payload.m_DrawIndirect.m_Stride = stride;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndirect(
            commandBuffer, buffer, offset, drawCount, stride );

        profiledCommandBuffer.PostCommand( drawcall );
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

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eDrawIndexed;
        drawcall.m_Payload.m_DrawIndexed.m_IndexCount = indexCount;
        drawcall.m_Payload.m_DrawIndexed.m_InstanceCount = instanceCount;
        drawcall.m_Payload.m_DrawIndexed.m_FirstIndex = firstIndex;
        drawcall.m_Payload.m_DrawIndexed.m_VertexOffset = vertexOffset;
        drawcall.m_Payload.m_DrawIndexed.m_FirstInstance = firstInstance;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndexed(
            commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );

        profiledCommandBuffer.PostCommand( drawcall );
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

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eDrawIndexedIndirect;
        drawcall.m_Payload.m_DrawIndexedIndirect.m_Buffer = buffer;
        drawcall.m_Payload.m_DrawIndexedIndirect.m_Offset = offset;
        drawcall.m_Payload.m_DrawIndexedIndirect.m_DrawCount = drawCount;
        drawcall.m_Payload.m_DrawIndexedIndirect.m_Stride = stride;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndexedIndirect(
            commandBuffer, buffer, offset, drawCount, stride );

        profiledCommandBuffer.PostCommand( drawcall );
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

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eDrawIndirectCount;
        drawcall.m_Payload.m_DrawIndirectCount.m_Buffer = argsBuffer;
        drawcall.m_Payload.m_DrawIndirectCount.m_Offset = argsOffset;
        drawcall.m_Payload.m_DrawIndirectCount.m_CountBuffer = countBuffer;
        drawcall.m_Payload.m_DrawIndirectCount.m_CountOffset = countOffset;
        drawcall.m_Payload.m_DrawIndirectCount.m_MaxDrawCount = maxDrawCount;
        drawcall.m_Payload.m_DrawIndirectCount.m_Stride = stride;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndirectCount(
            commandBuffer, argsBuffer, argsOffset, countBuffer, countOffset, maxDrawCount, stride );

        profiledCommandBuffer.PostCommand( drawcall );
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
        
        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eDrawIndexedIndirectCount;
        drawcall.m_Payload.m_DrawIndirectCount.m_Buffer = argsBuffer;
        drawcall.m_Payload.m_DrawIndirectCount.m_Offset = argsOffset;
        drawcall.m_Payload.m_DrawIndirectCount.m_CountBuffer = countBuffer;
        drawcall.m_Payload.m_DrawIndirectCount.m_CountOffset = countOffset;
        drawcall.m_Payload.m_DrawIndirectCount.m_MaxDrawCount = maxDrawCount;
        drawcall.m_Payload.m_DrawIndirectCount.m_Stride = stride;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndexedIndirectCount(
            commandBuffer, argsBuffer, argsOffset, countBuffer, countOffset, maxDrawCount, stride );

        profiledCommandBuffer.PostCommand( drawcall );
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

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eDispatch;
        drawcall.m_Payload.m_Dispatch.m_GroupCountX = x;
        drawcall.m_Payload.m_Dispatch.m_GroupCountY = y;
        drawcall.m_Payload.m_Dispatch.m_GroupCountZ = z;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDispatch( commandBuffer, x, y, z );

        profiledCommandBuffer.PostCommand( drawcall );
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

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eDispatchIndirect;
        drawcall.m_Payload.m_DispatchIndirect.m_Buffer = buffer;
        drawcall.m_Payload.m_DispatchIndirect.m_Offset = offset;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDispatchIndirect( commandBuffer, buffer, offset );

        profiledCommandBuffer.PostCommand( drawcall );
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

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eCopyBuffer;
        drawcall.m_Payload.m_CopyBuffer.m_SrcBuffer = srcBuffer;
        drawcall.m_Payload.m_CopyBuffer.m_DstBuffer = dstBuffer;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyBuffer(
            commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions );

        profiledCommandBuffer.PostCommand( drawcall );
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

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eCopyBufferToImage;
        drawcall.m_Payload.m_CopyBufferToImage.m_SrcBuffer = srcBuffer;
        drawcall.m_Payload.m_CopyBufferToImage.m_DstImage = dstImage;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyBufferToImage(
            commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions );

        profiledCommandBuffer.PostCommand( drawcall );
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

        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eCopyImage;
        drawcall.m_Payload.m_CopyImage.m_SrcImage = srcImage;
        drawcall.m_Payload.m_CopyImage.m_DstImage = dstImage;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyImage(
            commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions );

        profiledCommandBuffer.PostCommand( drawcall );
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

        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eCopyImageToBuffer;
        drawcall.m_Payload.m_CopyImageToBuffer.m_SrcImage = srcImage;
        drawcall.m_Payload.m_CopyImageToBuffer.m_DstBuffer = dstBuffer;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyImageToBuffer(
            commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions );

        profiledCommandBuffer.PostCommand( drawcall );
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

        // Setup drawcall descriptor
        DeviceProfilerDrawcall clearDrawcall;
        clearDrawcall.m_Type = DeviceProfilerDrawcallType::eClearAttachments;
        clearDrawcall.m_Payload.m_ClearAttachments.m_Count = attachmentCount;

        profiledCommandBuffer.PreCommand( clearDrawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdClearAttachments(
            commandBuffer, attachmentCount, pAttachments, rectCount, pRects );

        profiledCommandBuffer.PostCommand( clearDrawcall );
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

        // Setup drawcall descriptor
        DeviceProfilerDrawcall clearDrawcall;
        clearDrawcall.m_Type = DeviceProfilerDrawcallType::eClearColorImage;
        clearDrawcall.m_Payload.m_ClearColorImage.m_Image = image;
        clearDrawcall.m_Payload.m_ClearColorImage.m_Value = *pColor;

        profiledCommandBuffer.PreCommand( clearDrawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdClearColorImage(
            commandBuffer, image, imageLayout, pColor, rangeCount, pRanges );

        profiledCommandBuffer.PostCommand( clearDrawcall );
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

        // Setup drawcall descriptor
        DeviceProfilerDrawcall clearDrawcall;
        clearDrawcall.m_Type = DeviceProfilerDrawcallType::eClearDepthStencilImage;
        clearDrawcall.m_Payload.m_ClearDepthStencilImage.m_Image = image;
        clearDrawcall.m_Payload.m_ClearDepthStencilImage.m_Value = *pDepthStencil;

        profiledCommandBuffer.PreCommand( clearDrawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdClearDepthStencilImage(
            commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges );

        profiledCommandBuffer.PostCommand( clearDrawcall );
    }

    /***********************************************************************************\

    Function:
        CmdResolveImage

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdResolveImage(
        VkCommandBuffer commandBuffer,
        VkImage srcImage,
        VkImageLayout srcImageLayout,
        VkImage dstImage,
        VkImageLayout dstImageLayout,
        uint32_t regionCount,
        const VkImageResolve* pRegions )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall resolveDrawcall;
        resolveDrawcall.m_Type = DeviceProfilerDrawcallType::eClearDepthStencilImage;
        resolveDrawcall.m_Payload.m_ResolveImage.m_SrcImage = srcImage;
        resolveDrawcall.m_Payload.m_ResolveImage.m_DstImage = dstImage;

        profiledCommandBuffer.PreCommand( resolveDrawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdResolveImage(
            commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions );

        profiledCommandBuffer.PostCommand( resolveDrawcall );
    }

    /***********************************************************************************\

    Function:
        CmdBlitImage

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdBlitImage(
        VkCommandBuffer commandBuffer,
        VkImage srcImage,
        VkImageLayout srcImageLayout,
        VkImage dstImage,
        VkImageLayout dstImageLayout,
        uint32_t regionCount,
        const VkImageBlit* pRegions,
        VkFilter filter )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall blitDrawcall;
        blitDrawcall.m_Type = DeviceProfilerDrawcallType::eBlitImage;
        blitDrawcall.m_Payload.m_BlitImage.m_SrcImage = srcImage;
        blitDrawcall.m_Payload.m_BlitImage.m_DstImage = dstImage;

        profiledCommandBuffer.PreCommand( blitDrawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdBlitImage(
            commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter );

        profiledCommandBuffer.PostCommand( blitDrawcall );
    }

    /***********************************************************************************\

    Function:
        CmdFillBuffer

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdFillBuffer(
        VkCommandBuffer commandBuffer,
        VkBuffer dstBuffer,
        VkDeviceSize dstOffset,
        VkDeviceSize size,
        uint32_t data )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall fillDrawcall;
        fillDrawcall.m_Type = DeviceProfilerDrawcallType::eFillBuffer;
        fillDrawcall.m_Payload.m_FillBuffer.m_Buffer = dstBuffer;
        fillDrawcall.m_Payload.m_FillBuffer.m_Offset = dstOffset;
        fillDrawcall.m_Payload.m_FillBuffer.m_Size = size;
        fillDrawcall.m_Payload.m_FillBuffer.m_Data = data;

        profiledCommandBuffer.PreCommand( fillDrawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdFillBuffer(
            commandBuffer, dstBuffer, dstOffset, size, data );

        profiledCommandBuffer.PostCommand( fillDrawcall );
    }

    /***********************************************************************************\

    Function:
        CmdUpdateBuffer

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdUpdateBuffer(
        VkCommandBuffer commandBuffer,
        VkBuffer dstBuffer,
        VkDeviceSize dstOffset,
        VkDeviceSize size,
        const void* pData )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall updateDrawcall;
        updateDrawcall.m_Type = DeviceProfilerDrawcallType::eUpdateBuffer;
        updateDrawcall.m_Payload.m_UpdateBuffer.m_Buffer = dstBuffer;
        updateDrawcall.m_Payload.m_UpdateBuffer.m_Offset = dstOffset;
        updateDrawcall.m_Payload.m_UpdateBuffer.m_Size = size;

        profiledCommandBuffer.PreCommand( updateDrawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdUpdateBuffer(
            commandBuffer, dstBuffer, dstOffset, size, pData );

        profiledCommandBuffer.PostCommand( updateDrawcall );
    }
}
