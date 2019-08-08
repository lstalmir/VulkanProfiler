#pragma once
#include "profiler_layer_functions/VkDispatch.h"
#include <vulkan/vulkan.h>

namespace Profiler
{
    /***********************************************************************************\

    Structure:
        ProfilerCallbacks

    Description:
        VkDevice functions used by the Profiler instances.

    \***********************************************************************************/
    struct ProfilerCallbacks
    {
        PFN_vkCreateQueryPool           pfnCreateQueryPool;
        PFN_vkDestroyQueryPool          pfnDestroyQueryPool;
        PFN_vkCreateCommandPool         pfnCreateCommandPool;
        PFN_vkDestroyCommandPool        pfnDestroyCommandPool;
        PFN_vkAllocateCommandBuffers    pfnAllocateCommandBuffers;
        PFN_vkFreeCommandBuffers        pfnFreeCommandBuffers;
        PFN_vkBeginCommandBuffer        pfnBeginCommandBuffer;
        PFN_vkEndCommandBuffer          pfnEndCommandBuffer;
        PFN_vkCmdWriteTimestamp         pfnCmdWriteTimestamp;
        PFN_vkQueueSubmit               pfnQueueSubmit;

        ProfilerCallbacks()
        {
            memset( this, 0, sizeof( *this ) );
        }

        ProfilerCallbacks( VkDevice device, PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr )
            : pfnCreateQueryPool( GETDEVICEPROCADDR( device, vkCreateQueryPool ) )
            , pfnDestroyQueryPool( GETDEVICEPROCADDR( device, vkDestroyQueryPool ) )
            , pfnCreateCommandPool( GETDEVICEPROCADDR( device, vkCreateCommandPool ) )
            , pfnDestroyCommandPool( GETDEVICEPROCADDR( device, vkDestroyCommandPool ) )
            , pfnAllocateCommandBuffers( GETDEVICEPROCADDR( device, vkAllocateCommandBuffers ) )
            , pfnFreeCommandBuffers( GETDEVICEPROCADDR( device, vkFreeCommandBuffers ) )
            , pfnBeginCommandBuffer( GETDEVICEPROCADDR( device, vkBeginCommandBuffer ) )
            , pfnEndCommandBuffer( GETDEVICEPROCADDR( device, vkEndCommandBuffer ) )
            , pfnCmdWriteTimestamp( GETDEVICEPROCADDR( device, vkCmdWriteTimestamp ) )
            , pfnQueueSubmit( GETDEVICEPROCADDR( device, vkQueueSubmit ) )
        {
        }
    };
}
