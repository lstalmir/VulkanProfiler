#pragma once
#include "Dispatch.h"
#include "VkCommandBuffer_functions.h"
#include "VkQueue_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Structure:
        VkDevice_Functions

    Description:
        Set of VkDevice functions which are overloaded in this layer.

    \***********************************************************************************/
    struct VkDevice_Functions
        : VkCommandBuffer_Functions
        , VkQueue_Functions
    {
        // vkGetDeviceProcAddr
        static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetDeviceProcAddr(
            VkDevice device,
            const char* pName );

        // vkDestroyDevice
        static VKAPI_ATTR void VKAPI_CALL DestroyDevice(
            VkDevice device,
            VkAllocationCallbacks* pAllocator );

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

        // vkAllocateMemory
        static VKAPI_ATTR VkResult VKAPI_CALL AllocateMemory(
            VkDevice device,
            const VkMemoryAllocateInfo* pAllocateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDeviceMemory* pMemory );

        // vkFreeMemory
        static VKAPI_ATTR void VKAPI_CALL FreeMemory(
            VkDevice device,
            VkDeviceMemory memory,
            const VkAllocationCallbacks* pAllocator );
    };
}
