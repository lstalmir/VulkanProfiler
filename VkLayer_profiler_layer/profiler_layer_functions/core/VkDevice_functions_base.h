#pragma once
#include "profiler/profiler.h"
#include "profiler_overlay/profiler_overlay.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include "profiler_layer_functions/Dispatch.h"
#include <vulkan/vk_layer.h>
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
            DeviceProfiler Profiler;

            ProfilerOverlayOutput Overlay;
        };

        static DispatchableMap<Dispatch> DeviceDispatch;

        // Invoked on vkCreateDevice
        static VkResult OnDeviceCreate(
            VkPhysicalDevice physicalDevice,
            const VkDeviceCreateInfo* pCreateInfo,
            PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr,
            PFN_vkSetDeviceLoaderData pfnSetDeviceLoaderData,
            const VkAllocationCallbacks* pAllocator,
            VkDevice device );

        // Invoked on vkDestroyDevice
        static void OnDeviceDestroy( 
            VkDevice device );
    };
}
