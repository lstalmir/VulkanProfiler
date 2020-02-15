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
        const VkAllocationCallbacks* pAllocator,
        VkDevice device )
    {
        // Get address of vkGetDeviceProcAddr function
        auto pLayerCreateInfo = GetLayerLinkInfo<VkLayerDeviceCreateInfo>( pCreateInfo );

        if( !pLayerCreateInfo )
        {
            // No loader device create info
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        // Create new dispatch table for the device
        auto& dd = DeviceDispatch.Create( device );

        layer_init_device_dispatch_table( device, &dd.DispatchTable,
            pLayerCreateInfo->u.pLayerInfo->pfnNextGetDeviceProcAddr );

        // Get instance dispatch table
        auto& id = VkInstance_Functions::InstanceDispatch.Get( physicalDevice );

        VkApplicationInfo applicationInfo;
        ClearStructure( &applicationInfo, VK_STRUCTURE_TYPE_APPLICATION_INFO );

        applicationInfo.apiVersion = id.ApiVersion;

        // Initialize the profiler object
        VkResult result = dd.Profiler.Initialize( &applicationInfo,
            physicalDevice, &id.DispatchTable, device, &dd.DispatchTable );

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
