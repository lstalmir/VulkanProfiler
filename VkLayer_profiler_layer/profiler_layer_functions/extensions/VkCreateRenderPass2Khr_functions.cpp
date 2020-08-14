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
