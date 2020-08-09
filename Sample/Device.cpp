#include "Device.h"
#include <set>

namespace Sample
{
    Device::Device( vk::Instance instance,
        vk::SurfaceKHR surface,
        const std::vector<const char*>& layers,
        const std::vector<const char*>& extensions )
    {
        float bestSuitability = 0.f;

        for( auto physicalDevice : instance.enumeratePhysicalDevices() )
        {
            float suitability = getPhysicalDeviceSuitability( physicalDevice, surface, layers, extensions );

            if( suitability > bestSuitability )
            {
                bestSuitability = suitability;
                m_PhysicalDevice = physicalDevice;
            }
        }

        if( !m_PhysicalDevice )
        {
            throw std::runtime_error( "Could not find suitable GPU device" );
        }

        m_QueueFamilyIndices = getPhysicalDeviceQueueFamilyIndices( m_PhysicalDevice, surface );
        m_PhysicalDeviceProperties = m_PhysicalDevice.getProperties();

        auto queues = getQueueCreateInfos( m_QueueFamilyIndices );

        m_Device = m_PhysicalDevice.createDevice(
            vk::DeviceCreateInfo(
                vk::DeviceCreateFlags(),
                static_cast<uint32_t>(queues.size()), queues.data(),
                static_cast<uint32_t>(layers.size()), layers.data(),
                static_cast<uint32_t>(extensions.size()), extensions.data(),
                nullptr ) );

        m_GraphicsQueue = m_Device.getQueue( m_QueueFamilyIndices.m_GraphicsQueueFamilyIndex, 0 );
        m_PresentQueue = m_Device.getQueue( m_QueueFamilyIndices.m_PresentQueueFamilyIndex, 0 );
    }

    Device::~Device()
    {
        destroy();
    }

    void Device::destroy()
    {
        if( !m_Device )
        {
            // Already destroyed
            return;
        }

        m_Device.destroy();
        m_Device = nullptr;
    }

    vk::Pipeline Device::createGraphicsPipeline(
        vk::PipelineLayout layout,
        vk::RenderPass renderPass,
        const std::vector<vk::PipelineShaderStageCreateInfo>& shaderStages,
        const vk::PipelineVertexInputStateCreateInfo& vertexState,
        const vk::PipelineInputAssemblyStateCreateInfo& inputAssemblyState,
        const vk::PipelineViewportStateCreateInfo& viewportState,
        const vk::PipelineRasterizationStateCreateInfo& rasterizerState,
        const vk::PipelineMultisampleStateCreateInfo& multisampleState,
        const vk::PipelineDepthStencilStateCreateInfo& depthStencilState,
        const vk::PipelineColorBlendStateCreateInfo& colorBlendState )
    {
        return m_Device.createGraphicsPipeline(
            nullptr,
            vk::GraphicsPipelineCreateInfo()
            .setStageCount( shaderStages.size() )
            .setPStages( shaderStages.data() )
            .setPVertexInputState( &vertexState )
            .setPInputAssemblyState( &inputAssemblyState )
            .setPViewportState( &viewportState )
            .setPRasterizationState( &rasterizerState )
            .setPMultisampleState( &multisampleState )
            .setPDepthStencilState( &depthStencilState )
            .setPColorBlendState( &colorBlendState )
            .setLayout( layout )
            .setRenderPass( renderPass ) )
            #if VK_HEADER_VERSION >= 136
            .value // Return type of createGraphicsPipeline has changed to (VkResult,VkPipeline) pair
            #endif
            ;
    }

    float Device::getPhysicalDeviceSuitability(
        vk::PhysicalDevice device,
        vk::SurfaceKHR surface,
        const std::vector<const char*>& layers,
        const std::vector<const char*>& extensions )
    {
        float suitability = 1.f;

        // Find required queue family indices
        auto queueFamilyIndices = getPhysicalDeviceQueueFamilyIndices( device, surface );

        // Check if device supports required queue types
        if( queueFamilyIndices.m_GraphicsQueueFamilyIndex == ~0 || queueFamilyIndices.m_PresentQueueFamilyIndex == ~0 )
        {
            suitability *= 0.f;
        }

        // Give bonus points for supporting both graphics and presentation commands in the same queue
        if( queueFamilyIndices.m_GraphicsQueueFamilyIndex == queueFamilyIndices.m_PresentQueueFamilyIndex )
        {
            suitability *= 2.f;
        }

        // Give bonus points for GPU type
        auto properties = device.getProperties();

        switch( properties.deviceType )
        {
        case vk::PhysicalDeviceType::eDiscreteGpu: suitability *= 2.f; break;
        case vk::PhysicalDeviceType::eIntegratedGpu: suitability *= 1.f; break;
        case vk::PhysicalDeviceType::eVirtualGpu: suitability *= 0.8f; break;
        case vk::PhysicalDeviceType::eCpu: suitability *= 0.5f; break;
        case vk::PhysicalDeviceType::eOther: suitability *= 0.2f; break;
        }

        // Check support of required device extensions
        std::set<std::string> _Extensions( extensions.cbegin(), extensions.cend() );

        for( auto extensionProperties : device.enumerateDeviceExtensionProperties() )
        {
            _Extensions.erase( (const char*)extensionProperties.extensionName );
        }

        // Check if all extensions are available
        if( !_Extensions.empty() )
        {
            suitability *= 0.f;
        }

        return suitability;
    }

    Device::QueueFamilyIndices Device::getPhysicalDeviceQueueFamilyIndices(
        vk::PhysicalDevice device,
        vk::SurfaceKHR surface )
    {
        QueueFamilyIndices indices;
        memset( &indices, 0xFF, sizeof( indices ) );

        uint32_t queueFamilyIndex = 0;

        for( auto queueFamilyProperties : device.getQueueFamilyProperties() )
        {
            const bool supportsGraphics = !!(queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics);

            // Check if the queue supports presentation to the surface
            const bool supportsPresent = device.getSurfaceSupportKHR( queueFamilyIndex, surface );

            // Check if any queue of this family may be created
            if( queueFamilyProperties.queueCount > 0 )
            {
                if( supportsPresent && supportsGraphics )
                {
                    // This is the best queue, supports both drawing and presenting
                    indices.m_GraphicsQueueFamilyIndex = queueFamilyIndex;
                    indices.m_PresentQueueFamilyIndex = queueFamilyIndex;
                    break;
                }

                if( indices.m_GraphicsQueueFamilyIndex == ~0 && supportsGraphics )
                {
                    // Queue supports graphics commands
                    indices.m_GraphicsQueueFamilyIndex = queueFamilyIndex;
                }

                if( indices.m_PresentQueueFamilyIndex == ~0 && supportsPresent )
                {
                    // Queue supports present command
                    indices.m_PresentQueueFamilyIndex = queueFamilyIndex;
                }
            }

            queueFamilyIndex++;
        }

        return indices;
    }

    std::vector<vk::DeviceQueueCreateInfo> Device::getQueueCreateInfos(
        Device::QueueFamilyIndices indices )
    {
        std::vector<vk::DeviceQueueCreateInfo> createInfos;

        std::set<uint32_t> uniqueQueueFamilyIndices = {
            indices.m_GraphicsQueueFamilyIndex,
            indices.m_PresentQueueFamilyIndex };

        // static, the variable must be accessible from outside of this function
        static const float priority = 1.f;

        for( auto queueFamilyIndex : uniqueQueueFamilyIndices )
        {
            createInfos.push_back( vk::DeviceQueueCreateInfo(
                vk::DeviceQueueCreateFlags(), queueFamilyIndex, 1, &priority ) );
        }

        return createInfos;
    }
}
