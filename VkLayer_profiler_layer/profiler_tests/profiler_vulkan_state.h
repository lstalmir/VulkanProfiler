// Copyright (c) 2019-2024 Lukasz Stalmirski
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
#include <algorithm>
#include <vector>
#include <string>
#include <string_view>
#include <string.h>

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
        virtual void* GetCreateInfo() { return nullptr; }

        // Called after the VkPhysicalDevice features have been queried to check if required bits are supported.
        virtual bool CheckSupport( const VkPhysicalDeviceFeatures2* pFeatures ) const { return true; }
        virtual void Configure( VkPhysicalDeviceFeatures2* pFeatures ) {}
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
        std::vector<VkQueue>        Queues;
        VkCommandPool               CommandPool;
        VkDescriptorPool            DescriptorPool;

        struct CreateInfo
        {
            std::vector<VulkanExtension*> InstanceExtensions;
            std::vector<VulkanExtension*> DeviceExtensions;
            std::vector<VulkanFeature*> DeviceFeatures;
        };

    public:
        inline VulkanState( const CreateInfo& createInfo = CreateInfo() )
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
            // Application info
            {
                ApplicationInfo = {};
                ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
                ApplicationInfo.apiVersion = VK_API_VERSION_1_1;
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
                // Create a queue of each family for testing.
                std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos( PhysicalDeviceQueueProperties.size() );

                const float deviceQueueDefaultPriority = 1.f;

                const uint32_t deviceQueueCount = static_cast<uint32_t>( deviceQueueCreateInfos.size() );
                for( uint32_t queueFamilyIndex = 0; queueFamilyIndex < deviceQueueCount; ++queueFamilyIndex )
                {
                    VkDeviceQueueCreateInfo& deviceQueueCreateInfo = deviceQueueCreateInfos[queueFamilyIndex];
                    deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                    deviceQueueCreateInfo.queueFamilyIndex = queueFamilyIndex;
                    deviceQueueCreateInfo.queueCount = 1;
                    deviceQueueCreateInfo.pQueuePriorities = &deviceQueueDefaultPriority;
                }

                VkDeviceCreateInfo deviceCreateInfo = {};
                deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
                deviceCreateInfo.queueCreateInfoCount = deviceQueueCount;
                deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();

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
                auto AppendNext = [&]( void* pNextStructure ) {
                    if( pNextStructure )
                    {
                        *ppNext = (VkBaseOutStructure*)pNextStructure;
                        ppNext = &( *ppNext )->pNext;
                    }
                };

                // Vulkan 1.1 core features support.
                VkPhysicalDeviceVulkan11Features vulkan11Features = {};
                vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
                AppendNext( ApiCheck( VK_API_VERSION_1_1 ) ? &vulkan11Features : nullptr );

                // Vulkan 1.2 core features support.
                VkPhysicalDeviceVulkan12Features vulkan12Features = {};
                vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
                AppendNext( ApiCheck( VK_API_VERSION_1_2 ) ? &vulkan12Features : nullptr );

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

                    AppendNext( pFeature->GetCreateInfo() );
                }

                vkGetPhysicalDeviceFeatures2( PhysicalDevice, &features );

                for( VulkanFeature* pFeature : createInfo.DeviceFeatures )
                {
                    pFeature->Enabled = pFeature->CheckSupport( &features );
                    if( !pFeature->Enabled && pFeature->Required )
                    {
                        throw VulkanError( VK_ERROR_FEATURE_NOT_PRESENT, pFeature->Name );
                    }
                }

                // Clear the core features to enable only the required bits.
                memset( &features.features, 0, sizeof( features.features ) );
                ClearStructure( vulkan11Features );
                ClearStructure( vulkan12Features );

                for( VulkanFeature* pFeature : createInfo.DeviceFeatures )
                {
                    if( pFeature->Enabled )
                    {
                        pFeature->Configure( &features );
                    }
                }

                VERIFY_RESULT( this, vkCreateDevice( PhysicalDevice, &deviceCreateInfo, nullptr, &Device ) );

                // Get queue handles
                Queues.resize( deviceQueueCount );

                for( uint32_t queueFamilyIndex = 0; queueFamilyIndex < deviceQueueCount; ++queueFamilyIndex )
                {
                    vkGetDeviceQueue( Device, queueFamilyIndex, 0, &Queues[queueFamilyIndex] );
                }

                // Get graphics queue handle
                Queue = Queues[QueueFamilyIndex];
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

            // Create command pool
            {
                VkCommandPoolCreateInfo commandPoolCreateInfo = {};
                commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                commandPoolCreateInfo.queueFamilyIndex = QueueFamilyIndex;

                VERIFY_RESULT( this, vkCreateCommandPool( Device, &commandPoolCreateInfo, nullptr, &CommandPool ) );
            }
        }

        inline void VerifyResult( VkResult result, const char* message )
        {
            if( result < 0 )
            {
                throw VulkanError( result, message );
            }
        }

        inline ~VulkanState()
        {
            vkDeviceWaitIdle( Device );

            // Destroy resources allocated for the test
            vkDestroyCommandPool( Device, CommandPool, nullptr );
            vkDestroyDescriptorPool( Device, DescriptorPool, nullptr );

            // This frees all resources created with this device
            vkDestroyDevice( Device, nullptr );
            Device = VK_NULL_HANDLE;
            QueueFamilyIndex = -1;
            Queue = VK_NULL_HANDLE;
            Queues.clear();
            CommandPool = VK_NULL_HANDLE;
            DescriptorPool = VK_NULL_HANDLE;

            // This frees all resources created with this instance
            vkDestroyInstance( Instance, nullptr );
            Instance = VK_NULL_HANDLE;
            PhysicalDevice = VK_NULL_HANDLE;
            PhysicalDeviceProperties = {};
            PhysicalDeviceMemoryProperties = {};
            PhysicalDeviceQueueProperties.clear();
            ApplicationInfo = {};
        }

        inline bool ApiCheck( uint32_t apiVersion ) const
        {
            return ( ApplicationInfo.apiVersion >= apiVersion ) &&
                   ( PhysicalDeviceProperties.apiVersion >= apiVersion );
        }

        inline VkQueue GetQueue( VkQueueFlags flags = VK_QUEUE_GRAPHICS_BIT ) const
        {
            const uint32_t queueCount = static_cast<uint32_t>( Queues.size() );
            for( uint32_t i = 0; i < queueCount; ++i )
            {
                if( ( PhysicalDeviceQueueProperties[i].queueFlags & flags ) == flags )
                {
                    return Queues[i];
                }
            }
            return VK_NULL_HANDLE;
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
                    continue;
                }

                extensions.push_back( pExtension->Name.c_str() );
                pExtension->Enabled = true;
            }

            return extensions;
        }

        template<typename T>
        static void ClearStructure( T& structure )
        {
            auto sType = structure.sType;
            auto* pNext = structure.pNext;
            memset( &structure, 0, sizeof( T ) );
            structure.sType = sType;
            structure.pNext = pNext;
        }
    };
}
