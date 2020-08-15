#pragma once
#include "VkInstance_functions_base.h"
#include "VkPhysicalDevice_functions.h"
#include "VkSurfaceKhr_functions.h"
#include "VkWin32SurfaceKhr_functions.h"
#include "VkXlibSurfaceKhr_functions.h"
#include "VkWaylandSurfaceKhr_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Structure:
        VkInstance_Functions

    Description:
        Set of VkInstance functions which are overloaded in this layer.

    \***********************************************************************************/
    struct VkInstance_Functions
        : VkPhysicalDevice_Functions
        , VkSurfaceKhr_Functions
        , VkWin32SurfaceKhr_Functions
        , VkXlibSurfaceKhr_Functions
        , VkWaylandSurfaceKhr_Functions
    {
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
