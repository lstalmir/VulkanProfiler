#include "VkLayer_profiler_layer.h"
#include "profiler_layer/instance_functions.h"

////////////////////////////////////////////////////////////////////////////////////////
VK_LAYER_EXPORT VkResult VKAPI_CALL vkCreateInstance(
    const VkInstanceCreateInfo*         pCreateInfo,
    const VkAllocationCallbacks*        pAllocator,
    VkInstance*                         pInstance )
{
    const VkLayerInstanceCreateInfo* pLayerCreateInfo =
        reinterpret_cast<const VkLayerInstanceCreateInfo*>(pCreateInfo->pNext);

    // Step through the chain of pNext until we get to the link info
    while( ( pLayerCreateInfo )
        && ( pLayerCreateInfo->sType != VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO ||
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
        std::lock_guard<std::mutex> lk( g_InstanceMtx );

        LayerInstanceDispatchTable functions =
            LayerInstanceDispatchTable( *pInstance, pfnGetInstanceProcAddr );

        g_InstanceFunctions.emplace( *pInstance, functions );
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////
VK_LAYER_EXPORT void VKAPI_CALL vkDestroyInstance(
    VkInstance                          instance,
    const VkAllocationCallbacks*        pAllocator )
{
    std::lock_guard<std::mutex> lk( g_InstanceMtx );
    g_InstanceFunctions.erase( instance );
}
