#pragma once
#include "Dispatch.h"
#include "profiler_layer_objects/VkInstance_object.h"
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
            VkInstance_Object Instance;
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

        #ifdef VK_USE_PLATFORM_WIN32_KHR
        // vkCreateWin32SurfaceKHR
        static VKAPI_ATTR VkResult VKAPI_CALL CreateWin32SurfaceKHR(
            VkInstance instance,
            const VkWin32SurfaceCreateInfoKHR* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkSurfaceKHR* pSurface );
        #endif

        #ifdef VK_USE_PLATFORM_WAYLAND_KHR

        #endif
        
        #ifdef VK_USE_PLATFORM_XLIB_KHR
        // vkCreateXlibSurfaceKHR
        static VKAPI_ATTR VkResult VKAPI_CALL CreateXlibSurfaceKHR(
            VkInstance instance,
            const VkXlibSurfaceCreateInfoKHR* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkSurfaceKHR* pSurface );
        #endif

        // vkDestroySurfaceKHR
        static VKAPI_ATTR void VKAPI_CALL DestroySurfaceKHR(
            VkInstance instance,
            VkSurfaceKHR surface,
            const VkAllocationCallbacks* pAllocator );
    };

}
