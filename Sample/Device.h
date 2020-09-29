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

#pragma once
#include <vulkan/vulkan.hpp>

#include "VkProfilerEXT.h"

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
            const std::vector<const char*>& extensions,
            const VkProfilerCreateInfoEXT* pProfilerCreateInfo = nullptr );

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
