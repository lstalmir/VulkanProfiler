// Copyright (c) 2019-2021 Lukasz Stalmirski
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

#include "profiler_command_pool.h"
#include "profiler.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        DeviceProfilerCommandPool

    Description:
        Constructor.

    \***********************************************************************************/
    DeviceProfilerCommandPool::DeviceProfilerCommandPool( DeviceProfiler& profiler, VkCommandPool commandPool, const VkCommandPoolCreateInfo& createInfo )
        : m_CommandPool( commandPool )
        , m_CommandQueueFlags( 0 )
    {
        // Get target command queue family properties
        const VkQueueFamilyProperties& queueFamilyProperties =
            profiler.m_pDevice->pPhysicalDevice->QueueFamilyProperties[ createInfo.queueFamilyIndex ];

        m_CommandQueueFlags = queueFamilyProperties.queueFlags;
    }

    /***********************************************************************************\

    Function:
        GetHandle

    Description:
        Get command pool handle.

    \***********************************************************************************/
    VkCommandPool DeviceProfilerCommandPool::GetHandle() const
    {
        return m_CommandPool;
    }

    /***********************************************************************************\

    Function:
        GetCommandQueueFlags

    Description:
        Get flags of target command queue.

    \***********************************************************************************/
    VkQueueFlags DeviceProfilerCommandPool::GetCommandQueueFlags() const
    {
        return m_CommandQueueFlags;
    }
}
