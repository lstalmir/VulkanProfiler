#include "VkInstance_functions.h"
#include "VkDevice_functions.h"
#include "VkLoader_functions.h"
#include "VkLayer_profiler_layer.generated.h"
#include "profiler_layer_functions/Helpers.h"

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

        auto EnumerateDeviceExtensionProperties = VkDevice_Functions::EnumerateDeviceExtensionProperties;
        auto EnumerateDeviceLayerProperties = VkDevice_Functions::EnumerateDeviceLayerProperties;

        // VkInstance_Functions
        GETPROCADDR( GetInstanceProcAddr );
        GETPROCADDR( CreateInstance );
        GETPROCADDR( DestroyInstance );
        GETPROCADDR( CreateDevice );
        GETPROCADDR( EnumerateInstanceLayerProperties );
        GETPROCADDR( EnumerateInstanceExtensionProperties );
        GETPROCADDR( EnumerateDeviceLayerProperties );
        GETPROCADDR( EnumerateDeviceExtensionProperties );
        GETPROCADDR( SetDebugUtilsObjectNameEXT );
        #ifdef VK_USE_PLATFORM_WIN32_KHR
        GETPROCADDR( CreateWin32SurfaceKHR );
        #endif
        #ifdef VK_USE_PLATFORM_WAYLAND_KHR
        GETPROCADDR( CreateWaylandSurfaceKHR );
        #endif
        #ifdef VK_USE_PLATFORM_XLIB_KHR
        GETPROCADDR( CreateXlibSurfaceKHR );
        #endif
        GETPROCADDR( DestroySurfaceKHR );

        // Get address from the next layer
        auto& id = InstanceDispatch.Get( instance );

        return id.Instance.Callbacks.GetInstanceProcAddr( instance, pName );
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
            auto& id = InstanceDispatch.Create( *pInstance );

            id.Instance.Handle = *pInstance;
            id.Instance.ApplicationInfo.apiVersion = pCreateInfo->pApplicationInfo->apiVersion;

            // Get function addresses
            layer_init_instance_dispatch_table(
                *pInstance, &id.Instance.Callbacks, pfnGetInstanceProcAddr );
            
            // Call loader to obtain API version we're running
            VkLoader_Functions::EnumerateInstanceVersion( &id.Instance.LoaderVersion );

            id.Instance.SetInstanceLoaderData = pfnSetInstanceLoaderData;

            id.Instance.Callbacks.CreateDevice = reinterpret_cast<PFN_vkCreateDevice>(
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
        const VkAllocationCallbacks* pAllocator )
    {
        auto& id = InstanceDispatch.Get( instance );

        // Destroy the instance
        id.Instance.Callbacks.DestroyInstance( instance, pAllocator );

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
        const VkAllocationCallbacks* pAllocator,
        VkDevice* pDevice )
    {
        auto& id = InstanceDispatch.Get( physicalDevice );

        // Prefetch the device link info before creating the device to be sure we have vkDestroyDevice function available
        auto* pLayerLinkInfo = GetLayerLinkInfo<VkLayerDeviceCreateInfo>( pCreateInfo, VK_LAYER_LINK_INFO );
        auto* pLoaderCallbacks = GetLayerLinkInfo<VkLayerDeviceCreateInfo>( pCreateInfo, VK_LOADER_DATA_CALLBACK );

        if( !pLayerLinkInfo )
        {
            // Link info not found, vkGetDeviceProcAddr unavailable
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr =
            pLayerLinkInfo->u.pLayerInfo->pfnNextGetDeviceProcAddr;

        PFN_vkSetDeviceLoaderData pfnSetDeviceLoaderData =
            (pLoaderCallbacks != nullptr)
            ? pLoaderCallbacks->u.pfnSetDeviceLoaderData
            : VkLoader_Functions::SetDeviceLoaderData;

        // Move chain on for next layer
        pLayerLinkInfo->u.pLayerInfo = pLayerLinkInfo->u.pLayerInfo->pNext;

        // Create the device
        VkResult result = id.Instance.Callbacks.CreateDevice(
            physicalDevice, pCreateInfo, pAllocator, pDevice );

        // Initialize dispatch for the created device object
        if( result == VK_SUCCESS )
        {
            result = VkDevice_Functions::OnDeviceCreate(
                physicalDevice,
                pCreateInfo,
                pfnGetDeviceProcAddr,
                pfnSetDeviceLoaderData,
                pAllocator,
                *pDevice );
        }

        if( result != VK_SUCCESS &&
            *pDevice != VK_NULL_HANDLE )
        {
            // Initialization of the layer failed, destroy the device
            PFN_vkDestroyDevice pfnDestroyDevice = reinterpret_cast<PFN_vkDestroyDevice>(
                pfnGetDeviceProcAddr( *pDevice, "vkDestroyDevice" ));

            pfnDestroyDevice( *pDevice, pAllocator );
        }

        // Device created successfully
        return result;
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

    #ifdef VK_USE_PLATFORM_WIN32_KHR
    /***********************************************************************************\

    Function:
        CreateWin32SurfaceKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkInstance_Functions::CreateWin32SurfaceKHR(
        VkInstance instance,
        const VkWin32SurfaceCreateInfoKHR* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkSurfaceKHR* pSurface )
    {
        auto& id = InstanceDispatch.Get( instance );

        VkResult result = id.Instance.Callbacks.CreateWin32SurfaceKHR(
            instance, pCreateInfo, pAllocator, pSurface );

        if( result == VK_SUCCESS )
        {
            VkSurfaceKhr_Object surfaceObject = {};
            surfaceObject.Handle = *pSurface;
            surfaceObject.Window = pCreateInfo->hwnd;

            id.Instance.Surfaces.emplace( *pSurface, surfaceObject );
        }

        return result;
    }
    #endif // VK_USE_PLATFORM_WIN32_KHR

    #ifdef VK_USE_PLATFORM_XLIB_KHR
    /***********************************************************************************\

    Function:
        CreateWin32SurfaceKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkInstance_Functions::CreateXlibSurfaceKHR(
        VkInstance instance,
        const VkXlibSurfaceCreateInfoKHR* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkSurfaceKHR* pSurface )
    {
        auto& id = InstanceDispatch.Get( instance );

        VkResult result = id.Instance.Callbacks.CreateXlibSurfaceKHR(
            instance, pCreateInfo, pAllocator, pSurface );

        if( result == VK_SUCCESS )
        {
            VkSurfaceKhr_Object surfaceObject = {};
            surfaceObject.Handle = *pSurface;
            surfaceObject.Window = pCreateInfo->window;

            id.Instance.Surfaces.emplace( *pSurface, surfaceObject );
        }

        return result;
    }
    #endif // VK_USE_PLATFORM_XLIB_KHR

    /***********************************************************************************\

    Function:
        DestroySurfaceKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkInstance_Functions::DestroySurfaceKHR(
        VkInstance instance,
        VkSurfaceKHR surface,
        const VkAllocationCallbacks* pAllocator )
    {
        auto& id = InstanceDispatch.Get( instance );

        // Remove surface entry
        id.Instance.Surfaces.erase( surface );

        id.Instance.Callbacks.DestroySurfaceKHR(
            instance, surface, pAllocator );
    }
}
