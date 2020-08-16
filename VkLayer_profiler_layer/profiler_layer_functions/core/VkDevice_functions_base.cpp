#include "VkDevice_functions_base.h"
#include "VkInstance_functions.h"
#include "profiler_layer_functions/Helpers.h"
#include "profiler/profiler_helpers.h"

namespace Profiler
{
    // Initialize profilers dispatch tables map
    DispatchableMap<VkDevice_Functions_Base::Dispatch> VkDevice_Functions_Base::DeviceDispatch;


    /***********************************************************************************\

    Function:
        CreateDeviceBase

    Description:
        Initializes profiler for the device.

    \***********************************************************************************/
    VkResult VkDevice_Functions_Base::CreateDeviceBase(
        VkPhysicalDevice physicalDevice,
        const VkDeviceCreateInfo* pCreateInfo,
        PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr,
        PFN_vkSetDeviceLoaderData pfnSetDeviceLoaderData,
        const VkAllocationCallbacks* pAllocator,
        VkDevice device )
    {
        // Get instance dispatch table
        auto& id = VkInstance_Functions::InstanceDispatch.Get( physicalDevice );

        // Create new dispatch table for the device
        auto& dd = DeviceDispatch.Create( device );

        init_layer_device_dispatch_table( device, pfnGetDeviceProcAddr, dd.Device.Callbacks );

        dd.Device.SetDeviceLoaderData = pfnSetDeviceLoaderData;

        dd.Device.Handle = device;
        dd.Device.pInstance = &id.Instance;
        dd.Device.PhysicalDevice = physicalDevice;

        // Get device properties
        id.Instance.Callbacks.GetPhysicalDeviceProperties(
            physicalDevice, &dd.Device.Properties );

        id.Instance.Callbacks.GetPhysicalDeviceMemoryProperties(
            physicalDevice, &dd.Device.MemoryProperties );

        dd.Device.VendorID = VkDevice_Vendor_ID( dd.Device.Properties.vendorID );

        // Enumerate queue families
        uint32_t queueFamilyPropertyCount = 0;
        id.Instance.Callbacks.GetPhysicalDeviceQueueFamilyProperties(
            physicalDevice, &queueFamilyPropertyCount, nullptr );

        std::vector<VkQueueFamilyProperties> queueFamilyProperties( queueFamilyPropertyCount );
        id.Instance.Callbacks.GetPhysicalDeviceQueueFamilyProperties(
            physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data() );

        // Create wrapper for each device queue
        for( uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; ++i )
        {
            const VkDeviceQueueCreateInfo& queueCreateInfo =
                pCreateInfo->pQueueCreateInfos[ i ];

            const VkQueueFamilyProperties& queueProperties =
                queueFamilyProperties[ queueCreateInfo.queueFamilyIndex ];

            for( uint32_t queueIndex = 0; queueIndex < queueCreateInfo.queueCount; ++queueIndex )
            {
                VkQueue_Object queueObject;

                // Get queue handle
                dd.Device.Callbacks.GetDeviceQueue(
                    device, queueCreateInfo.queueFamilyIndex, queueIndex, &queueObject.Handle );

                queueObject.Flags = queueProperties.queueFlags;
                queueObject.Family = queueCreateInfo.queueFamilyIndex;
                queueObject.Index = queueIndex;

                dd.Device.Queues.emplace( queueObject.Handle, queueObject );
            }
        }

        // Check if profiler create info was provided
        const VkProfilerCreateInfoEXT* pProfilerCreateInfo = nullptr;

        for( const auto& it : PNextIterator( pCreateInfo->pNext ) )
        {
            if( it.sType == VK_STRUCTURE_TYPE_PROFILER_CREATE_INFO_EXT )
            {
                pProfilerCreateInfo = reinterpret_cast<const VkProfilerCreateInfoEXT*>(&it);
                break;
            }
        }

        // Initialize the profiler object
        VkResult result = dd.Profiler.Initialize( &dd.Device, pProfilerCreateInfo );

        if( result != VK_SUCCESS )
        {
            // Profiler initialization failed
            DeviceDispatch.Erase( device );

            return result;
        }

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        DestroyDeviceBase

    Description:
        Destroys profiler for the device.

    \***********************************************************************************/
    void VkDevice_Functions_Base::DestroyDeviceBase( VkDevice device )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Destroy the profiler instance
        dd.Profiler.Destroy();
        // Destroy the overlay
        dd.Overlay.Destroy();

        DeviceDispatch.Erase( device );
    }
}
