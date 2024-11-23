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

#pragma once
#include "VkDevice_functions_base.h"

namespace Profiler
{
    struct VkCopyCommands2Khr_Functions : VkDevice_Functions_Base
    {
        // vkCmdBlitImage2KHR
        static VKAPI_ATTR void VKAPI_CALL CmdBlitImage2KHR(
            VkCommandBuffer commandBuffer,
            const VkBlitImageInfo2KHR* pBlitImageInfo );

        // vkCmdCopyBuffer2KHR
        static VKAPI_ATTR void VKAPI_CALL CmdCopyBuffer2KHR(
            VkCommandBuffer commandBuffer,
            const VkCopyBufferInfo2KHR* pCopyBufferInfo );

        // vkCmdCopyBufferToImage2KHR
        static VKAPI_ATTR void VKAPI_CALL CmdCopyBufferToImage2KHR(
            VkCommandBuffer commandBuffer,
            const VkCopyBufferToImageInfo2KHR* pCopyBufferToImageInfo );

        // vkCmdCopyImage2KHR
        static VKAPI_ATTR void VKAPI_CALL CmdCopyImage2KHR(
            VkCommandBuffer commandBuffer,
            const VkCopyImageInfo2KHR* pCopyImageInfo );

        // vkCmdCopyImageToBuffer2KHR
        static VKAPI_ATTR void VKAPI_CALL CmdCopyImageToBuffer2KHR(
            VkCommandBuffer commandBuffer,
            const VkCopyImageToBufferInfo2KHR* pCopyImageToBufferInfo );

        // vkCmdResolveImage2KHR
        static VKAPI_ATTR void VKAPI_CALL CmdResolveImage2KHR(
            VkCommandBuffer commandBuffer,
            const VkResolveImageInfo2KHR* pResolveImageInfo );
    };
}
