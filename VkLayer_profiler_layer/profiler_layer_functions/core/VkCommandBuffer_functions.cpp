// Copyright (c) 2020 Lukasz Stalmirski
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

        auto pCommand = BeginRenderPassCommand::Create( pBeginInfo, subpassContents );

        profiledCommandBuffer.PreCommand( pCommand );

        // Begin the render pass
        dd.Device.Callbacks.CmdBeginRenderPass( commandBuffer, pBeginInfo, subpassContents );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = EndRenderPassCommand::Create();

        profiledCommandBuffer.PreCommand( pCommand );

        // End the render pass
        dd.Device.Callbacks.CmdEndRenderPass( commandBuffer );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = NextSubpassCommand::Create( contents );

        profiledCommandBuffer.PreCommand( pCommand );

        // Begin next subpass
        dd.Device.Callbacks.CmdNextSubpass( commandBuffer, contents );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = BeginRenderPassCommand::Create( pBeginInfo, pSubpassBeginInfo->contents );

        profiledCommandBuffer.PreCommand( pCommand );

        // Begin the render pass
        dd.Device.Callbacks.CmdBeginRenderPass2( commandBuffer, pBeginInfo, pSubpassBeginInfo );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = EndRenderPassCommand::Create();

        profiledCommandBuffer.PreCommand( pCommand );

        // End the render pass
        dd.Device.Callbacks.CmdEndRenderPass2( commandBuffer, pSubpassEndInfo );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = NextSubpassCommand::Create( pSubpassBeginInfo->contents );

        profiledCommandBuffer.PreCommand( pCommand );

        // Begin next subpass
        dd.Device.Callbacks.CmdNextSubpass2( commandBuffer, pSubpassBeginInfo, pSubpassEndInfo );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = BindPipelineCommand::Create( bindPoint, pipeline );

        profiledCommandBuffer.PreCommand( pCommand );

        // Bind the pipeline
        dd.Device.Callbacks.CmdBindPipeline( commandBuffer, bindPoint, pipeline );

        // Profile the pipeline time
        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = ExecuteCommandsCommand::Create( commandBufferCount, pCommandBuffers );

        profiledCommandBuffer.PreCommand( pCommand );

        // Execute commands
        dd.Device.Callbacks.CmdExecuteCommands( commandBuffer, commandBufferCount, pCommandBuffers );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = PipelineBarrierCommand::Create(
            srcStageMask,
            dstStageMask,
            dependencyFlags,
            memoryBarrierCount,
            pMemoryBarriers,
            bufferMemoryBarrierCount,
            pBufferMemoryBarriers,
            imageMemoryBarrierCount,
            pImageMemoryBarriers );

        profiledCommandBuffer.PreCommand( pCommand );

        // Insert the barrier
        dd.Device.Callbacks.CmdPipelineBarrier( commandBuffer,
            srcStageMask, dstStageMask, dependencyFlags,
            memoryBarrierCount, pMemoryBarriers,
            bufferMemoryBarrierCount, pBufferMemoryBarriers,
            imageMemoryBarrierCount, pImageMemoryBarriers );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = DrawCommand::Create(
            vertexCount,
            instanceCount,
            firstVertex,
            firstInstance );

        profiledCommandBuffer.PreCommand( pCommand );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDraw(
            commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = DrawIndirectCommand::Create(
            buffer,
            offset,
            drawCount,
            stride );

        profiledCommandBuffer.PreCommand( pCommand );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndirect(
            commandBuffer, buffer, offset, drawCount, stride );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = DrawIndexedCommand::Create(
            indexCount,
            instanceCount,
            firstIndex,
            vertexOffset,
            firstInstance );

        profiledCommandBuffer.PreCommand( pCommand );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndexed(
            commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = DrawIndexedIndirectCommand::Create(
            buffer,
            offset,
            drawCount,
            stride );

        profiledCommandBuffer.PreCommand( pCommand );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndexedIndirect(
            commandBuffer, buffer, offset, drawCount, stride );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = DrawIndirectCountCommand::Create(
            argsBuffer,
            argsOffset,
            countBuffer,
            countOffset,
            maxDrawCount,
            stride );

        profiledCommandBuffer.PreCommand( pCommand );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndirectCount(
            commandBuffer, argsBuffer, argsOffset, countBuffer, countOffset, maxDrawCount, stride );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = DrawIndexedIndirectCountCommand::Create(
            argsBuffer,
            argsOffset,
            countBuffer,
            countOffset,
            maxDrawCount,
            stride );

        profiledCommandBuffer.PreCommand( pCommand );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndexedIndirectCount(
            commandBuffer, argsBuffer, argsOffset, countBuffer, countOffset, maxDrawCount, stride );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = DispatchCommand::Create( x, y, z );

        profiledCommandBuffer.PreCommand( pCommand );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDispatch( commandBuffer, x, y, z );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = DispatchIndirectCommand::Create( buffer, offset );

        profiledCommandBuffer.PreCommand( pCommand );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDispatchIndirect( commandBuffer, buffer, offset );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = CopyBufferCommand::Create( srcBuffer, dstBuffer );

        profiledCommandBuffer.PreCommand( pCommand );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyBuffer(
            commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = CopyBufferToImageCommand::Create( srcBuffer, dstImage );

        profiledCommandBuffer.PreCommand( pCommand );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyBufferToImage(
            commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = CopyImageCommand::Create( srcImage, dstImage );

        profiledCommandBuffer.PreCommand( pCommand );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyImage(
            commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = CopyImageToBufferCommand::Create( srcImage, dstBuffer );

        profiledCommandBuffer.PreCommand( pCommand );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyImageToBuffer(
            commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = ClearAttachmentsCommand::Create( attachmentCount, pAttachments, rectCount, pRects );

        profiledCommandBuffer.PreCommand( pCommand );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdClearAttachments(
            commandBuffer, attachmentCount, pAttachments, rectCount, pRects );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = ClearColorImageCommand::Create( image, imageLayout, pColor, rangeCount, pRanges );

        profiledCommandBuffer.PreCommand( pCommand );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdClearColorImage(
            commandBuffer, image, imageLayout, pColor, rangeCount, pRanges );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = ClearDepthStencilImageCommand::Create( image, imageLayout, pDepthStencil, rangeCount, pRanges );

        profiledCommandBuffer.PreCommand( pCommand );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdClearDepthStencilImage(
            commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = ResolveImageCommand::Create( srcImage, dstImage );

        profiledCommandBuffer.PreCommand( pCommand );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdResolveImage(
            commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = BlitImageCommand::Create( srcImage, dstImage );

        profiledCommandBuffer.PreCommand( pCommand );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdBlitImage(
            commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = FillBufferCommand::Create( dstBuffer, dstOffset, size, data );

        profiledCommandBuffer.PreCommand( pCommand );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdFillBuffer(
            commandBuffer, dstBuffer, dstOffset, size, data );

        profiledCommandBuffer.PostCommand( pCommand );
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

        auto pCommand = UpdateBufferCommand::Create( dstBuffer, dstOffset, size, pData );

        profiledCommandBuffer.PreCommand( pCommand );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdUpdateBuffer(
            commandBuffer, dstBuffer, dstOffset, size, pData );

        profiledCommandBuffer.PostCommand( pCommand );
    }
}
