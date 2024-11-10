// Copyright (c) 2019-2021 Lukasz Stalmirski
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

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
        id.Instance.Callbacks.Initialize( instance, pfnGetInstanceProcAddr );

        // Load settings
        id.Instance.LayerSettings.LoadFromVulkanLayerSettings( pCreateInfo, pAllocator );

        // Fill additional callbacks
        id.Instance.SetInstanceLoaderData = pfnSetInstanceLoaderData;
        id.Instance.Callbacks.CreateDevice = reinterpret_cast<PFN_vkCreateDevice>(
            pfnGetInstanceProcAddr( instance, "vkCreateDevice" ));

        // Save enabled extensions
        for( uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; ++i )
        {
            id.Instance.EnabledExtensions.insert( pCreateInfo->ppEnabledExtensionNames[ i ] );
        }

        // Initialize the memory profiler
        id.Instance.HostMemoryProfiler.Initialize();
        id.Instance.HostMemoryProfilerManager.Initialize();
        id.Instance.HostMemoryProfilerManager.RegisterMemoryProfiler(
            instance, &id.Instance.HostMemoryProfiler );

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
        auto& id = InstanceDispatch.Get( instance );

        id.Instance.HostMemoryProfilerManager.UnregisterMemoryProfiler( instance );
        id.Instance.HostMemoryProfilerManager.Destroy();
        id.Instance.HostMemoryProfiler.Destroy();

        InstanceDispatch.Erase( instance );
    }
}
