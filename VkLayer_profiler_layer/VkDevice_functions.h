#pragma once
#include "VkDispatch.h"
#include <vulkan/vk_layer.h>

namespace Profiler
{
    /***********************************************************************************\

    Structure:
        VkInstance_Functions

    Description:
        Set of VkDevice functions which are overloaded in this layer.

    \***********************************************************************************/
    struct VkDevice_Functions
    {
        struct DispatchTable
        {
            VkFunction< PFN_vkGetDeviceProcAddr     > pfnGetDeviceProcAddr;
            VkFunction< PFN_vkDestroyDevice         > pfnDestroyDevice;

            DispatchTable( VkDevice device, PFN_vkGetDeviceProcAddr gpa );
        };

        static VkDispatch<VkDevice, DispatchTable> Dispatch;

        // Get address of this layer's function implementation
        static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetProcAddr(
            const char* name );

        // vkGetDeviceProcAddr
        static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetDeviceProcAddr(
            VkDevice instance,
            const char* name );

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
