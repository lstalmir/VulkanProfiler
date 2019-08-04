#include "VkLayer_profiler_layer.h"
#include <unordered_map>
#include <mutex>

////////////////////////////////////////////////////////////////////////////////////////
#define GetNextProcAddr( INSTANCE ) pfnGetInstanceProcAddr( INSTANCE, __FUNCTION__ )

struct InstanceFunctions
{
    PFN_vkGetInstanceProcAddr pfnGetInstanceProcAddr = nullptr;
};

struct VkInstanceEqualFunc
{
    inline bool operator()( const VkInstance& a, const VkInstance& b ) const
    {
        // Compare addresses of dispatch tables
        return *reinterpret_cast<void**>(a) == *reinterpret_cast<void**>(b);
    }
};

using InstanceFunctionMap = std::unordered_map<
    VkInstance,
    InstanceFunctions,
    std::hash<VkInstance>,
    VkInstanceEqualFunc>;

std::mutex g_InstanceMtx;
InstanceFunctionMap g_InstanceFunctions;

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
        (PFN_vkCreateInstance)GetNextProcAddr( VK_NULL_HANDLE );

    // Invoke vkCreateInstance of next layer
    VkResult result = pfnCreateInstance( pCreateInfo, pAllocator, pInstance );

    // Register callbacks to the next layer
    if( result == VK_SUCCESS )
    {
        std::lock_guard<std::mutex> lk( g_InstanceMtx );

        InstanceFunctions functions;
        functions.pfnGetInstanceProcAddr = pfnGetInstanceProcAddr;

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
