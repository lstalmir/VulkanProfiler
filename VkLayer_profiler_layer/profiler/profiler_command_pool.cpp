// Copyright (c) 2019-2025 Lukasz Stalmirski
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
        , m_QueueFamilyIndex( createInfo.queueFamilyIndex )
        , m_SupportsTimestampQuery( false )
    {
        // Get target command queue family properties
        const VkQueueFamilyProperties& queueFamilyProperties =
            profiler.m_pDevice->pPhysicalDevice->QueueFamilyProperties[ createInfo.queueFamilyIndex ];

        // Profile the command buffer only if it will be submitted to the queue supporting graphics or compute commands
        // This is requirement of vkCmdResetQueryPool (VUID-vkCmdResetQueryPool-commandBuffer-cmdpool)
        const bool canResetQueryPool =
            (queueFamilyProperties.queueFlags & (
                VK_QUEUE_GRAPHICS_BIT |
                VK_QUEUE_COMPUTE_BIT |
                VK_QUEUE_VIDEO_DECODE_BIT_KHR |
                VK_QUEUE_VIDEO_ENCODE_BIT_KHR ));

        // Profile only queues that support timestamp queries (VUID-vkCmdWriteTimestamp-timestampValidBits-00829)
        if( canResetQueryPool && queueFamilyProperties.timestampValidBits != 0 )
        {
            m_SupportsTimestampQuery = true;
        }
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
        GetQueueFamilyIndex

    Description:
        Get command pool's target queue family index.

    \***********************************************************************************/
    uint32_t DeviceProfilerCommandPool::GetQueueFamilyIndex() const
    {
        return m_QueueFamilyIndex;
    }

    /***********************************************************************************\

    Function:
        GetCommandQueueFlags

    Description:
        Get flags of target command queue.

    \***********************************************************************************/
    bool DeviceProfilerCommandPool::SupportsTimestampQuery() const
    {
        return m_SupportsTimestampQuery;
    }

    /***********************************************************************************\

    Function:
        DeviceProfilerInternalCommandPool

    Description:
        Constructor.

    \***********************************************************************************/
    DeviceProfilerInternalCommandPool::DeviceProfilerInternalCommandPool( DeviceProfiler& profiler, VkCommandPool commandPool, const VkCommandPoolCreateInfo& createInfo )
        : DeviceProfilerCommandPool( profiler, commandPool, createInfo )
        , m_Profiler( profiler )
    {
    }

    /***********************************************************************************\

    Function:
        ~DeviceProfilerInternalCommandPool

    Description:
        Destructor.

    \***********************************************************************************/
    DeviceProfilerInternalCommandPool::~DeviceProfilerInternalCommandPool()
    {
        // Destroy command pool
        m_Profiler.m_pDevice->Callbacks.DestroyCommandPool( m_Profiler.m_pDevice->Handle, GetHandle(), nullptr );
    }

    /***********************************************************************************\

    Function:
        GetMutex

    Description:
        Returns the mutex for synchronizing access to the command pool.

    \***********************************************************************************/
    std::mutex& DeviceProfilerInternalCommandPool::GetMutex()
    {
        return m_Mutex;
    }
}
