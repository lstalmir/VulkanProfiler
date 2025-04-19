// Copyright (c) 2024 Lukasz Stalmirski
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

#include "VkCopyCommands2Khr_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        CmdBlitImage2KHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCopyCommands2Khr_Functions::CmdBlitImage2KHR(
        VkCommandBuffer commandBuffer,
        const VkBlitImageInfo2KHR* pBlitImageInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        TipGuard tip( dd.Device.TIP, __func__ );

        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eBlitImage;
        drawcall.m_Payload.m_BlitImage.m_SrcImage = pBlitImageInfo->srcImage;
        drawcall.m_Payload.m_BlitImage.m_DstImage = pBlitImageInfo->dstImage;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdBlitImage2KHR(
            commandBuffer,
            pBlitImageInfo );

        profiledCommandBuffer.PostCommand( drawcall );
    }

    /***********************************************************************************\

    Function:
        CmdCopyBuffer2KHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCopyCommands2Khr_Functions::CmdCopyBuffer2KHR(
        VkCommandBuffer commandBuffer,
        const VkCopyBufferInfo2KHR* pCopyBufferInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        TipGuard tip( dd.Device.TIP, __func__ );

        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eCopyBuffer;
        drawcall.m_Payload.m_CopyBuffer.m_SrcBuffer = pCopyBufferInfo->srcBuffer;
        drawcall.m_Payload.m_CopyBuffer.m_DstBuffer = pCopyBufferInfo->dstBuffer;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyBuffer2KHR(
            commandBuffer,
            pCopyBufferInfo );

        profiledCommandBuffer.PostCommand( drawcall );
    }

    /***********************************************************************************\

    Function:
        CmdCopyBufferToImage2KHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCopyCommands2Khr_Functions::CmdCopyBufferToImage2KHR(
        VkCommandBuffer commandBuffer,
        const VkCopyBufferToImageInfo2KHR* pCopyBufferToImageInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        TipGuard tip( dd.Device.TIP, __func__ );

        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eCopyBufferToImage;
        drawcall.m_Payload.m_CopyBufferToImage.m_SrcBuffer = pCopyBufferToImageInfo->srcBuffer;
        drawcall.m_Payload.m_CopyBufferToImage.m_DstImage = pCopyBufferToImageInfo->dstImage;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyBufferToImage2KHR(
            commandBuffer,
            pCopyBufferToImageInfo );

        profiledCommandBuffer.PostCommand( drawcall );
    }

    /***********************************************************************************\

    Function:
        CmdCopyImage2KHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCopyCommands2Khr_Functions::CmdCopyImage2KHR(
        VkCommandBuffer commandBuffer,
        const VkCopyImageInfo2KHR* pCopyImageInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        TipGuard tip( dd.Device.TIP, __func__ );

        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eCopyImage;
        drawcall.m_Payload.m_CopyImage.m_SrcImage = pCopyImageInfo->srcImage;
        drawcall.m_Payload.m_CopyImage.m_DstImage = pCopyImageInfo->dstImage;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyImage2KHR(
            commandBuffer,
            pCopyImageInfo );

        profiledCommandBuffer.PostCommand( drawcall );
    }

    /***********************************************************************************\

    Function:
        CmdCopyImageToBuffer2KHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCopyCommands2Khr_Functions::CmdCopyImageToBuffer2KHR(
        VkCommandBuffer commandBuffer,
        const VkCopyImageToBufferInfo2KHR* pCopyImageToBufferInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        TipGuard tip( dd.Device.TIP, __func__ );

        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eCopyImageToBuffer;
        drawcall.m_Payload.m_CopyImageToBuffer.m_SrcImage = pCopyImageToBufferInfo->srcImage;
        drawcall.m_Payload.m_CopyImageToBuffer.m_DstBuffer = pCopyImageToBufferInfo->dstBuffer;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdCopyImageToBuffer2KHR(
            commandBuffer,
            pCopyImageToBufferInfo );

        profiledCommandBuffer.PostCommand( drawcall );
    }

    /***********************************************************************************\

    Function:
        CmdResolveImage2KHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCopyCommands2Khr_Functions::CmdResolveImage2KHR(
        VkCommandBuffer commandBuffer,
        const VkResolveImageInfo2KHR* pResolveImageInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        TipGuard tip( dd.Device.TIP, __func__ );

        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eResolveImage;
        drawcall.m_Payload.m_ResolveImage.m_SrcImage = pResolveImageInfo->srcImage;
        drawcall.m_Payload.m_ResolveImage.m_DstImage = pResolveImageInfo->dstImage;

        profiledCommandBuffer.PreCommand( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdResolveImage2KHR(
            commandBuffer,
            pResolveImageInfo );

        profiledCommandBuffer.PostCommand( drawcall );
    }
}
