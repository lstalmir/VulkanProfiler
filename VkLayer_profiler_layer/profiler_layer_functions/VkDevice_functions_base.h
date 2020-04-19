#pragma once
#include "Dispatch.h"
#include "profiler/profiler.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include <vk_layer.h>
#include <vk_layer_dispatch_table.h>

namespace Profiler
{
    /***********************************************************************************\

    Structure:
        VkDevice_ProfiledFunctionsBase

    Description:
        Base for all components of VkDevice containing functions, which will be profiled.
        Manages Profiler object for the device.

        Functions OnDeviceCreate and OnDeviceDestroy should be called once for each
        device created.

    \***********************************************************************************/
    struct VkDevice_Functions_Base
    {
        struct Dispatch
        {
            VkDevice_Object Device;
            Profiler Profiler;
        };

        static DispatchableMap<Dispatch> DeviceDispatch;

        // Invoked on vkCreateDevice
        static VkResult OnDeviceCreate(
            VkPhysicalDevice physicalDevice,
            const VkDeviceCreateInfo* pCreateInfo,
            PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr,
            const VkAllocationCallbacks* pAllocator,
            VkDevice device );

        // Invoked on vkDestroyDevice
        static void OnDeviceDestroy( 
            VkDevice device );
    };
}
