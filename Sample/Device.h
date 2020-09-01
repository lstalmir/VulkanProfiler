#pragma once
#include <vulkan/vulkan.hpp>

namespace Sample
{
    struct Device
    {
        struct QueueFamilyIndices
        {
            uint32_t                            m_GraphicsQueueFamilyIndex;
            uint32_t                            m_PresentQueueFamilyIndex;
        };

        QueueFamilyIndices                      m_QueueFamilyIndices;
        vk::PhysicalDevice                      m_PhysicalDevice;
        vk::PhysicalDeviceProperties            m_PhysicalDeviceProperties;
        vk::Device                              m_Device;
        vk::Queue                               m_GraphicsQueue;
        vk::Queue                               m_PresentQueue;

        Device( vk::Instance instance,
            vk::SurfaceKHR surface,
            const std::vector<const char*>& layers,
            const std::vector<const char*>& extensions );

        ~Device();

        void destroy();

        vk::Pipeline createGraphicsPipeline(
            vk::PipelineLayout layout,
            vk::RenderPass renderPass,
            const std::vector<vk::PipelineShaderStageCreateInfo>& shaderStages,
            const vk::PipelineVertexInputStateCreateInfo& vertexState,
            const vk::PipelineInputAssemblyStateCreateInfo& inputAssemblyState,
            const vk::PipelineViewportStateCreateInfo& viewportState,
            const vk::PipelineRasterizationStateCreateInfo& rasterizerState,
            const vk::PipelineMultisampleStateCreateInfo& multisampleState,
            const vk::PipelineDepthStencilStateCreateInfo& depthStencilState,
            const vk::PipelineColorBlendStateCreateInfo& colorBlendState );

    private:
        QueueFamilyIndices getPhysicalDeviceQueueFamilyIndices(
            vk::PhysicalDevice device,
            vk::SurfaceKHR surface );

        std::vector<vk::DeviceQueueCreateInfo> getQueueCreateInfos(
            QueueFamilyIndices indices );
    };
}
