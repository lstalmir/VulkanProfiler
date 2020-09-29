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
#include "Device.h"
#include "Image.h"
#include <vulkan/vulkan.hpp>

namespace Sample
{
    struct SwapChain
    {
        Device*                     m_pDevice;
        vk::SwapchainKHR            m_Swapchain;
        vk::SurfaceKHR              m_Surface;
        vk::Extent2D                m_Extent;
        vk::Format                  m_Format;
        std::vector<Image>          m_Images;
        std::vector<vk::Semaphore>  m_ImageAvailableSemaphores;
        std::vector<vk::Semaphore>  m_ImageRenderedSemaphores;
        vk::Semaphore               m_NextImageAvailableSemaphore;
        vk::Viewport                m_Viewport;
        vk::Rect2D                  m_ScissorRect;
        bool                        m_VSync;
        bool                        m_Acquired;
        uint32_t                    m_AcquiredImageIndex;
        
        SwapChain( Device& device, vk::SurfaceKHR surface, bool vsync = false );

        ~SwapChain();

        vk::Result acquireNextImage();

        void recreate();

        void destroy();
    };
}
