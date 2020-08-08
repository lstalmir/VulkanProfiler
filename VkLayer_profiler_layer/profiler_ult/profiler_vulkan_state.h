#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string_view>

#include <vk_dispatch_table_helper.h>

#include "profiler_layer_functions/VkInstance_functions.h"
#include "profiler_layer_functions/VkDevice_functions.h"

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

    class VulkanState
    {
    public:
        VkApplicationInfo           ApplicationInfo;
        VkInstance                  Instance;
        VkPhysicalDevice            PhysicalDevice;
        VkPhysicalDeviceProperties  PhysicalDeviceProperties;
        VkDevice                    Device;
        uint32_t                    QueueFamilyIndex;
        VkQueue                     Queue;
        VkCommandPool               CommandPool;
        VkDescriptorPool            DescriptorPool;

    public:
        inline VulkanState()
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

                VERIFY_RESULT( this, vkCreateInstance( &instanceCreateInfo, nullptr, &Instance ) );
            }

            // Select primary display device
            {
                uint32_t physicalDeviceCount = 1;
                VERIFY_RESULT( this, vkEnumeratePhysicalDevices( Instance, &physicalDeviceCount, &PhysicalDevice ) );

                // Get selected physical device properties
                vkGetPhysicalDeviceProperties( PhysicalDevice, &PhysicalDeviceProperties );
            }
            
            // Select graphics queue
            {
                uint32_t queueFamilyCount = 0;
                vkGetPhysicalDeviceQueueFamilyProperties( PhysicalDevice, &queueFamilyCount, nullptr );

                std::vector<VkQueueFamilyProperties> queueFamilyProperties( queueFamilyCount );
                vkGetPhysicalDeviceQueueFamilyProperties( PhysicalDevice, &queueFamilyCount, queueFamilyProperties.data() );

                for( uint32_t i = 0; i < queueFamilyCount; ++i )
                {
                    const VkQueueFamilyProperties& properties = queueFamilyProperties[ i ];
                    
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

                VERIFY_RESULT( this, vkCreateDevice( PhysicalDevice, &deviceCreateInfo, nullptr, &Device ) );

                // Get graphics queue handle
                vkGetDeviceQueue( Device, QueueFamilyIndex, 0, &Queue );
            }

            // Create command pool
            {
                VkCommandPoolCreateInfo commandPoolCreateInfo = {};
                commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                commandPoolCreateInfo.queueFamilyIndex = QueueFamilyIndex;

                VERIFY_RESULT( this, vkCreateCommandPool( Device, &commandPoolCreateInfo, nullptr, &CommandPool ) );
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
                layer_init_instance_dispatch_table( Instance, &id.Instance.Callbacks, vkGetInstanceProcAddr );

                VkDevice_Functions::Dispatch& dd = VkDevice_Functions::DeviceDispatch.Create( Device );
                dd.Device.Handle = Device;
                dd.Device.PhysicalDevice = PhysicalDevice;
                dd.Device.Properties = PhysicalDeviceProperties;
                dd.Device.pInstance = &id.Instance;
                layer_init_device_dispatch_table( Device, &dd.Device.Callbacks, vkGetDeviceProcAddr );

                VkQueue_Object queue = {};
                queue.Handle = Queue;
                queue.Family = QueueFamilyIndex;
                queue.Index = 0;
                queue.Flags = VK_QUEUE_GRAPHICS_BIT;
                dd.Device.Queues.emplace( Queue, queue );

                dd.Profiler.Initialize( &dd.Device, nullptr );
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

        inline VkLayerDispatchTable GetLayerDispatchTable() const
        {
            VkLayerDispatchTable dispatchTable = {};
            layer_init_device_dispatch_table( Device, &dispatchTable, VkDevice_Functions::GetDeviceProcAddr );
            return dispatchTable;
        }

        inline VkLayerInstanceDispatchTable GetLayerInstanceDispatchTable() const
        {
            VkLayerInstanceDispatchTable dispatchTable = {};
            layer_init_instance_dispatch_table( Instance, &dispatchTable, VkInstance_Functions::GetInstanceProcAddr );
            return dispatchTable;
        }
    };
}
