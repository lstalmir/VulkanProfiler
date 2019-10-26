#pragma once
#include "Device.h"
#include <vulkan/vulkan.hpp>

namespace Sample
{
    struct Image
    {
        vk::Device                  m_Device;
        vk::Image                   m_Image;
        vk::ImageLayout             m_Layout;
        vk::Format                  m_Format;
        vk::Extent3D                m_Extent;
        vk::SampleCountFlagBits     m_Samples;
        bool                        m_Owns;

        Image( Device& device,
            vk::Image image,
            vk::ImageLayout layout,
            vk::Format format,
            vk::Extent3D extent,
            vk::SampleCountFlagBits samples );

        ~Image();

        void layoutTransition(
            vk::CommandBuffer commandBuffer,
            vk::ImageLayout newLayout,
            vk::ImageSubresourceRange range );

        void destroy();
    };
}
