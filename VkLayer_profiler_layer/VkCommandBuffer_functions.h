#pragma once
#include "VkDevice_functions.h"
#include <vulkan/vk_layer.h>

namespace Profiler
{
    /***********************************************************************************\

    Structure:
        VkCommandBuffer_Functions

    Description:
        Set of VkCommandBuffer functions which are overloaded in this layer.

    \***********************************************************************************/
    struct VkCommandBuffer_Functions : VkDevice_Functions
    {
        // Pointers to next layer's function implementations
        struct DispatchTable
        {
            PFN_vkBeginCommandBuffer pfnBeginCommandBuffer;
            PFN_vkEndCommandBuffer   pfnEndCommandBuffer;
            PFN_vkCmdDraw            pfnCmdDraw;
            PFN_vkCmdDrawIndexed     pfnCmdDrawIndexed;

            DispatchTable( VkDevice device, PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr )
                : pfnBeginCommandBuffer( GETDEVICEPROCADDR( device, vkBeginCommandBuffer ) )
                , pfnEndCommandBuffer( GETDEVICEPROCADDR( device, vkEndCommandBuffer ) )
                , pfnCmdDraw( GETDEVICEPROCADDR( device, vkCmdDraw ) )
                , pfnCmdDrawIndexed( GETDEVICEPROCADDR( device, vkCmdDrawIndexed ) )
            {
            }
        };

        static VkDispatch<VkDevice, DispatchTable> Dispatch;

        // Get address of this layer's function implementation
        static PFN_vkVoidFunction GetInterceptedProcAddr( const char* name );

        // Get address of function implementation
        static PFN_vkVoidFunction GetProcAddr( VkDevice device, const char* pName );

        static void OnDeviceCreate( VkDevice device, PFN_vkGetDeviceProcAddr gpa );
        static void OnDeviceDestroy( VkDevice device );


        // vkBeginCommandBuffer
        static VKAPI_ATTR VkResult VKAPI_CALL BeginCommandBuffer(
            VkCommandBuffer commandBuffer,
            const VkCommandBufferBeginInfo* pBeginInfo );

        // vkEndCommandBuffer
        static VKAPI_ATTR VkResult VKAPI_CALL EndCommandBuffer(
            VkCommandBuffer commandBuffer );

        // vkCmdDraw
        static VKAPI_ATTR void VKAPI_CALL CmdDraw(
            VkCommandBuffer commandBuffer,
            uint32_t vertexCount,
            uint32_t instanceCount,
            uint32_t firstVertex,
            uint32_t firstInstance );

        // vkCmdDrawIndexed
        static VKAPI_ATTR void VKAPI_CALL CmdDrawIndexed(
            VkCommandBuffer commandBuffer,
            uint32_t indexCount,
            uint32_t instanceCount,
            uint32_t firstIndex,
            int32_t vertexOffset,
            uint32_t firstInstance );
    };
}
