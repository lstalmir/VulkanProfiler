#include "VkDevice_functions_base.h"
#include "VkInstance_functions.h"
#include "Helpers.h"
#include "profiler/profiler_helpers.h"

namespace Profiler
{
    // Initialize profilers dispatch tables map
    DispatchableMap<VkDevice_Functions_Base::Dispatch> VkDevice_Functions_Base::DeviceDispatch;


    /***********************************************************************************\

    Function:
        OnDeviceCreate

    Description:
        Initializes profiler for the device.

    \***********************************************************************************/
    VkResult VkDevice_Functions_Base::OnDeviceCreate(
        VkPhysicalDevice physicalDevice,
        const VkDeviceCreateInfo* pCreateInfo,
        PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr,
        const VkAllocationCallbacks* pAllocator,
        VkDevice device )
    {
        // Get instance dispatch table
        auto& id = VkInstance_Functions::InstanceDispatch.Get( physicalDevice );

        // Create new dispatch table for the device
        auto& dd = DeviceDispatch.Create( device );

        layer_init_device_dispatch_table( device, &dd.Device.Callbacks,
            pfnGetDeviceProcAddr );

        dd.Device.Handle = device;
        dd.Device.pInstance = &id.Instance;
        dd.Device.PhysicalDevice = physicalDevice;

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

                dd.Device.Queues.push_back( queueObject );
            }
        }

        // Initialize the profiler object
        VkResult result = dd.Profiler.Initialize( &dd.Device );

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
        OnDeviceDestroy

    Description:
        Destroys profiler for the device.

    \***********************************************************************************/
    void VkDevice_Functions_Base::OnDeviceDestroy( VkDevice device )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Destroy the profiler instance
        dd.Profiler.Destroy();

        DeviceDispatch.Erase( device );
    }
}
