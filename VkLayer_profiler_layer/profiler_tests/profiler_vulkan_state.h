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

#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string_view>

#include "vk_dispatch_tables.h"

#include "profiler_layer_functions/core/VkInstance_functions.h"
#include "profiler_layer_functions/core/VkDevice_functions.h"

#define VERIFY_RESULT( VK, EXPR ) VK->VerifyResult( (EXPR), #EXPR )

namespace Profiler
{
    class VulkanError
    {
    public:
        const VkResult              Result;
        const std::string           Message;

    public:
        inline VulkanError( VkResult result, std::string_view message ) : Result( result ), Message( message ) {}

    };

    class VulkanExtension
    {
    public:
        std::string Name;
        bool Required;
        bool Enabled;
        uint32_t Spec;

    public:
        explicit VulkanExtension( const std::string& name, bool required = true )
            : Name( name )
            , Required( required )
            , Enabled( false )
            , Spec( 0 )
        {
        }
    };

    class VulkanFeature
    {
    public:
        std::string Name;
        std::string ExtensionName;
        bool Required;
        bool Enabled;

    public:
        explicit VulkanFeature( const std::string& name, const std::string& extensionName = std::string(), bool required = true )
            : Name( name )
            , ExtensionName( extensionName )
            , Required( required )
            , Enabled( false )
        {}

        virtual ~VulkanFeature() {}

        // VulkanState obtains this pointer for 2 purposes.
        // First, it reads available features from VkPhysicalDevice into the structure. Then Configure is called and
        // this object can adjust the feature bits, or return false if the requested feature is not present.
        // Next, the same pointer is passed to the vkCreateDevice in the pNext chain.
        virtual void* GetCreateInfo() = 0;

        // Called after the VkPhysicalDevice features have been queried to check if required bits are supported.
        virtual bool Configure() { return true; }
    };

    class VulkanState
    {
    public:
        VkApplicationInfo           ApplicationInfo;
        VkInstance                  Instance;
        VkPhysicalDevice            PhysicalDevice;
        VkPhysicalDeviceProperties  PhysicalDeviceProperties;
        VkPhysicalDeviceMemoryProperties PhysicalDeviceMemoryProperties;
        std::vector<VkQueueFamilyProperties> PhysicalDeviceQueueProperties;
        VkDevice                    Device;
        uint32_t                    QueueFamilyIndex;
        VkQueue                     Queue;
        VkCommandPool               CommandPool;
        VkDescriptorPool            DescriptorPool;

        struct CreateInfo
        {
            std::vector<VulkanExtension*> InstanceExtensions = {};
            std::vector<VulkanExtension*> DeviceExtensions = {};
            std::vector<VulkanFeature*> DeviceFeatures = {};
        };

    public:
        inline VulkanState( CreateInfo createInfo = CreateInfo() )
            : ApplicationInfo()
            , Instance( VK_NULL_HANDLE )
            , PhysicalDevice( VK_NULL_HANDLE )
            , PhysicalDeviceProperties()
            , Device( VK_NULL_HANDLE )
            , QueueFamilyIndex( -1 )
            , Queue( VK_NULL_HANDLE )
            , CommandPool( VK_NULL_HANDLE )
            , DescriptorPool( VK_NULL_HANDLE )
        {
            DeviceProfiler* profiler = nullptr;

            // Application info
            {
                ApplicationInfo = {};
                ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
                ApplicationInfo.apiVersion = VK_API_VERSION_1_0;
                ApplicationInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
                ApplicationInfo.pApplicationName = "VK_LAYER_profiler_ULT";
                ApplicationInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
                ApplicationInfo.pEngineName = "VK_LAYER_profiler_ULT";
            }

            // Init instance
            {
                VkInstanceCreateInfo instanceCreateInfo = {};
                instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
                instanceCreateInfo.pApplicationInfo = &ApplicationInfo;

                uint32_t availableExtensionCount = 0;
                vkEnumerateInstanceExtensionProperties( nullptr, &availableExtensionCount, nullptr );
                std::vector<VkExtensionProperties> availableExtensions( availableExtensionCount );
                vkEnumerateInstanceExtensionProperties( nullptr, &availableExtensionCount, availableExtensions.data() );

                std::vector<const char*> extensions = GetExtensions( createInfo.InstanceExtensions, availableExtensions );
                instanceCreateInfo.enabledExtensionCount = uint32_t( extensions.size() );
                instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

                VERIFY_RESULT( this, vkCreateInstance( &instanceCreateInfo, nullptr, &Instance ) );
            }

            // Select primary display device
            {
                uint32_t physicalDeviceCount = 1;
                VERIFY_RESULT( this, vkEnumeratePhysicalDevices( Instance, &physicalDeviceCount, &PhysicalDevice ) );

                // Get selected physical device properties
                vkGetPhysicalDeviceProperties( PhysicalDevice, &PhysicalDeviceProperties );
                vkGetPhysicalDeviceMemoryProperties( PhysicalDevice, &PhysicalDeviceMemoryProperties );
            }
            
            // Select graphics queue
            {
                uint32_t queueFamilyCount = 0;
                vkGetPhysicalDeviceQueueFamilyProperties( PhysicalDevice, &queueFamilyCount, nullptr );

                PhysicalDeviceQueueProperties.resize( queueFamilyCount );
                vkGetPhysicalDeviceQueueFamilyProperties( PhysicalDevice, &queueFamilyCount, PhysicalDeviceQueueProperties.data() );

                for( uint32_t i = 0; i < queueFamilyCount; ++i )
                {
                    const VkQueueFamilyProperties& properties = PhysicalDeviceQueueProperties[ i ];
                    
                    if( (properties.queueCount > 0) &&
                        (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                        (properties.timestampValidBits > 0) )
                    {
                        QueueFamilyIndex = i;
                        break;
                    }
                }
            }

            // Create logical device
            {
                const float deviceQueueDefaultPriority = 1.f;

                VkDeviceQueueCreateInfo deviceQueueCreateInfo = {};
                deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                deviceQueueCreateInfo.queueFamilyIndex = QueueFamilyIndex;
                deviceQueueCreateInfo.queueCount = 1;
                deviceQueueCreateInfo.pQueuePriorities = &deviceQueueDefaultPriority;

                VkDeviceCreateInfo deviceCreateInfo = {};
                deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
                deviceCreateInfo.queueCreateInfoCount = 1;
                deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;

                uint32_t availableExtensionCount = 0;
                vkEnumerateDeviceExtensionProperties( PhysicalDevice, nullptr, &availableExtensionCount, nullptr );
                std::vector<VkExtensionProperties> availableExtensions( availableExtensionCount );
                vkEnumerateDeviceExtensionProperties( PhysicalDevice, nullptr, &availableExtensionCount, availableExtensions.data() );

                std::vector<const char*> extensions = GetExtensions( createInfo.DeviceExtensions, availableExtensions );
                deviceCreateInfo.enabledExtensionCount = uint32_t( extensions.size() );
                deviceCreateInfo.ppEnabledExtensionNames = extensions.data();

                VkPhysicalDeviceFeatures2 features = {};
                features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                deviceCreateInfo.pNext = &features;

                VkBaseOutStructure** ppNext = (VkBaseOutStructure**)&features.pNext;
                for( VulkanFeature* pFeature : createInfo.DeviceFeatures )
                {
                    if( !pFeature->ExtensionName.empty() )
                    {
                        auto it = std::find_if( extensions.begin(), extensions.end(),
                            [&]( const char* pExtensionName ) -> bool {
                                return strcmp( pExtensionName, pFeature->ExtensionName.c_str() ) == 0;
                            } );

                        if( it == extensions.end() )
                        {
                            continue;
                        }
                    }

                    *ppNext = (VkBaseOutStructure*)pFeature->GetCreateInfo();
                    ppNext = &( *ppNext )->pNext;
                }

                vkGetPhysicalDeviceFeatures2( PhysicalDevice, &features );

                for( VulkanFeature* pFeature : createInfo.DeviceFeatures )
                {
                    pFeature->Enabled = pFeature->Configure();
                    if( !pFeature->Enabled && pFeature->Required )
                    {
                        throw VulkanError( VK_ERROR_FEATURE_NOT_PRESENT, pFeature->Name );
                    }
                }

                VERIFY_RESULT( this, vkCreateDevice( PhysicalDevice, &deviceCreateInfo, nullptr, &Device ) );

                // Get graphics queue handle
                vkGetDeviceQueue( Device, QueueFamilyIndex, 0, &Queue );
            }

            // Create descriptor pool
            {
                const VkDescriptorPoolSize descriptorPoolSizes[] =
                {
                    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
                };

                VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
                descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
                descriptorPoolCreateInfo.maxSets = 1000;
                descriptorPoolCreateInfo.poolSizeCount = std::extent_v<decltype(descriptorPoolSizes)>;
                descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;
                
                VERIFY_RESULT( this, vkCreateDescriptorPool( Device, &descriptorPoolCreateInfo, nullptr, &DescriptorPool ) );
            }

            // Initialize layer
            {
                VkInstance_Functions::Dispatch& id = VkInstance_Functions::InstanceDispatch.Create( Instance );
                id.Instance.Handle = Instance;
                id.Instance.ApplicationInfo = ApplicationInfo;
                id.Instance.SetInstanceLoaderData = SetInstanceLoaderData;
                id.Instance.Callbacks.Initialize( Instance, vkGetInstanceProcAddr );

                VkPhysicalDevice_Object& dev = id.Instance.PhysicalDevices[ PhysicalDevice ];
                dev.Properties = PhysicalDeviceProperties;
                dev.MemoryProperties = PhysicalDeviceMemoryProperties;
                dev.QueueFamilyProperties = PhysicalDeviceQueueProperties;

                VkDevice_Functions::Dispatch& dd = VkDevice_Functions::DeviceDispatch.Create( Device );
                dd.Device.Handle = Device;
                dd.Device.pPhysicalDevice = &dev;
                dd.Device.pInstance = &id.Instance;
                dd.Device.SetDeviceLoaderData = SetDeviceLoaderData;
                dd.Device.Callbacks.Initialize( Device, vkGetDeviceProcAddr );

                VkQueue_Object queue = {};
                queue.Handle = Queue;
                queue.Family = QueueFamilyIndex;
                queue.Index = 0;
                queue.Flags = VK_QUEUE_GRAPHICS_BIT;
                dd.Device.Queues.emplace( Queue, queue );

                dd.Profiler.Initialize( &dd.Device, nullptr );
                profiler = &dd.Profiler;
            }

            // Create command pool
            {
                VkCommandPoolCreateInfo commandPoolCreateInfo = {};
                commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                commandPoolCreateInfo.queueFamilyIndex = QueueFamilyIndex;

                VERIFY_RESULT( this, vkCreateCommandPool( Device, &commandPoolCreateInfo, nullptr, &CommandPool ) );

                // Register the command pool.
                profiler->CreateCommandPool( CommandPool, &commandPoolCreateInfo );
            }
        }

        inline void VerifyResult( VkResult result, const char* message )
        {
            if( result != VK_SUCCESS && result != VK_INCOMPLETE )
            {
                throw VulkanError( result, message );
            }
        }

        inline ~VulkanState()
        {
            vkDeviceWaitIdle( Device );

            VkDevice_Functions::Dispatch& dd = VkDevice_Functions::DeviceDispatch.Get( Device );
            dd.Profiler.Destroy();

            VkDevice_Functions::DeviceDispatch.Erase( Device );
            // This frees all resources created with this device
            vkDestroyDevice( Device, nullptr );
            Device = VK_NULL_HANDLE;
            QueueFamilyIndex = -1;
            Queue = VK_NULL_HANDLE;
            CommandPool = VK_NULL_HANDLE;
            DescriptorPool = VK_NULL_HANDLE;

            VkInstance_Functions::InstanceDispatch.Erase( Instance );
            // This frees all resources created with this instance
            vkDestroyInstance( Instance, nullptr );
            Instance = VK_NULL_HANDLE;
            PhysicalDevice = VK_NULL_HANDLE;
            PhysicalDeviceProperties = {};
            ApplicationInfo = {};
        }

        inline VkLayerDeviceDispatchTable GetLayerDispatchTable() const
        {
            VkLayerDeviceDispatchTable dispatchTable = {};
            dispatchTable.Initialize( Device, VkDevice_Functions::GetDeviceProcAddr );
            return dispatchTable;
        }

        inline VkLayerInstanceDispatchTable GetLayerInstanceDispatchTable() const
        {
            VkLayerInstanceDispatchTable dispatchTable = {};
            dispatchTable.Initialize( Instance, VkInstance_Functions::GetInstanceProcAddr );
            return dispatchTable;
        }

        inline static VkResult SetInstanceLoaderData( VkInstance instance, void *object )
        {
            (*(void**)object) = (*(void**)instance);
            return VK_SUCCESS;
        }

        inline static VkResult SetDeviceLoaderData( VkDevice device, void *object )
        {
            (*(void**)object) = (*(void**)device);
            return VK_SUCCESS;
        }

    private:
        std::vector<const char*> GetExtensions(
            const std::vector<VulkanExtension*>& requestedExtensions,
            const std::vector<VkExtensionProperties>& supportedExtensions ) const
        {
            std::vector<const char*> extensions = {};
            for( VulkanExtension* pExtension : requestedExtensions )
            {
                auto it = std::find_if( supportedExtensions.begin(), supportedExtensions.end(),
                    [&]( const VkExtensionProperties& ext ) -> bool {
                        if( strcmp( ext.extensionName, pExtension->Name.c_str() ) == 0 )
                            return ( pExtension->Spec == 0 ) ||
                                   ( pExtension->Spec == ext.specVersion );
                        return false;
                    } );

                if( it == supportedExtensions.end() )
                {
                    // Throw an exception if the required extension is not present.
                    if( pExtension->Required )
                    {
                        throw VulkanError( VK_ERROR_EXTENSION_NOT_PRESENT,
                            pExtension->Name + ", spec " + std::to_string( pExtension->Spec ) );
                    }
                }

                extensions.push_back( pExtension->Name.c_str() );
                pExtension->Enabled = true;
            }

            return extensions;
        }
    };
}
