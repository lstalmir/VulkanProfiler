#include "SwapChain.h"

namespace Sample
{
    SwapChain::SwapChain( Device& device, vk::SurfaceKHR surface, bool vsync )
    {
        m_Device = device.m_Device;
        m_Surface = surface;
        m_Acquired = false;
        m_AcquiredImageIndex = 0;

        // Check if the device supports presentation
        const bool presentSupported = device.m_PhysicalDevice.getSurfaceSupportKHR(
            device.m_QueueFamilyIndices.m_PresentQueueFamilyIndex,
            surface );

        if( !presentSupported )
        {
            throw std::runtime_error( "Selected physical device does not support presentation" );
        }

        vk::SurfaceCapabilitiesKHR surfaceCapabilities =
            device.m_PhysicalDevice.getSurfaceCapabilitiesKHR( surface );

        // Get the best supported image format
        const vk::SurfaceFormatKHR requrestedSurfaceFormat{
            vk::Format::eB8G8R8A8Unorm,
            vk::ColorSpaceKHR::eSrgbNonlinear };

        vk::SurfaceFormatKHR surfaceFormat{
            vk::Format::eUndefined,
            vk::ColorSpaceKHR::eSrgbNonlinear };

        std::vector<vk::SurfaceFormatKHR> surfaceFormats =
            device.m_PhysicalDevice.getSurfaceFormatsKHR( surface );

        if( surfaceFormats.size() == 1 &&
            surfaceFormats[0].format == vk::Format::eUndefined )
        {
            // All common image formats are supported
            surfaceFormat = requrestedSurfaceFormat;
        }
        else
        {
            // Fall back to the first supported format if the requested one is not supported
            if( surfaceFormats.size() > 0 )
            {
                surfaceFormat = surfaceFormats[0];
            }

            // Check if requested format is supported
            for( auto format : surfaceFormats )
            {
                if( format == requrestedSurfaceFormat )
                {
                    surfaceFormat = format;
                    break;
                }
            }
        }

        if( surfaceFormat.format == vk::Format::eUndefined )
        {
            // The surface does not support any formats
            throw std::runtime_error( "The surface does not support any presentation formats" );
        }

        // Get the best supported present mode
        vk::PresentModeKHR presentMode;

        for( auto mode : device.m_PhysicalDevice.getSurfacePresentModesKHR( surface ) )
        {
            // Mailbox: inserts images until the queue is full, replaces the last image
            //  when it is full.
            if( mode == vk::PresentModeKHR::eMailbox && !vsync )
            {
                presentMode = mode;
                break;
            }

            // Fifo: inserts images until the queue is full, blocks when it is full.
            if( mode == vk::PresentModeKHR::eFifo )
            {
                presentMode = mode;
                continue;
            }

            // Whatever, as long as it is supported...
            if( presentMode != vk::PresentModeKHR::eMailbox &&
                presentMode != vk::PresentModeKHR::eFifo )
            {
                presentMode = mode;
            }
        }

        // Get command queue index
        const std::vector<uint32_t> uniqueQueueFamilyIndices = {
            device.m_QueueFamilyIndices.m_PresentQueueFamilyIndex };

        // Create the swapchain
        m_Swapchain = m_Device.createSwapchainKHR(
            vk::SwapchainCreateInfoKHR(
                vk::SwapchainCreateFlagsKHR(),
                surface,
                surfaceCapabilities.minImageCount,
                surfaceFormat.format,
                surfaceFormat.colorSpace,
                surfaceCapabilities.currentExtent,
                1,
                vk::ImageUsageFlagBits::eColorAttachment,
                vk::SharingMode::eExclusive,
                static_cast<uint32_t>(uniqueQueueFamilyIndices.size()),
                uniqueQueueFamilyIndices.data(),
                surfaceCapabilities.currentTransform,
                vk::CompositeAlphaFlagBitsKHR::eOpaque,
                presentMode,
                false ) );

        m_Format = surfaceFormat.format;
        m_Extent = surfaceCapabilities.currentExtent;

        // Prepare swapchain images for rendering
        for( auto image : m_Device.getSwapchainImagesKHR( m_Swapchain ) )
        {
            m_Images.push_back( Image(
                device,
                image,
                vk::ImageLayout::eUndefined,
                surfaceFormat.format,
                vk::Extent3D( surfaceCapabilities.currentExtent ),
                vk::SampleCountFlagBits::e1 ) );

            m_ImageAvailableSemaphores.push_back(
                m_Device.createSemaphore( vk::SemaphoreCreateInfo() ) );

            m_ImageRenderedSemaphores.push_back(
                m_Device.createSemaphore( vk::SemaphoreCreateInfo() ) );
        }

        m_Viewport = vk::Viewport( 0, 0, m_Extent.width, m_Extent.height, 0, 1 );
        m_ScissorRect = vk::Rect2D( vk::Offset2D(), m_Extent );
    }

    SwapChain::~SwapChain()
    {
        destroy();
    }

    void SwapChain::acquireNextImage()
    {
        if( m_Acquired )
        {
            // Do not acquire next image if current one hasn't been used yet
            return;
        }

        // We don't know which image will be acquired beforehand, remember which semaphore
        // will be signalled next.
        m_NextImageAvailableSemaphore = m_ImageAvailableSemaphores[m_AcquiredImageIndex];

        m_AcquiredImageIndex = m_Device.acquireNextImageKHR(
            m_Swapchain, UINT64_MAX, m_NextImageAvailableSemaphore, nullptr ).value;

        m_Acquired = true;
    }

    void SwapChain::destroy()
    {
        if( !m_Swapchain )
        {
            // Already released
            return;
        }

        m_Device.waitIdle();

        for( auto semaphore : m_ImageAvailableSemaphores )
        {
            m_Device.destroySemaphore( semaphore );
        }
        m_ImageAvailableSemaphores.clear();

        for( auto semaphore : m_ImageRenderedSemaphores )
        {
            m_Device.destroySemaphore( semaphore );
        }
        m_ImageRenderedSemaphores.clear();

        for( auto image : m_Images )
        {
            image.destroy();
        }
        m_Images.clear();

        m_Device.destroySwapchainKHR( m_Swapchain );
        m_Swapchain = nullptr;
    }
}
