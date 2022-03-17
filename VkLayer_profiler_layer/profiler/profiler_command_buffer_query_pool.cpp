// Copyright (c) 2022 Lukasz Stalmirski
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

#include "profiler_command_buffer_query_pool.h"
#include "profiler.h"

#include <assert.h>

namespace Profiler
{
    CommandBufferQueryPool::CommandBufferQueryPool( DeviceProfiler& profiler, VkCommandBufferLevel level )
        : m_Profiler( profiler )
        , m_Device( *profiler.m_pDevice )
        , m_pQueryPools()
        , m_QueryPoolSize( 32768 )
        , m_CurrentQueryPoolIndex( 0 )
        , m_CurrentQueryIndex( UINT32_MAX )
        , m_PerformanceQueryPoolINTEL( VK_NULL_HANDLE )
    {
        // Initialize performance query once
        if( (level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) &&
            (m_Profiler.m_MetricsApiINTEL.IsAvailable()) )
        {
            VkQueryPoolCreateInfoINTEL intelCreateInfo = {};
            intelCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO_INTEL;
            intelCreateInfo.performanceCountersSampling = VK_QUERY_POOL_SAMPLING_MODE_MANUAL_INTEL;

            VkQueryPoolCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
            createInfo.pNext = &intelCreateInfo;
            createInfo.queryType = VK_QUERY_TYPE_PERFORMANCE_QUERY_INTEL;
            createInfo.queryCount = 1;

            m_Profiler.m_pDevice->Callbacks.CreateQueryPool(
                m_Profiler.m_pDevice->Handle,
                &createInfo,
                nullptr,
                &m_PerformanceQueryPoolINTEL );
        }
    }

    CommandBufferQueryPool::~CommandBufferQueryPool()
    {
        for( auto* pQueryPool : m_pQueryPools )
        {
            delete pQueryPool;
        }
    }
}
