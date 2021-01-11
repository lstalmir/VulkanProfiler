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

#include "VkPhysicalDevice_functions.h"
#include "VkInstance_functions.h"
#include "VkDevice_functions.h"
#include "VkLoader_functions.h"
#include "profiler_layer_functions/Helpers.h"
#include "profiler_layer_objects/VkDevice_object.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        CreateDevice

    Description:


    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkPhysicalDevice_Functions::CreateDevice(
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

        // Get physical device description
        VkPhysicalDeviceProperties deviceProperties;
        id.Instance.Callbacks.GetPhysicalDeviceProperties( physicalDevice, &deviceProperties );

        // ppEnabledExtensionNames may change, create set to keep all needed extensions and avoid duplicates
        std::unordered_set<std::string> deviceExtensions;

        for( uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; ++i )
        {
            deviceExtensions.insert( pCreateInfo->ppEnabledExtensionNames[ i ] );
        }

        // Enumerate available device extensions
        uint32_t availableExtensionCount = 0;
        id.Instance.Callbacks.EnumerateDeviceExtensionProperties(
            physicalDevice, nullptr, &availableExtensionCount, nullptr );

        VkExtensionProperties* pAvailableDeviceExtensions = new VkExtensionProperties[ availableExtensionCount ];
        id.Instance.Callbacks.EnumerateDeviceExtensionProperties(
            physicalDevice, nullptr, &availableExtensionCount, pAvailableDeviceExtensions );

        // Enable available optional device extensions
        const auto optionalDeviceExtensions = DeviceProfiler::EnumerateOptionalDeviceExtensions();

        for( uint32_t i = 0; i < availableExtensionCount; ++i )
        {
            if( optionalDeviceExtensions.count( pAvailableDeviceExtensions[ i ].extensionName ) )
            {
                deviceExtensions.insert( pAvailableDeviceExtensions[ i ].extensionName );
            }
        }

        delete[] pAvailableDeviceExtensions;

        // Convert to continuous memory block
        std::vector<const char*> enabledDeviceExtensions;
        enabledDeviceExtensions.reserve( deviceExtensions.size() );

        for( const std::string& extensionName : deviceExtensions )
        {
            enabledDeviceExtensions.push_back( extensionName.c_str() );
        }

        // Override device create info
        VkDeviceCreateInfo createInfo = (*pCreateInfo);
        createInfo.enabledExtensionCount = enabledDeviceExtensions.size();
        createInfo.ppEnabledExtensionNames = enabledDeviceExtensions.data();

        // Move chain on for next layer
        pLayerLinkInfo->u.pLayerInfo = pLayerLinkInfo->u.pLayerInfo->pNext;

        // Create the device
        VkResult result = id.Instance.Callbacks.CreateDevice(
            physicalDevice, &createInfo, pAllocator, pDevice );

        // Initialize dispatch for the created device object
        if( result == VK_SUCCESS )
        {
            result = VkDevice_Functions::CreateDeviceBase(
                physicalDevice,
                &createInfo,
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

            *pDevice = VK_NULL_HANDLE;
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        EnumerateDeviceLayerProperties

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkPhysicalDevice_Functions::EnumerateDeviceLayerProperties(
        VkPhysicalDevice physicalDevice,
        uint32_t* pPropertyCount,
        VkLayerProperties* pLayerProperties )
    {
        // Device layers are deprecated since Vulkan 1.1, return instance layers
        return VkInstance_Functions::EnumerateInstanceLayerProperties( pPropertyCount, pLayerProperties );
    }

    /***********************************************************************************\

    Function:
        EnumerateDeviceExtensionProperties

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkPhysicalDevice_Functions::EnumerateDeviceExtensionProperties(
        VkPhysicalDevice physicalDevice,
        const char* pLayerName,
        uint32_t* pPropertyCount,
        VkExtensionProperties* pProperties )
    {
        const bool queryThisLayerExtensionsOnly =
            (pLayerName) &&
            (strcmp( pLayerName, VK_LAYER_profiler_name ) != 0);

        // EnumerateDeviceExtensionProperties is actually VkInstance (VkPhysicalDevice) function.
        // Get dispatch table associated with the VkPhysicalDevice and invoke next layer's
        // vkEnumerateDeviceExtensionProperties implementation.
        auto id = InstanceDispatch.Get( physicalDevice );

        VkResult result = VK_SUCCESS;

        // SPEC: pPropertyCount MUST be valid uint32_t pointer
        uint32_t propertyCount = *pPropertyCount;

        if( !pLayerName || !queryThisLayerExtensionsOnly )
        {
            result = id.Instance.Callbacks.EnumerateDeviceExtensionProperties(
                physicalDevice, pLayerName, pPropertyCount, pProperties );
        }
        else
        {
            // pPropertyCount now contains number of pProperties used
            *pPropertyCount = 0;
        }

        static VkExtensionProperties layerExtensions[] = {
            { VK_EXT_PROFILER_EXTENSION_NAME, VK_EXT_PROFILER_SPEC_VERSION },
            { VK_EXT_DEBUG_MARKER_EXTENSION_NAME, VK_EXT_DEBUG_MARKER_SPEC_VERSION } };

        if( !pLayerName || queryThisLayerExtensionsOnly )
        {
            if( pProperties )
            {
                const size_t dstBufferSize =
                    (propertyCount - *pPropertyCount) * sizeof( VkExtensionProperties );

                // Copy device extension properties to output ptr
                std::memcpy( pProperties + (*pPropertyCount), layerExtensions,
                    std::min( dstBufferSize, sizeof( layerExtensions ) ) );

                if( dstBufferSize < sizeof( layerExtensions ) )
                {
                    // Not enough space in the buffer
                    result = VK_INCOMPLETE;
                }
            }

            // Return number of extensions exported by this layer
            *pPropertyCount += std::extent_v<decltype(layerExtensions)>;
        }

        return result;
    }
}
