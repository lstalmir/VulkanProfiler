#include "VkDevice_functions.h"
#include "VkInstance_functions.h"
#include "VkCommandBuffer_functions.h"
#include "VkQueue_functions.h"
#include "VkLayer_profiler_layer.generated.h"

namespace Profiler
{
    // Define VkDevice dispatch tables map
    VkDispatch<VkDevice, VkDevice_Functions::DispatchTable> VkDevice_Functions::DeviceFunctions;

    // Define VkDevice profilers map
    VkDispatchableMap<VkDevice, Profiler> VkDevice_Functions::DeviceProfilers;

    /***********************************************************************************\

    Function:
        GetProcAddr

    Description:
        Gets address of this layer's function implementation.

    \***********************************************************************************/
    PFN_vkVoidFunction VkDevice_Functions::GetInterceptedProcAddr( const char* pName )
    {
        // Intercepted functions
        GETPROCADDR( GetDeviceProcAddr );
        GETPROCADDR( CreateDevice );
        GETPROCADDR( DestroyDevice );
        GETPROCADDR( EnumerateDeviceLayerProperties );
        GETPROCADDR( EnumerateDeviceExtensionProperties );

        // CommandBuffer functions
        if( PFN_vkVoidFunction function = VkCommandBuffer_Functions::GetInterceptedProcAddr( pName ) )
        {
            return function;
        }

        // Queue functions
        if( PFN_vkVoidFunction function = VkQueue_Functions::GetInterceptedProcAddr( pName ) )
        {
            return function;
        }

        // Function not overloaded
        return nullptr;
    }

    /***********************************************************************************\

    Function:
        GetProcAddr

    Description:
        Gets pointer to the VkDevice function implementation.

    \***********************************************************************************/
    PFN_vkVoidFunction VkDevice_Functions::GetProcAddr( VkDevice device, const char* pName )
    {
        return GetDeviceProcAddr( device, pName );
    }


    /***********************************************************************************\

    Function:
        GetDeviceProcAddr

    Description:
        Gets pointer to the VkDevice function implementation.

    \***********************************************************************************/
    VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL VkDevice_Functions::GetDeviceProcAddr(
        VkDevice device,
        const char* pName )
    {
        // Overloaded functions
        if( PFN_vkVoidFunction function = GetInterceptedProcAddr( pName ) )
        {
            return function;
        }

        // Get address from the next layer
        auto dispatchTable = DeviceFunctions.GetDispatchTable( device );

        return dispatchTable.pfnGetDeviceProcAddr( device, pName );
    }

    /***********************************************************************************\

    Function:
        CreateDevice

    Description:
        Creates VkDevice object and initializes dispatch table.

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDevice_Functions::CreateDevice(
        VkPhysicalDevice physicalDevice,
        const VkDeviceCreateInfo* pCreateInfo,
        VkAllocationCallbacks* pAllocator,
        VkDevice* pDevice )
    {
        const VkLayerDeviceCreateInfo* pLayerCreateInfo =
            reinterpret_cast<const VkLayerDeviceCreateInfo*>(pCreateInfo->pNext);

        // Step through the chain of pNext until we get to the link info
        while( (pLayerCreateInfo)
            && (pLayerCreateInfo->sType != VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO ||
                pLayerCreateInfo->function != VK_LAYER_LINK_INFO) )
        {
            pLayerCreateInfo = reinterpret_cast<const VkLayerDeviceCreateInfo*>(pLayerCreateInfo->pNext);
        }

        if( !pLayerCreateInfo )
        {
            // No loader device create info
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        // Get pointers to next layer's vkGetInstanceProcAddr and vkGetDeviceProcAddr
        PFN_vkGetInstanceProcAddr pfnGetInstanceProcAddr =
            pLayerCreateInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr;
        PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr =
            pLayerCreateInfo->u.pLayerInfo->pfnNextGetDeviceProcAddr;

        PFN_vkCreateDevice pfnCreateDevice = GETINSTANCEPROCADDR( VK_NULL_HANDLE, vkCreateDevice );

        // Invoke vkCreateDevice of next layer
        VkResult result = pfnCreateDevice( physicalDevice, pCreateInfo, pAllocator, pDevice );

        // Create profiler instance for the device
        if( result == VK_SUCCESS )
        {
            ProfilerCallbacks deviceProfilerCallbacks( *pDevice, pfnGetDeviceProcAddr );

            Profiler deviceProfiler;
            result = deviceProfiler.Initialize( *pDevice, deviceProfilerCallbacks );

            if( result != VK_SUCCESS )
            {
                PFN_vkDestroyDevice pfnDestroyDevice = GETDEVICEPROCADDR( *pDevice, vkDestroyDevice );

                // Destroy the device
                pfnDestroyDevice( *pDevice, pAllocator );

                return result;
            }

            std::scoped_lock lk( DeviceProfilers );

            auto it = DeviceProfilers.try_emplace( *pDevice, deviceProfiler );
            if( !it.second )
            {
                // TODO error, should have created new element
            }
        }

        // Register callbacks to the next layer
        if( result == VK_SUCCESS )
        {
            DeviceFunctions.CreateDispatchTable( *pDevice, pfnGetDeviceProcAddr );

            // Initialize VkCommandBuffer functions for the device
            VkCommandBuffer_Functions::OnDeviceCreate( *pDevice, pfnGetDeviceProcAddr );

            // Initialize VkQueue functions for this device
            VkQueue_Functions::OnDeviceCreate( *pDevice, pfnGetDeviceProcAddr );
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        DestroyDevice

    Description:
        Removes dispatch table associated with the VkDevice object.

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDevice_Functions::DestroyDevice(
        VkDevice device,
        VkAllocationCallbacks pAllocator )
    {
        DeviceFunctions.DestroyDispatchTable( device );
        DeviceProfilers.erase( device );

        // Cleanup VkCommandBuffer function callbacks
        VkCommandBuffer_Functions::OnDeviceDestroy( device );

        // Cleanup VkQueue function callbacks
        VkQueue_Functions::OnDeviceDestroy( device );
    }

    /***********************************************************************************\

    Function:
        EnumerateDeviceLayerProperties

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDevice_Functions::EnumerateDeviceLayerProperties(
        uint32_t* pPropertyCount,
        VkLayerProperties* pLayerProperties )
    {
        return VkInstance_Functions::EnumerateInstanceLayerProperties( pPropertyCount, pLayerProperties );
    }

    /***********************************************************************************\

    Function:
        EnumerateDeviceExtensionProperties

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDevice_Functions::EnumerateDeviceExtensionProperties(
        VkPhysicalDevice physicalDevice,
        const char* pLayerName,
        uint32_t* pPropertyCount,
        VkExtensionProperties* pProperties )
    {
        // Pass through any queries that aren't to us
        if( !pLayerName || strcmp( pLayerName, VK_LAYER_profiler_name ) != 0 )
        {
            if( physicalDevice == VK_NULL_HANDLE )
                return VK_SUCCESS;

            // EnumerateDeviceExtensionProperties is actually VkInstance (VkPhysicalDevice) function.
            // Get dispatch table associated with the VkPhysicalDevice and invoke next layer's
            // vkEnumerateDeviceExtensionProperties implementation.
            auto instanceDispatchTable = VkInstance_Functions::InstanceFunctions.GetDispatchTable( physicalDevice );

            return instanceDispatchTable.pfnEnumerateDeviceExtensionProperties(
                physicalDevice, pLayerName, pPropertyCount, pProperties );
        }

        // Don't expose any extensions
        if( pPropertyCount )
        {
            (*pPropertyCount) = 0;
        }

        return VK_SUCCESS;
    }
}
