#pragma once
#include "Dispatch.h"
#include <vk_layer.h>
#include <vk_layer_dispatch_table.h>

namespace Profiler
{
    /***********************************************************************************\

    Structure:
        VkInstance_Functions

    Description:
        Set of VkInstance functions which are overloaded in this layer.

    \***********************************************************************************/
    struct VkInstance_Functions
    {
        struct Dispatch
        {
            VkLayerInstanceDispatchTable DispatchTable;
            uint32_t ApiVersion;
        };

        static DispatchableMap<Dispatch> InstanceDispatch;

        // vkGetInstanceProcAddr
        static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetInstanceProcAddr(
            VkInstance instance,
            const char* pName );

        // vkCreateInstance
        static VKAPI_ATTR VkResult VKAPI_CALL CreateInstance(
            const VkInstanceCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkInstance* pInstance );

        // vkDestroyInstance
        static VKAPI_ATTR void VKAPI_CALL DestroyInstance(
            VkInstance instance,
            const VkAllocationCallbacks* pAllocator );

        // vkCreateDevice
        static VKAPI_ATTR VkResult VKAPI_CALL CreateDevice(
            VkPhysicalDevice physicalDevice,
            const VkDeviceCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDevice* pDevice );

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
