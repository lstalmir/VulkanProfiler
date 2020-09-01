#pragma once
#include "vk_dispatch_tables.h"
#include "VkInstance_object.h"
#include "VkQueue_object.h"
#include "VkSwapchainKhr_object.h"
#include <map>
#include <vector>

namespace Profiler
{
    enum class VkDevice_Vendor_ID
    {
        eUnknown = 0,
        eAMD = 0x1002,
        eARM = 0x13B3,
        eINTEL = 0x8086,
        eNV = 0x10DE,
        eQualcomm = 0x5143
    };

    struct VkDevice_debug_Object
    {
        std::map<uint64_t, std::string> ObjectNames;
    };

    struct VkDevice_Object
    {
        VkDevice Handle;

        VkInstance_Object* pInstance;
        VkPhysicalDevice PhysicalDevice;

        VkDevice_Vendor_ID VendorID;

        // Dispatch tables
        VkLayerDeviceDispatchTable Callbacks;
        PFN_vkSetDeviceLoaderData SetDeviceLoaderData;

        VkPhysicalDeviceProperties Properties;
        VkPhysicalDeviceMemoryProperties MemoryProperties;
        
        VkDevice_debug_Object Debug;

        std::unordered_map<VkQueue, VkQueue_Object> Queues;

        // Enabled extensions
        std::unordered_set<std::string> EnabledExtensions;

        // Swapchains created with this device
        std::unordered_map<VkSwapchainKHR, VkSwapchainKhr_Object> Swapchains;
    };
}
