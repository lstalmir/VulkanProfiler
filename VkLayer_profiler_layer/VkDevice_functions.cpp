#include "VkDevice_functions.h"
#include "VkInstance_functions.h"
#include "VkLayer_profiler_layer.generated.h"

namespace Profiler
{
    // Define the device dispatcher
    VkDispatch<VkDevice, VkDevice_Functions::DispatchTable> VkDevice_Functions::Dispatch;

    /***********************************************************************************\

    Function:
        DispatchTable

    Description:
        Constructor

    \***********************************************************************************/
    VkDevice_Functions::DispatchTable::DispatchTable( VkDevice device, PFN_vkGetDeviceProcAddr gpa )
        : pfnGetDeviceProcAddr( device, gpa, "vkGetDeviceProcAddr" )
        , pfnDestroyDevice( device, gpa, "vkDestroyDevice" )
    {
    }

    /***********************************************************************************\

    Function:
        GetProcAddr

    Description:
        Gets address of this layer's function implementation.

    \***********************************************************************************/
    PFN_vkVoidFunction VkDevice_Functions::GetProcAddr( const char* name )
    {
        // Intercepted functions
        GETPROCADDR( GetDeviceProcAddr );
        GETPROCADDR( CreateDevice );
        GETPROCADDR( DestroyDevice );
        GETPROCADDR( EnumerateDeviceLayerProperties );
        GETPROCADDR( EnumerateDeviceExtensionProperties );

        // Function not overloaded
        return nullptr;
    }

    /***********************************************************************************\

    Function:
        GetDeviceProcAddr

    Description:
        Gets pointer to the VkDevice function implementation.

    \***********************************************************************************/
    VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL VkDevice_Functions::GetDeviceProcAddr(
        VkDevice device,
        const char* name )
    {
        // Overloaded functions
        if( PFN_vkVoidFunction function = VkDevice_Functions::GetProcAddr( name ) )
            return function;

        // Get address from the next layer
        auto dispatchTable = Dispatch.GetDispatchTable( device );

        return dispatchTable.pfnGetDeviceProcAddr( device, name );
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
            pLayerCreateInfo =
                reinterpret_cast<const VkLayerDeviceCreateInfo*>(pLayerCreateInfo->pNext);
        }

        if( !pLayerCreateInfo )
        {
            // No loader device create info
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        PFN_vkGetInstanceProcAddr pfnGetInstanceProcAddr =
            pLayerCreateInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr;

        PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr =
            pLayerCreateInfo->u.pLayerInfo->pfnNextGetDeviceProcAddr;

        PFN_vkCreateDevice pfnCreateDevice =
            (PFN_vkCreateDevice)pfnGetInstanceProcAddr( VK_NULL_HANDLE, "vkCreateDevice" );

        // Invoke vkCreateDevice of next layer
        VkResult result = pfnCreateDevice( physicalDevice, pCreateInfo, pAllocator, pDevice );

        // Register callbacks to the next layer
        if( result == VK_SUCCESS )
        {
            Dispatch.CreateDispatchTable( *pDevice, pfnGetDeviceProcAddr );
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
        Dispatch.DestroyDispatchTable( device );
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

            auto instanceDispatchTable = VkInstance_Functions::Dispatch.GetDispatchTable( physicalDevice );

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
