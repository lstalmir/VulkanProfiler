#include "VkInstance_functions.h"
#include "VkDevice_functions.h"
#include "VkLayer_profiler_layer.generated.h"

namespace
{
    static inline void CopyString( char* dst, size_t dstSize, const char* src )
    {
#   ifdef WIN32
        strcpy_s( dst, dstSize, src );
#   else
        strcpy( dst, src );
#   endif
    }

    template<size_t dstSize>
    static inline void CopyString( char( &dst )[dstSize], const char* src )
    {
        return CopyString( dst, dstSize, src );
    }
}

namespace Profiler
{
    // Define the instance dispatcher
    VkDispatch<VkInstance, VkInstance_Functions::DispatchTable> VkInstance_Functions::Dispatch;

    /***********************************************************************************\

    Function:
        GetInterceptedProcAddr

    Description:
        Gets address of this layer's function implementation.

    \***********************************************************************************/
    PFN_vkVoidFunction VkInstance_Functions::GetInterceptedProcAddr( const char* pName )
    {
        // Intercepted functions
        GETPROCADDR( GetInstanceProcAddr );
        GETPROCADDR( CreateInstance );
        GETPROCADDR( DestroyInstance );
        GETPROCADDR( EnumerateInstanceLayerProperties );
        GETPROCADDR( EnumerateInstanceExtensionProperties );

        if( PFN_vkVoidFunction function = VkDevice_Functions::GetInterceptedProcAddr( pName ) )
            return function;

        // Function not overloaded
        return nullptr;
    }

    /***********************************************************************************\

    Function:
        GetProcAddr

    Description:
        Gets address of function implementation.

    \***********************************************************************************/
    PFN_vkVoidFunction VkInstance_Functions::GetProcAddr( VkInstance instance, const char* pName )
    {
        return GetInstanceProcAddr( instance, pName );
    }

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
        // Overloaded functions
        if( PFN_vkVoidFunction function = GetInterceptedProcAddr( pName ) )
            return function;

        // Get address from the next layer
        auto dispatchTable = Dispatch.GetDispatchTable( instance );

        return dispatchTable.pfnGetInstanceProcAddr( instance, pName );
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

        PFN_vkCreateInstance pfnCreateInstance =
            (PFN_vkCreateInstance)pfnGetInstanceProcAddr( VK_NULL_HANDLE, "vkCreateInstance" );

        // Invoke vkCreateInstance of next layer
        VkResult result = pfnCreateInstance( pCreateInfo, pAllocator, pInstance );

        // Register callbacks to the next layer
        if( result == VK_SUCCESS )
        {
            Dispatch.CreateDispatchTable( *pInstance, pfnGetInstanceProcAddr );
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
        Dispatch.DestroyDispatchTable( instance );
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
