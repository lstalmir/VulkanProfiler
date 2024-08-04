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

#include "profiler_query_pool.h"
#include "profiler_memory_manager.h"
#include "profiler.h"

#define VKPROF_USE_GPU_TIMESTAMP_BUFFER 0

namespace Profiler
{
    TimestampQueryPool::TimestampQueryPool( DeviceProfiler& profiler, uint32_t queryCount )
        : m_Profiler( profiler )
        , m_QueryPool( VK_NULL_HANDLE )
        , m_QueryResultsBuffer( VK_NULL_HANDLE )
        , m_QueryResultsBufferAllocation( { nullptr } )
    {
        TipGuardDbg tip( m_Profiler.m_pDevice->TIP, __func__ );

        // Create the command pool.
        VkQueryPoolCreateInfo queryPoolCreateInfo = {};
        queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queryPoolCreateInfo.queryCount = queryCount;

        m_Profiler.m_pDevice->Callbacks.CreateQueryPool(
            m_Profiler.m_pDevice->Handle,
            &queryPoolCreateInfo,
            nullptr,
            &m_QueryPool );

#if VKPROF_USE_GPU_TIMESTAMP_BUFFER
        // Create the staging buffer.
        VkBufferCreateInfo bufferCreateInfo = {};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferCreateInfo.size = queryCount * sizeof( uint64_t );

        m_Profiler.m_pDevice->Callbacks.CreateBuffer(
            m_Profiler.m_pDevice->Handle,
            &bufferCreateInfo,
            nullptr,
            &m_QueryResultsBuffer );

        // Allocate memory for the buffer.
        VkMemoryRequirements bufferMemoryRequirements;
        m_Profiler.m_pDevice->Callbacks.GetBufferMemoryRequirements(
            m_Profiler.m_pDevice->Handle,
            m_QueryResultsBuffer,
            &bufferMemoryRequirements );

        m_Profiler.m_MemoryManager.AllocateMemory(
            bufferMemoryRequirements,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            &m_QueryResultsBufferAllocation );

        m_Profiler.m_pDevice->Callbacks.BindBufferMemory(
            m_Profiler.m_pDevice->Handle,
            m_QueryResultsBuffer,
            m_QueryResultsBufferAllocation.m_pPool->m_DeviceMemory,
            m_QueryResultsBufferAllocation.m_Offset );
#else
        m_QueryResultsBufferAllocation.m_Size = sizeof( uint64_t ) * queryCount;
        m_QueryResultsBufferAllocation.m_pMappedMemory =
            malloc( m_QueryResultsBufferAllocation.m_Size );
#endif
    }

    TimestampQueryPool::~TimestampQueryPool()
    {
        if( m_QueryPool != VK_NULL_HANDLE )
        {
            m_Profiler.m_pDevice->Callbacks.DestroyQueryPool(
                m_Profiler.m_pDevice->Handle,
                m_QueryPool,
                nullptr );
        }

#if VKPROF_USE_GPU_TIMESTAMP_BUFFER
        if( m_QueryResultsBuffer != VK_NULL_HANDLE )
        {
            m_Profiler.m_pDevice->Callbacks.DestroyBuffer(
                m_Profiler.m_pDevice->Handle,
                m_QueryResultsBuffer,
                nullptr );
        }

        if( m_QueryResultsBufferAllocation.m_pPool != nullptr )
        {
            m_Profiler.m_MemoryManager.FreeMemory( &m_QueryResultsBufferAllocation );
        }
#else
        free( m_QueryResultsBufferAllocation.m_pMappedMemory );
#endif
    }

    void TimestampQueryPool::ResolveQueryDataGpu( VkCommandBuffer commandBuffer, uint32_t queryCount )
    {
        TipGuardDbg tip( m_Profiler.m_pDevice->TIP, __func__ );

#if VKPROF_USE_GPU_TIMESTAMP_BUFFER
        m_Profiler.m_pDevice->Callbacks.CmdCopyQueryPoolResults(
            commandBuffer,
            m_QueryPool,
            0, queryCount,
            m_QueryResultsBuffer,
            0, sizeof( uint64_t ),
            VK_QUERY_RESULT_64_BIT );
#endif
    }

    void TimestampQueryPool::ResolveQueryDataCpu( uint32_t queryCount )
    {
        TipGuardDbg tip( m_Profiler.m_pDevice->TIP, __func__ );

#if !VKPROF_USE_GPU_TIMESTAMP_BUFFER
        m_Profiler.m_pDevice->Callbacks.GetQueryPoolResults(
            m_Profiler.m_pDevice->Handle,
            m_QueryPool,
            0, queryCount,
            m_QueryResultsBufferAllocation.m_Size,
            m_QueryResultsBufferAllocation.m_pMappedMemory,
            sizeof( uint64_t ),
            VK_QUERY_RESULT_64_BIT );
#endif
    }
}
