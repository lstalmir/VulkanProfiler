#include "SwapChain.h"

namespace Sample
{
    SwapChain::SwapChain( Device& device, vk::SurfaceKHR surface, bool vsync )
    {
        m_pDevice = &device;
        m_Swapchain = nullptr;
        m_Surface = surface;
        m_VSync = vsync;
        m_Acquired = false;
        m_AcquiredImageIndex = 0;
        recreate();
    }

    SwapChain::~SwapChain()
    {
        destroy();
    }

    vk::Result SwapChain::acquireNextImage()
    {
        // We don't know which image will be acquired beforehand, remember which semaphore
        // will be signalled next.
        m_NextImageAvailableSemaphore = m_ImageAvailableSemaphores[m_AcquiredImageIndex];

        auto resultValue = m_pDevice->m_Device.acquireNextImageKHR(
            m_Swapchain, UINT64_MAX, m_NextImageAvailableSemaphore, nullptr );

        m_AcquiredImageIndex = resultValue.value;

        return resultValue.result;
    }

    void SwapChain::recreate()
    {
        // Check if the device supports presentation
        const bool presentSupported = m_pDevice->m_PhysicalDevice.getSurfaceSupportKHR(
            m_pDevice->m_QueueFamilyIndices.m_PresentQueueFamilyIndex,
            m_Surface );

        if( !presentSupported )
        {
            throw std::runtime_error( "Selected physical device does not support presentation" );
        }

        vk::SurfaceCapabilitiesKHR surfaceCapabilities =
            m_pDevice->m_PhysicalDevice.getSurfaceCapabilitiesKHR( m_Surface );

        // Get the best supported image format
        const vk::SurfaceFormatKHR requrestedSurfaceFormat{
            vk::Format::eB8G8R8A8Unorm,
            vk::ColorSpaceKHR::eSrgbNonlinear };

        vk::SurfaceFormatKHR surfaceFormat{
            vk::Format::eUndefined,
            vk::ColorSpaceKHR::eSrgbNonlinear };

        std::vector<vk::SurfaceFormatKHR> surfaceFormats =
            m_pDevice->m_PhysicalDevice.getSurfaceFormatsKHR( m_Surface );

        if( surfaceFormats.size() == 1 &&
            surfaceFormats[ 0 ].format == vk::Format::eUndefined )
        {
            // All common image formats are supported
            surfaceFormat = requrestedSurfaceFormat;
        }
        else
        {
            // Fall back to the first supported format if the requested one is not supported
            if( surfaceFormats.size() > 0 )
            {
                surfaceFormat = surfaceFormats[ 0 ];
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
        vk::PresentModeKHR presentMode = vk::PresentModeKHR::eImmediate;

        for( auto mode : m_pDevice->m_PhysicalDevice.getSurfacePresentModesKHR( m_Surface ) )
        {
            // Mailbox: inserts images until the queue is full, replaces the last image
            //  when it is full.
            if( mode == vk::PresentModeKHR::eMailbox && !m_VSync )
            {
                presentMode = mode;
                break;
            }

            // Fifo: inserts images until the queue is full, blocks when it is full.
            if( mode == vk::PresentModeKHR::eFifo && m_VSync )
            {
                presentMode = mode;
                break;
            }
        }

        // Get command queue index
        const std::vector<uint32_t> uniqueQueueFamilyIndices = {
            m_pDevice->m_QueueFamilyIndices.m_PresentQueueFamilyIndex };

        vk::SwapchainKHR oldSwapchain = m_Swapchain;

        // Create the swapchain
        m_Swapchain = m_pDevice->m_Device.createSwapchainKHR(
            vk::SwapchainCreateInfoKHR(
                vk::SwapchainCreateFlagsKHR(),
                m_Surface,
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
                false,
                oldSwapchain ) );

        if( !m_Swapchain && oldSwapchain )
        {
            m_Swapchain = oldSwapchain;
            return;
        }

        if( oldSwapchain )
        {
            m_pDevice->m_Device.destroySwapchainKHR( oldSwapchain );

            for( auto semaphore : m_ImageAvailableSemaphores )
            {
                m_pDevice->m_Device.destroySemaphore( semaphore );
            }

            for( auto semaphore : m_ImageRenderedSemaphores )
            {
                m_pDevice->m_Device.destroySemaphore( semaphore );
            }

            m_ImageAvailableSemaphores.clear();
            m_ImageRenderedSemaphores.clear();
            m_Images.clear();
        }

        m_Format = surfaceFormat.format;
        m_Extent = surfaceCapabilities.currentExtent;

        // Prepare swapchain images for rendering
        for( auto image : m_pDevice->m_Device.getSwapchainImagesKHR( m_Swapchain ) )
        {
            m_Images.push_back( Image(
                *m_pDevice,
                image,
                vk::ImageLayout::eUndefined,
                surfaceFormat.format,
                vk::Extent3D( surfaceCapabilities.currentExtent ),
                vk::SampleCountFlagBits::e1 ) );

            m_ImageAvailableSemaphores.push_back(
                m_pDevice->m_Device.createSemaphore( vk::SemaphoreCreateInfo() ) );

            m_ImageRenderedSemaphores.push_back(
                m_pDevice->m_Device.createSemaphore( vk::SemaphoreCreateInfo() ) );
        }

        m_Viewport = vk::Viewport( 0, 0, m_Extent.width, m_Extent.height, 0, 1 );
        m_ScissorRect = vk::Rect2D( vk::Offset2D(), m_Extent );
    }

    void SwapChain::destroy()
    {
        if( !m_Swapchain )
        {
            // Already released
            return;
        }

        m_pDevice->m_Device.waitIdle();

        for( auto semaphore : m_ImageAvailableSemaphores )
        {
            m_pDevice->m_Device.destroySemaphore( semaphore );
        }
        m_ImageAvailableSemaphores.clear();

        for( auto semaphore : m_ImageRenderedSemaphores )
        {
            m_pDevice->m_Device.destroySemaphore( semaphore );
        }
        m_ImageRenderedSemaphores.clear();

        for( auto image : m_Images )
        {
            image.destroy();
        }
        m_Images.clear();

        m_pDevice->m_Device.destroySwapchainKHR( m_Swapchain );
        m_Swapchain = nullptr;
    }
}
