#include "VkInstance_functions.h"
#include "VkDevice_functions.h"
#include "VkLayer_profiler_layer.generated.h"
#include "Helpers.h"

namespace Profiler
{
    DispatchableMap<VkInstance_Functions::Dispatch> VkInstance_Functions::InstanceDispatch;


    /***********************************************************************************\

    Function:
        GetInstanceProcAddr

    Description:
        Gets pointer to the VkInstance function implementation.

    \***********************************************************************************/
    VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL VkInstance_Functions::GetInstanceProcAddr(
        VkInstance instance,
        const char* pName )
    {
        // Debug utils extension is instance extension, but SetDebugUtilsObjectNameEXT is device function
        auto SetDebugUtilsObjectNameEXT = VkDevice_Functions::SetDebugUtilsObjectNameEXT;

        // VkInstance_Functions
        GETPROCADDR( GetInstanceProcAddr );
        GETPROCADDR( CreateInstance );
        GETPROCADDR( DestroyInstance );
        GETPROCADDR( CreateDevice );
        GETPROCADDR( EnumerateInstanceLayerProperties );
        GETPROCADDR( EnumerateInstanceExtensionProperties );
        GETPROCADDR( SetDebugUtilsObjectNameEXT );

        // Get address from the next layer
        auto& id = InstanceDispatch.Get( instance );

        return id.DispatchTable.GetInstanceProcAddr( instance, pName );
    }

    /***********************************************************************************\

    Function:
        CreateInstance

    Description:
        Creates VkInstance object and initializes dispatch table.

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkInstance_Functions::CreateInstance(
        const VkInstanceCreateInfo* pCreateInfo,
        VkAllocationCallbacks* pAllocator,
        VkInstance* pInstance )
    {
        const VkLayerInstanceCreateInfo* pLayerCreateInfo =
            reinterpret_cast<const VkLayerInstanceCreateInfo*>(pCreateInfo->pNext);

        // Step through the chain of pNext until we get to the link info
        while( (pLayerCreateInfo)
            && (pLayerCreateInfo->sType != VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO ||
                pLayerCreateInfo->function != VK_LAYER_LINK_INFO) )
        {
            pLayerCreateInfo = reinterpret_cast<const VkLayerInstanceCreateInfo*>(pLayerCreateInfo->pNext);
        }

        if( !pLayerCreateInfo )
        {
            // No loader instance create info
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        PFN_vkGetInstanceProcAddr pfnGetInstanceProcAddr =
            pLayerCreateInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr;

        PFN_vkCreateInstance pfnCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(
            pfnGetInstanceProcAddr( VK_NULL_HANDLE, "vkCreateInstance" ));

        // Invoke vkCreateInstance of next layer
        VkResult result = pfnCreateInstance( pCreateInfo, pAllocator, pInstance );

        // Register callbacks to the next layer
        if( result == VK_SUCCESS )
        {
            auto& id = InstanceDispatch.Create( *pInstance );

            // Get function addresses
            layer_init_instance_dispatch_table( *pInstance, &id.DispatchTable, pfnGetInstanceProcAddr );

            id.DispatchTable.CreateDevice = reinterpret_cast<PFN_vkCreateDevice>(
                pfnGetInstanceProcAddr( *pInstance, "vkCreateDevice" ));
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        DestroyInstance

    Description:
        Removes dispatch table associated with the VkInstance object.

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkInstance_Functions::DestroyInstance(
        VkInstance instance,
        VkAllocationCallbacks* pAllocator )
    {
        auto& id = InstanceDispatch.Get( instance );

        // Destroy the instance
        id.DispatchTable.DestroyInstance( instance, pAllocator );

        InstanceDispatch.Erase( instance );
    }

    /***********************************************************************************\
    
    Function:
        CreateDevice

    Description:
        

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkInstance_Functions::CreateDevice(
        VkPhysicalDevice physicalDevice,
        const VkDeviceCreateInfo* pCreateInfo,
        VkAllocationCallbacks* pAllocator,
        VkDevice* pDevice )
    {
        auto& id = InstanceDispatch.Get( physicalDevice );

        // Prefetch the device link info before creating the device to be sure we have vkDestroyDevice function available
        const auto* pLayerLinkInfo = GetLayerLinkInfo<VkLayerDeviceCreateInfo>( pCreateInfo );

        if( !pLayerLinkInfo )
        {
            // Link info not found, vkGetDeviceProcAddr unavailable
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        // Create the device
        VkResult result = id.DispatchTable.CreateDevice(
            physicalDevice, pCreateInfo, pAllocator, pDevice );

        // Initialize dispatch for the created device object
        result = VkDevice_Functions::OnDeviceCreate(
            physicalDevice, pCreateInfo, pAllocator, *pDevice );

        if( result != VK_SUCCESS )
        {
            // Initialization of the layer failed, destroy the device
            PFN_vkDestroyDevice pfnDestroyDevice = reinterpret_cast<PFN_vkDestroyDevice>(
                pLayerLinkInfo->u.pLayerInfo->pfnNextGetDeviceProcAddr( *pDevice, "vkDestroyDevice" ));

            pfnDestroyDevice( *pDevice, pAllocator );

            return result;
        }

        // Device created successfully
        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        EnumerateInstanceLayerProperties

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkInstance_Functions::EnumerateInstanceLayerProperties(
        uint32_t* pPropertyCount,
        VkLayerProperties* pLayerProperties )
    {
        if( pPropertyCount )
        {
            (*pPropertyCount) = 1;
        }

        if( pLayerProperties )
        {
            CopyString( pLayerProperties->layerName, VK_LAYER_profiler_name );
            CopyString( pLayerProperties->description, VK_LAYER_profiler_desc );

            pLayerProperties->implementationVersion = VK_LAYER_profiler_impl_ver;
            pLayerProperties->specVersion = VK_API_VERSION_1_0;
        }

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        EnumerateInstanceExtensionProperties

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkInstance_Functions::EnumerateInstanceExtensionProperties(
        const char* pLayerName,
        uint32_t* pPropertyCount,
        VkExtensionProperties* pExtensionProperties )
    {
        if( !pLayerName || strcmp( pLayerName, VK_LAYER_profiler_name ) != 0 )
        {
            return VK_ERROR_LAYER_NOT_PRESENT;
        }

        // Don't expose any extensions
        if( pPropertyCount )
        {
            (*pPropertyCount) = 0;
        }

        return VK_SUCCESS;
    }
}
