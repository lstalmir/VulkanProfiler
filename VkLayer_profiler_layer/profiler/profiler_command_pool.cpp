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
        : m_Profiler( profiler )
        , m_CommandPool( commandPool )
        , m_CommandQueueFlags( 0 )
        , m_InternalCommandPool( VK_NULL_HANDLE )
    {
        // Get target command queue family properties
        const VkQueueFamilyProperties& queueFamilyProperties =
            m_Profiler.m_pDevice->pPhysicalDevice->QueueFamilyProperties[ createInfo.queueFamilyIndex ];

        m_CommandQueueFlags = queueFamilyProperties.queueFlags;

        // Create additional command pool for internal usage
        VkCommandPoolCreateInfo internalCommandPoolCreateInfo = {};
        internalCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        internalCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        internalCommandPoolCreateInfo.queueFamilyIndex = -1;
        
        // Find queue family index with graphics or compute bit
        for( const auto& queue : m_Profiler.m_pDevice->Queues )
        {
            if( (queue.second.Flags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) != 0 )
            {
                internalCommandPoolCreateInfo.queueFamilyIndex = queue.second.Family;
                break;
            }
        }

        // Application should create at least one queue with graphics or compute bit
        // (it doesn't have to, but it should)
        assert( internalCommandPoolCreateInfo.queueFamilyIndex != -1 );

        VkResult result = m_Profiler.m_pDevice->Callbacks.CreateCommandPool(
            m_Profiler.m_pDevice->Handle,
            &internalCommandPoolCreateInfo,
            nullptr,
            &m_InternalCommandPool );

        assert( result == VK_SUCCESS );
        assert( m_InternalCommandPool != VK_NULL_HANDLE );
    }

    /***********************************************************************************\

    Function:
        ~DeviceProfilerCommandPool

    Description:
        Destructor.

    \***********************************************************************************/
    DeviceProfilerCommandPool::~DeviceProfilerCommandPool()
    {
        // Destroy the internal command pool
        m_Profiler.m_pDevice->Callbacks.DestroyCommandPool(
            m_Profiler.m_pDevice->Handle,
            m_InternalCommandPool,
            nullptr );
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

    /***********************************************************************************\

    Function:
        GetInternalHandle

    Description:
        Get internal command pool associated with the application's command pool.

    \***********************************************************************************/
    VkCommandPool DeviceProfilerCommandPool::GetInternalHandle() const
    {
        return m_InternalCommandPool;
    }
}
