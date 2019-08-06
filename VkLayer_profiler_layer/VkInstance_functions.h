#pragma once
#include "VkDispatch.h"
#include <vulkan/vk_layer.h>

namespace Profiler
{
    /***********************************************************************************\

    Structure:
        VkInstance_Functions

    Description:
        Set of VkInstance functions which are overloaded in this layer.

    \***********************************************************************************/
    struct VkInstance_Functions : Functions<VkInstance>
    {
        // Pointers to next layer's function implementations
        struct DispatchTable
        {
            PFN_vkGetInstanceProcAddr                pfnGetInstanceProcAddr;
            PFN_vkEnumerateDeviceExtensionProperties pfnEnumerateDeviceExtensionProperties;

            DispatchTable( VkInstance instance, PFN_vkGetInstanceProcAddr pfnGetInstanceProcAddr )
                : pfnGetInstanceProcAddr( GETINSTANCEPROCADDR( instance, vkGetInstanceProcAddr ) )
                , pfnEnumerateDeviceExtensionProperties( GETINSTANCEPROCADDR( instance, vkEnumerateDeviceExtensionProperties ) )
            {
            }
        };

        static VkDispatch<VkInstance, DispatchTable> Dispatch;

        // Get address of this layer's function implementation
        static PFN_vkVoidFunction GetInterceptedProcAddr( const char* pName );

        // Get address of function implementation
        static PFN_vkVoidFunction GetProcAddr( VkInstance instance, const char* pName );


        // vkGetInstanceProcAddr
        static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetInstanceProcAddr(
            VkInstance instance,
            const char* pName );

        // vkCreateInstance
        static VKAPI_ATTR VkResult VKAPI_CALL CreateInstance(
            const VkInstanceCreateInfo* pCreateInfo,
            VkAllocationCallbacks* pAllocator,
            VkInstance* pInstance );

        // vkDestroyInstance
        static VKAPI_ATTR void VKAPI_CALL DestroyInstance(
            VkInstance instance,
            VkAllocationCallbacks pAllocator );

        // vkEnumerateInstanceLayerProperties
        static VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceLayerProperties(
            uint32_t* pPropertyCount,
            VkLayerProperties* pLayerProperties );

        // vkEnumerateInstanceExtensionProperties
        static VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceExtensionProperties(
            const char* pLayerName,
            uint32_t* pPropertyCount,
            VkExtensionProperties* pExtensionProperties );
    };

}
