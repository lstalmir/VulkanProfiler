// Copyright (c) 2019-2025 Lukasz Stalmirski
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

#include "VkDevice_functions_base.h"
#include "VkInstance_functions.h"
#include "profiler_layer_functions/Helpers.h"
#include "profiler/profiler_helpers.h"

namespace Profiler
{
    // Initialize profilers dispatch tables map
    DispatchableMap<VkDevice_Functions_Base::Dispatch> VkDevice_Functions_Base::DeviceDispatch;


    /***********************************************************************************\

    Function:
        CreateDeviceBase

    Description:
        Initializes profiler for the device.

    \***********************************************************************************/
    VkResult VkDevice_Functions_Base::CreateDeviceBase(
        VkPhysicalDevice physicalDevice,
        const VkDeviceCreateInfo* pCreateInfo,
        PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr,
        PFN_vkSetDeviceLoaderData pfnSetDeviceLoaderData,
        const VkAllocationCallbacks* pAllocator,
        VkDevice device )
    {
        // Get instance dispatch table
        auto& id = VkInstance_Functions::InstanceDispatch.Get( physicalDevice );

        // Create new dispatch table for the device
        auto& dd = DeviceDispatch.Create( device );

        dd.Device.Callbacks.Initialize( device, pfnGetDeviceProcAddr );

        dd.Device.SetDeviceLoaderData = pfnSetDeviceLoaderData;

        dd.Device.Handle = device;
        dd.Device.pInstance = &id.Instance;
        dd.Device.pPhysicalDevice = &id.Instance.PhysicalDevices[ physicalDevice ];

        // Save enabled extensions
        for( uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; ++i )
        {
            dd.Device.EnabledExtensions.insert( pCreateInfo->ppEnabledExtensionNames[ i ] );
        }

        // Create wrapper for each device queue
        for( uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; ++i )
        {
            const VkDeviceQueueCreateInfo& queueCreateInfo =
                pCreateInfo->pQueueCreateInfos[ i ];

            const VkQueueFamilyProperties& queueProperties =
                dd.Device.pPhysicalDevice->QueueFamilyProperties[ queueCreateInfo.queueFamilyIndex ];

            for( uint32_t queueIndex = 0; queueIndex < queueCreateInfo.queueCount; ++queueIndex )
            {
                VkQueue_Object queueObject;

                // Get queue handle
                dd.Device.Callbacks.GetDeviceQueue(
                    device, queueCreateInfo.queueFamilyIndex, queueIndex, &queueObject.Handle );

                queueObject.Flags = queueProperties.queueFlags;
                queueObject.Family = queueCreateInfo.queueFamilyIndex;
                queueObject.Index = queueIndex;

                dd.Device.Queues.emplace( queueObject.Handle, queueObject );
            }
        }

        // Check if profiler create info was provided
        const VkProfilerCreateInfoEXT* pProfilerCreateInfo = nullptr;

        for( const auto& it : PNextIterator( pCreateInfo->pNext ) )
        {
            if( it.sType == VK_STRUCTURE_TYPE_PROFILER_CREATE_INFO_EXT )
            {
                pProfilerCreateInfo = reinterpret_cast<const VkProfilerCreateInfoEXT*>(&it);
                break;
            }
        }

        if( dd.Device.pInstance->EnabledExtensions.count( VK_EXT_DEBUG_UTILS_EXTENSION_NAME ) )
        {
            // Create debug messenger to receive validation messages
            VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = {};
            debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugMessengerCreateInfo.pfnUserCallback = VkDevice_debug_Object::DebugUtilsMessengerCallback;
            debugMessengerCreateInfo.pUserData = &dd.Device.Debug;

            VkResult result = id.Instance.Callbacks.CreateDebugUtilsMessengerEXT(
                id.Instance.Handle,
                &debugMessengerCreateInfo,
                pAllocator,
                &dd.Device.Debug.Messenger );
        }

        // Initialize the profiler object
        VkResult result = dd.Profiler.Initialize( &dd.Device, pProfilerCreateInfo );

        // Initialize the profiler frontend object
        dd.ProfilerFrontend.Initialize( dd.Device, dd.Profiler );

        if( result != VK_SUCCESS )
        {
            // Profiler initialization failed
            DeviceDispatch.Erase( device );
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        DestroyDeviceBase

    Description:
        Destroys profiler for the device.

    \***********************************************************************************/
    void VkDevice_Functions_Base::DestroyDeviceBase( VkDevice device )
    {
        auto& dd = DeviceDispatch.Get( device );

        // Destroy the profiler instance
        dd.Profiler.Destroy();
        // Destroy the overlay
        dd.Overlay.Destroy();

        if( dd.Device.Debug.Messenger != VK_NULL_HANDLE )
        {
            dd.Device.pInstance->Callbacks.DestroyDebugUtilsMessengerEXT(
                dd.Device.pInstance->Handle,
                dd.Device.Debug.Messenger,
                nullptr );
        }

        DeviceDispatch.Erase( device );
    }
}
