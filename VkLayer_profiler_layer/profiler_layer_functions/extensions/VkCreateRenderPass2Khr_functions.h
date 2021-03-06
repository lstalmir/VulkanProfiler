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
