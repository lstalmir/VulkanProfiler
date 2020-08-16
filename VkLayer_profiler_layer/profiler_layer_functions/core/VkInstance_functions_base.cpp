#include "VkInstance_functions_base.h"
#include "VkLoader_functions.h"

namespace Profiler
{
    DispatchableMap<VkInstance_Functions_Base::Dispatch> VkInstance_Functions_Base::InstanceDispatch;


    /***********************************************************************************\

    Function:
        CreateInstanceBase

    Description:
        Initializes layer infrastucture for the new instance.

    \***********************************************************************************/
    VkResult VkInstance_Functions_Base::CreateInstanceBase(
        const VkInstanceCreateInfo* pCreateInfo,
        PFN_vkGetInstanceProcAddr pfnGetInstanceProcAddr,
        PFN_vkSetInstanceLoaderData pfnSetInstanceLoaderData,
        const VkAllocationCallbacks* pAllocator,
        VkInstance instance )
    {
        auto& id = InstanceDispatch.Create( instance );

        id.Instance.Handle = instance;
        id.Instance.ApplicationInfo.apiVersion = pCreateInfo->pApplicationInfo->apiVersion;

        // Get function addresses
        init_layer_instance_dispatch_table( instance, pfnGetInstanceProcAddr, id.Instance.Callbacks );

        // Fill additional callbacks
        id.Instance.SetInstanceLoaderData = pfnSetInstanceLoaderData;
        id.Instance.Callbacks.CreateDevice = reinterpret_cast<PFN_vkCreateDevice>(
            pfnGetInstanceProcAddr( instance, "vkCreateDevice" ));

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        CreateInstanceBase

    Description:
        Destroys layer infrastucture of the instance.

    \***********************************************************************************/
    void VkInstance_Functions_Base::DestroyInstanceBase(
        VkInstance instance )
    {
        InstanceDispatch.Erase( instance );
    }
}
