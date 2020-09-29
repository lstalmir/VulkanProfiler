// Copyright (c) 2020 Lukasz Stalmirski
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

#include "VkInstance_functions.h"
#include "VkDevice_functions.h"
#include "VkLoader_functions.h"
#include "VkLayer_profiler_layer.generated.h"
#include "profiler_layer_functions/Helpers.h"

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
        const char* pName )
    {
        // VkInstance_Functions
        GETPROCADDR( GetInstanceProcAddr );
        GETPROCADDR( CreateInstance );
        GETPROCADDR( DestroyInstance );
        GETPROCADDR( EnumerateInstanceLayerProperties );
        GETPROCADDR( EnumerateInstanceExtensionProperties );

        // VkPhysicalDevice_Functions
        GETPROCADDR( CreateDevice );
        GETPROCADDR( EnumerateDeviceLayerProperties );
        GETPROCADDR( EnumerateDeviceExtensionProperties );

        // VK_KHR_surface functions
        GETPROCADDR( DestroySurfaceKHR );

        #ifdef VK_USE_PLATFORM_WIN32_KHR
        // VK_KHR_win32_surface functions
        GETPROCADDR( CreateWin32SurfaceKHR );
        #endif
        #ifdef VK_USE_PLATFORM_WAYLAND_KHR
        // VK_KHR_wayland_surface functions
        GETPROCADDR( CreateWaylandSurfaceKHR );
        #endif
        #ifdef VK_USE_PLATFORM_XCB_KHR
        // VK_KHR_xcb_surface functions
        GETPROCADDR( CreateXcbSurfaceKHR );
        #endif
        #ifdef VK_USE_PLATFORM_XLIB_KHR
        // VK_KHR_xlib_surface functions
        GETPROCADDR( CreateXlibSurfaceKHR );
        #endif

        // vkGetInstanceProcAddr can be used to query device functions
        PFN_vkVoidFunction deviceFunction = VkDevice_Functions::GetDeviceProcAddr( nullptr, pName );

        if( deviceFunction )
        {
            return deviceFunction;
        }

        // Get address from the next layer
        return InstanceDispatch.Get( instance ).Instance.Callbacks.GetInstanceProcAddr( instance, pName );
    }

    /***********************************************************************************\

    Function:
        CreateInstance

    Description:
        Creates VkInstance object and initializes dispatch table.

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkInstance_Functions::CreateInstance(
        const VkInstanceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkInstance* pInstance )
    {
        auto* pLayerCreateInfo = GetLayerLinkInfo<VkLayerInstanceCreateInfo>( pCreateInfo, VK_LAYER_LINK_INFO );
        auto* pLoaderCallbacks = GetLayerLinkInfo<VkLayerInstanceCreateInfo>( pCreateInfo, VK_LOADER_DATA_CALLBACK );

        if( !pLayerCreateInfo )
        {
            // No loader instance create info
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        PFN_vkGetInstanceProcAddr pfnGetInstanceProcAddr =
            pLayerCreateInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr;

        PFN_vkSetInstanceLoaderData pfnSetInstanceLoaderData =
            (pLoaderCallbacks != nullptr)
            ? pLoaderCallbacks->u.pfnSetInstanceLoaderData
            : VkLoader_Functions::SetInstanceLoaderData;

        PFN_vkCreateInstance pfnCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(
            pfnGetInstanceProcAddr( VK_NULL_HANDLE, "vkCreateInstance" ));

        // Move chain on for next layer
        pLayerCreateInfo->u.pLayerInfo = pLayerCreateInfo->u.pLayerInfo->pNext;

        // Invoke vkCreateInstance of next layer
        VkResult result = pfnCreateInstance( pCreateInfo, pAllocator, pInstance );

        // Register callbacks to the next layer
        if( result == VK_SUCCESS )
        {
            result = CreateInstanceBase(
                pCreateInfo,
                pfnGetInstanceProcAddr,
                pfnSetInstanceLoaderData,
                pAllocator,
                *pInstance );
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
        const VkAllocationCallbacks* pAllocator )
    {
        auto& id = InstanceDispatch.Get( instance );
        auto pfnDestroyInstance = id.Instance.Callbacks.DestroyInstance;

        // Cleanup layer infrastructure
        VkInstance_Functions_Base::DestroyInstanceBase( instance );

        // Destroy the instance
        pfnDestroyInstance( instance, pAllocator );
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
            strcpy( pLayerProperties->layerName, VK_LAYER_profiler_name );
            strcpy( pLayerProperties->description, VK_LAYER_profiler_desc );

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

        // Report implemented instance extensions
        static VkExtensionProperties layerExtensions[] = {
            { VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_SPEC_VERSION } };

        if( pExtensionProperties )
        {
            const size_t dstBufferSize = *pPropertyCount * sizeof( VkExtensionProperties );

            // Copy device extension properties to output ptr
            std::memcpy( pExtensionProperties, layerExtensions,
                std::min( dstBufferSize, sizeof( layerExtensions ) ) );

            if( dstBufferSize < sizeof( layerExtensions ) )
                return VK_INCOMPLETE;
        }

        // SPEC: pPropertyCount MUST be valid uint32_t pointer
        *pPropertyCount = std::extent_v<decltype(layerExtensions)>;

        return VK_SUCCESS;
    }
}
