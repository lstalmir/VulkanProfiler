#pragma once
#include "Device.h"
#include "Image.h"
#include <vulkan/vulkan.hpp>

namespace Sample
{
    struct SwapChain
    {
        vk::Device                  m_Device;
        vk::SwapchainKHR            m_Swapchain;
        vk::SurfaceKHR              m_Surface;
        vk::Extent2D                m_Extent;
        vk::Format                  m_Format;
        std::vector<Image>          m_Images;
        std::vector<vk::Semaphore>  m_ImageAvailableSemaphores;
        std::vector<vk::Semaphore>  m_ImageRenderedSemaphores;
        vk::Semaphore               m_NextImageAvailableSemaphore;
        bool                        m_Acquired;
        uint32_t                    m_AcquiredImageIndex;
        
        SwapChain( Device& device, vk::SurfaceKHR surface, bool vsync = false );

        ~SwapChain();

        void acquireNextImage();

        void destroy();
    };
}
