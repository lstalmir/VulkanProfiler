#pragma once
#include "VkDispatch.h"
#include "VkDevice_functions.h"
#include <vulkan/vk_layer.h>

namespace Profiler
{
    /***********************************************************************************\

    Structure:
        VkQueue_Functions

    Description:
        Set of VkQueue functions which are overloaded in this layer.

    \***********************************************************************************/
    struct VkQueue_Functions : VkDevice_Functions
    {
        // Pointers to next layer's function implementations
        struct DispatchTable
        {
            PFN_vkQueuePresentKHR   pfnQueuePresentKHR;
            PFN_vkQueueSubmit       pfnQueueSubmit;

            DispatchTable( VkDevice device, PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr )
                : pfnQueuePresentKHR( GETDEVICEPROCADDR( device, vkQueuePresentKHR ) )
                , pfnQueueSubmit( GETDEVICEPROCADDR( device, vkQueueSubmit ) )
            {
            }
        };

        static VkDispatch<VkDevice, DispatchTable> QueueFunctions;

        // Get address of this layer's function implementation
        static PFN_vkVoidFunction GetInterceptedProcAddr( const char* name );

        // Get address of function implementation
        static PFN_vkVoidFunction GetProcAddr( VkDevice device, const char* pName );

        static void OnDeviceCreate( VkDevice device, PFN_vkGetDeviceProcAddr gpa );
        static void OnDeviceDestroy( VkDevice device );


        // vkQueuePresentKHR
        static VKAPI_ATTR VkResult VKAPI_CALL QueuePresentKHR(
            VkQueue queue,
            const VkPresentInfoKHR* pPresentInfo );
    };
}
