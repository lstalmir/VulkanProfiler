#pragma once
#include "profiler_layer_objects/VkInstance_object.h"
#include "profiler_layer_functions/Dispatch.h"
#include <vulkan/vk_layer.h>

namespace Profiler
{
    /***********************************************************************************\

    Structure:
        VkInstance_Functions_Base

    Description:
        Base for all components of VkInstance.

        Functions OnInstanceCreate and OnInstanceDestroy should be called once for each
        instance created.

    \***********************************************************************************/
    struct VkInstance_Functions_Base
    {
        struct Dispatch
        {
            VkInstance_Object Instance;
        };

        static DispatchableMap<Dispatch> InstanceDispatch;

        // Invoked on vkCreateInstance
        static VkResult CreateInstanceBase(
            const VkInstanceCreateInfo* pCreateInfo,
            PFN_vkGetInstanceProcAddr pfnGetInstanceProcAddr,
            PFN_vkSetInstanceLoaderData pfnSetInstanceLoaderData,
            const VkAllocationCallbacks* pAllocator,
            VkInstance instance );

        // Invoked on vkDestroyDevice
        static void DestroyInstanceBase(
            VkInstance instance );
    };
}
