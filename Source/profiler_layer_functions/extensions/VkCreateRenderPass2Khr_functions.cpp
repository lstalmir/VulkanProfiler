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

#include "VkCreateRenderPass2Khr_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        CreateRenderPass2KHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkCreateRenderPass2Khr_Functions::CreateRenderPass2KHR(
        VkDevice device,
        const VkRenderPassCreateInfo2KHR* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkRenderPass* pRenderPass )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Create the render pass
        VkResult result = dd.Device.Callbacks.CreateRenderPass2KHR(
            device, pCreateInfo, pAllocator, pRenderPass );

        if( result == VK_SUCCESS )
        {
            // Register new render pass
            dd.Profiler.CreateRenderPass( *pRenderPass, pCreateInfo );
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        CmdBeginRenderPass2KHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCreateRenderPass2Khr_Functions::CmdBeginRenderPass2KHR(
        VkCommandBuffer commandBuffer,
        const VkRenderPassBeginInfo* pBeginInfo,
        const VkSubpassBeginInfoKHR* pSubpassBeginInfo )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        profiledCommandBuffer.PreBeginRenderPass( pBeginInfo, pSubpassBeginInfo->contents );

        // Begin the render pass
        dd.Device.Callbacks.CmdBeginRenderPass2KHR( commandBuffer, pBeginInfo, pSubpassBeginInfo );

        profiledCommandBuffer.PostBeginRenderPass( pBeginInfo, pSubpassBeginInfo->contents );
    }

    /***********************************************************************************\

    Function:
        CmdEndRenderPass2KHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCreateRenderPass2Khr_Functions::CmdEndRenderPass2KHR(
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
    VKAPI_ATTR void VKAPI_CALL VkCreateRenderPass2Khr_Functions::CmdNextSubpass2KHR(
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
}
