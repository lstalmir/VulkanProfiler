// Copyright (c) 2024 Lukasz Stalmirski
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
#include "profiler_test_icd_base.h"
#include "profiler_test_icd_helpers.h"
#include "profiler_test_image.h"

namespace Profiler::ICD
{
    struct Swapchain
    {
        VkImage m_Image;

        Swapchain( const VkSwapchainCreateInfoKHR& createInfo )
            : m_Image( nullptr )
        {
            VkImageCreateInfo imageCreateInfo = {};
            imageCreateInfo.extent = { createInfo.imageExtent.width, createInfo.imageExtent.height, 1 };
            imageCreateInfo.format = createInfo.imageFormat;
            imageCreateInfo.usage = createInfo.imageUsage;
            imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCreateInfo.mipLevels = 1;
            imageCreateInfo.arrayLayers = 1;
            vk_check( vk_new_nondispatchable<VkImage_T>( &m_Image, imageCreateInfo ) );
        }

        ~Swapchain()
        {
            delete m_Image;
        }
    };
}

struct VkSwapchainKHR_T : Profiler::ICD::Swapchain
{
    using Swapchain::Swapchain;
};
