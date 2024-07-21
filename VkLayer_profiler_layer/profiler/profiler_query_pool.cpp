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

namespace Profiler
{
    TimestampQueryPoolData::TimestampQueryPoolData( DeviceProfiler& profiler, uint32_t commandBufferCount, uint32_t queryCount )
        : m_Profiler( profiler )
        , m_Buffer( VK_NULL_HANDLE )
        , m_Allocation( VK_NULL_HANDLE )
        , m_AllocationInfo()
        , m_pCpuAllocation( nullptr )
        , m_pCommandBufferOffsets( nullptr )
    {
        // Create the staging buffer.
        VkBufferCreateInfo bufferCreateInfo = {};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferCreateInfo.size = queryCount * sizeof( uint64_t );

        VmaAllocationCreateInfo allocationCreateInfo = {};
        allocationCreateInfo.flags =
            VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT;
        allocationCreateInfo.usage =
            VMA_MEMORY_USAGE_AUTO_PREFER_HOST;

        VkResult result = m_Profiler.m_MemoryManager.AllocateBuffer(
            bufferCreateInfo,
            allocationCreateInfo,
            &m_Buffer,
            &m_Allocation,
            &m_AllocationInfo );

        if( result != VK_SUCCESS )
        {
            // Fallback to CPU allocation.
            m_pCpuAllocation = malloc( bufferCreateInfo.size );

            if( m_pCpuAllocation != nullptr )
            {
                memset( &m_AllocationInfo, 0, sizeof( m_AllocationInfo ) );
                m_AllocationInfo.size = bufferCreateInfo.size;
                m_AllocationInfo.pMappedData = m_pCpuAllocation;
            }
        }

        // Allocate array of offsets to the first query of each submitted command buffer.
        m_pCommandBufferOffsets = static_cast<uint32_t*>(malloc( commandBufferCount * sizeof( uint32_t ) ));
    }

    TimestampQueryPoolData::~TimestampQueryPoolData()
    {
        if( m_Buffer != VK_NULL_HANDLE )
        {
            m_Profiler.m_MemoryManager.FreeBuffer(
                m_Buffer,
                m_Allocation );
        }

        if( m_pCpuAllocation )
        {
            free( m_pCpuAllocation );
        }

        free( m_pCommandBufferOffsets );
    }

    TimestampQueryPool::TimestampQueryPool( DeviceProfiler& profiler, uint32_t queryCount )
        : m_Profiler( profiler )
        , m_QueryPool( VK_NULL_HANDLE )
    {
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
    }

    void TimestampQueryPool::ResolveQueryDataGpu( VkCommandBuffer commandBuffer, TimestampQueryPoolData& dst, uint32_t dstOffset, uint32_t queryCount )
    {
        m_Profiler.m_pDevice->Callbacks.CmdCopyQueryPoolResults(
            commandBuffer,
            m_QueryPool,
            0, queryCount,
            dst.GetBufferHandle(),
            dstOffset, sizeof( uint64_t ),
            VK_QUERY_RESULT_64_BIT );
    }

    void TimestampQueryPool::ResolveQueryDataCpu( TimestampQueryPoolData& dst, uint32_t dstOffset, uint32_t queryCount )
    {
        m_Profiler.m_pDevice->Callbacks.GetQueryPoolResults(
            m_Profiler.m_pDevice->Handle,
            m_QueryPool,
            0, queryCount,
            queryCount * sizeof( uint64_t ),
            reinterpret_cast<std::byte*>(dst.GetCpuAllocation()) + dstOffset,
            sizeof( uint64_t ),
            VK_QUERY_RESULT_64_BIT );
    }
}
