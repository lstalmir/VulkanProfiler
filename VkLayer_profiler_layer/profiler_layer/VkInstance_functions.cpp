#include "VkInstance_functions.h"
#include "VkInstance_dispatch.h"

#define DISPATCH_FUNCTION( NAME ) \
    if( strcmp( name, "vk" #NAME ) == 0 ) \
    { \
        return reinterpret_cast<PFN_vkVoidFunction>(NAME); \
    }

namespace Profiler
{
    /***********************************************************************************\

    Function:
        GetInstanceProcAddr

    Description:
        Gets pointer to the VkInstance function implementation.

    \***********************************************************************************/
    VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL VkInstance_Functions::GetInstanceProcAddr(
        VkInstance instance,
        const char* name )
    {
        // Overloaded functions
        DISPATCH_FUNCTION( GetInstanceProcAddr );
        DISPATCH_FUNCTION( CreateInstance );
        DISPATCH_FUNCTION( DestroyInstance );

        // Get address from the next layer
        auto dispatchTable = InstanceDispatch.GetInstanceDispatchTable( instance );

        return dispatchTable.pfnGetInstanceProcAddr( instance, name );
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
            pLayerCreateInfo =
                reinterpret_cast<const VkLayerInstanceCreateInfo*>(pLayerCreateInfo->pNext);
        }

        if( !pLayerCreateInfo )
        {
            // No loader instance create info
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        PFN_vkGetInstanceProcAddr pfnGetInstanceProcAddr =
            pLayerCreateInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr;

        PFN_vkCreateInstance pfnCreateInstance =
            (PFN_vkCreateInstance)pfnGetInstanceProcAddr( VK_NULL_HANDLE, "vkCreateInstance" );

        // Invoke vkCreateInstance of next layer
        VkResult result = pfnCreateInstance( pCreateInfo, pAllocator, pInstance );

        // Register callbacks to the next layer
        if( result == VK_SUCCESS )
        {
            InstanceDispatch.CreateInstanceDispatchTable( *pInstance, pfnGetInstanceProcAddr );
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
        VkAllocationCallbacks pAllocator )
    {
        InstanceDispatch.DestroyInstanceDispatchTable( instance );
    }
}
