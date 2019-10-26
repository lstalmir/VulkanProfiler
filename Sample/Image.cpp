#include "Image.h"

namespace Sample
{
    Image::Image( Device& device,
        vk::Image image,
        vk::ImageLayout layout,
        vk::Format format,
        vk::Extent3D extent,
        vk::SampleCountFlagBits samples )
    {
        m_Device = device.m_Device;
        m_Image = image;
        m_Layout = layout;
        m_Format = format;
        m_Extent = extent;
        m_Samples = samples;
        m_Owns = false;
    }

    Image::~Image()
    {
    }

    void Image::layoutTransition(
        vk::CommandBuffer commandBuffer,
        vk::ImageLayout newLayout,
        vk::ImageSubresourceRange range )
    {
        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::DependencyFlags(),
            nullptr,
            nullptr,
            { vk::ImageMemoryBarrier(
                vk::AccessFlags(),
                vk::AccessFlags(),
                m_Layout,
                newLayout,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                m_Image,
                range ) } );

        m_Layout = newLayout;
    }

    void Image::destroy()
    {
        if( !m_Image )
        {
            // Already destroyed
            return;
        }

        if( m_Owns )
        {
            m_Device.destroyImage( m_Image );
        }

        m_Image = nullptr;
    }
}
