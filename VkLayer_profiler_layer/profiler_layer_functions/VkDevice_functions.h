#pragma once
#include "VkDispatch.h"
#include "profiler_layer/profiler.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include <vulkan/vk_layer.h>

namespace Profiler
{
    /***********************************************************************************\

    Structure:
        VkDevice_Functions

    Description:
        Set of VkDevice functions which are overloaded in this layer.

    \***********************************************************************************/
    struct VkDevice_Functions : Functions<VkDevice>
    {
        // Pointers to next layer's function implementations
        struct DispatchTable
        {
            PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr;
            PFN_vkQueuePresentKHR   pfnQueuePresentKHR;

            DispatchTable( VkDevice device, PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr )
                : pfnGetDeviceProcAddr( GETDEVICEPROCADDR( device, vkGetDeviceProcAddr ) )
                , pfnQueuePresentKHR( GETDEVICEPROCADDR( device, vkQueuePresentKHR ) )
            {
            }
        };

        static VkDispatch<VkDevice, DispatchTable> DeviceFunctions;
        static VkDispatchableMap<VkDevice, Profiler*> DeviceProfilers;
        static VkDispatchableMap<VkDevice, VkDevice_Object> DeviceObjects;

        // Get address of this layer's function implementation
        static PFN_vkVoidFunction GetInterceptedProcAddr( const char* pName );

        // Get address of function implementation
        static PFN_vkVoidFunction GetProcAddr( VkDevice device, const char* pName );


        // vkGetDeviceProcAddr
        static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetDeviceProcAddr(
            VkDevice device,
            const char* pName );

        // vkCreateDevice
        static VKAPI_ATTR VkResult VKAPI_CALL CreateDevice(
            VkPhysicalDevice physicalDevice,
            const VkDeviceCreateInfo* pCreateInfo,
            VkAllocationCallbacks* pAllocator,
            VkDevice* pDevice );

        // vkDestroyDevice
        static VKAPI_ATTR void VKAPI_CALL DestroyDevice(
            VkDevice device,
            VkAllocationCallbacks pAllocator );

        // vkEnumerateDeviceLayerProperties
        static VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceLayerProperties(
            uint32_t* pPropertyCount,
            VkLayerProperties* pLayerProperties );

        // vkEnumerateDeviceExtensionProperties
        static VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceExtensionProperties(
            VkPhysicalDevice physicalDevice,
            const char* pLayerName,
            uint32_t* pPropertyCount,
            VkExtensionProperties* pProperties );
    };
}
