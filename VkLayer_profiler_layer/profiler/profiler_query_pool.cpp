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
    DeviceProfilerQueryDataBuffer::DeviceProfilerQueryDataBuffer( DeviceProfiler& profiler, uint64_t size )
        : m_Profiler( profiler )
        , m_Buffer( VK_NULL_HANDLE )
        , m_Allocation( VK_NULL_HANDLE )
        , m_AllocationInfo()
        , m_pCpuAllocation( nullptr )
        , m_Contexts()
    {
        // Create the staging buffer.
        VkBufferCreateInfo bufferCreateInfo = {};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferCreateInfo.size = size;

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
            memset( &m_AllocationInfo, 0, sizeof( m_AllocationInfo ) );

            // Fallback to CPU allocation.
            m_pCpuAllocation = malloc( bufferCreateInfo.size );

            if( m_pCpuAllocation != nullptr )
            {
                m_AllocationInfo.size = bufferCreateInfo.size;
                m_AllocationInfo.pMappedData = m_pCpuAllocation;
            }
        }
    }

    DeviceProfilerQueryDataBuffer::~DeviceProfilerQueryDataBuffer()
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
    }

    void DeviceProfilerQueryDataBuffer::FallbackToCpuAllocation()
    {
        if( m_Buffer != VK_NULL_HANDLE )
        {
            size_t bufferSize = m_AllocationInfo.size;
            m_Profiler.m_MemoryManager.FreeBuffer(
                m_Buffer,
                m_Allocation );
            memset( &m_AllocationInfo, 0, sizeof( m_AllocationInfo ) );

            m_pCpuAllocation = malloc( bufferSize );

            if( m_pCpuAllocation != nullptr )
            {
                m_AllocationInfo.size = bufferSize;
                m_AllocationInfo.pMappedData = m_pCpuAllocation;
            }
        }
    }

    bool DeviceProfilerQueryDataBuffer::UsesGpuAllocation() const
    {
        return m_Buffer != VK_NULL_HANDLE;
    }

    VkBuffer DeviceProfilerQueryDataBuffer::GetGpuBuffer() const
    {
        return m_Buffer;
    }

    uint8_t* DeviceProfilerQueryDataBuffer::GetCpuBuffer() const
    {
        return reinterpret_cast<uint8_t*>(m_pCpuAllocation);
    }

    const uint8_t* DeviceProfilerQueryDataBuffer::GetMappedData() const
    {
        return reinterpret_cast<const uint8_t*>(m_AllocationInfo.pMappedData);
    }

    DeviceProfilerQueryDataContext* DeviceProfilerQueryDataBuffer::CreateContext( const void* handle )
    {
        return &m_Contexts.emplace( handle, DeviceProfilerQueryDataContext() ).first->second;
    }

    const DeviceProfilerQueryDataContext* DeviceProfilerQueryDataBuffer::GetContext( const void* handle ) const
    {
        auto contextIt = m_Contexts.find( handle );
        if( contextIt != m_Contexts.cend() )
        {
            return &contextIt->second;
        }
        return nullptr;
    }

    DeviceProfilerQueryDataBufferWriter::DeviceProfilerQueryDataBufferWriter(
        DeviceProfiler& profiler,
        DeviceProfilerQueryDataBuffer& dataBuffer,
        VkCommandBuffer copyCommandBuffer )
        : m_pProfiler( &profiler )
        , m_pData( &dataBuffer )
        , m_pContext( nullptr )
        , m_CommandBuffer( copyCommandBuffer )
        , m_DataOffset( 0 )
    {
        if( m_CommandBuffer != VK_NULL_HANDLE )
        {
            m_GpuBuffer = m_pData->GetGpuBuffer();
        }
        else
        {
            m_CpuBuffer = m_pData->GetCpuBuffer();
        }
    }

    void DeviceProfilerQueryDataBufferWriter::SetContext( const void* handle )
    {
        m_pContext = m_pData->CreateContext( handle );
        m_pContext->m_TimestampDataOffset = m_DataOffset;
    }

    void DeviceProfilerQueryDataBufferWriter::WriteTimestampQueryResults( VkQueryPool queryPool, uint32_t queryCount )
    {
        uint32_t dataSize = (queryCount * sizeof( uint64_t ));

        if( m_CommandBuffer != VK_NULL_HANDLE )
        {
            m_pProfiler->m_pDevice->Callbacks.CmdCopyQueryPoolResults(
                m_CommandBuffer,
                queryPool,
                0, queryCount,
                m_GpuBuffer,
                m_DataOffset,
                sizeof( uint64_t ),
                VK_QUERY_RESULT_64_BIT );
        }
        else
        {
            m_pProfiler->m_pDevice->Callbacks.GetQueryPoolResults(
                m_pProfiler->m_pDevice->Handle,
                queryPool,
                0, queryCount,
                dataSize,
                m_CpuBuffer + m_DataOffset,
                sizeof( uint64_t ),
                VK_QUERY_RESULT_64_BIT );
        }

        m_pContext->m_TimestampDataSize += dataSize;
        m_DataOffset += dataSize;
    }

    void DeviceProfilerQueryDataBufferWriter::WritePerformanceQueryResults( VkQueryPool queryPool, uint32_t metricsSetIndex )
    {
        uint32_t dataSize = m_pProfiler->m_MetricsApiINTEL.GetReportSize( metricsSetIndex );
        m_pContext->m_PerformanceDataSize = dataSize;
        m_pContext->m_PerformanceDataMetricsSetIndex = metricsSetIndex;
        m_pContext->m_PerformanceQueryPool = queryPool;
        m_DataOffset += dataSize;
    }

    DeviceProfilerQueryDataBufferReader::DeviceProfilerQueryDataBufferReader( const DeviceProfiler& profiler, const DeviceProfilerQueryDataBuffer& dataBuffer )
        : m_pProfiler( &profiler )
        , m_pData( &dataBuffer )
        , m_pContext( nullptr )
        , m_pMappedData( m_pData->GetMappedData() )
        , m_pMappedTimestampQueryData( nullptr )
        , m_PerformanceQueryData( 0 )
    {}

    void DeviceProfilerQueryDataBufferReader::SetContext( const void* handle )
    {
        m_pContext = m_pData->GetContext( handle );
        m_pMappedTimestampQueryData = reinterpret_cast<const uint64_t*>(m_pMappedData + m_pContext->m_TimestampDataOffset);
        m_PerformanceQueryData.resize( m_pContext->m_PerformanceDataSize );
    }

    uint64_t DeviceProfilerQueryDataBufferReader::ReadTimestampQueryResult( uint64_t queryIndex ) const
    {
        return m_pMappedTimestampQueryData[ queryIndex ];
    }

    uint32_t DeviceProfilerQueryDataBufferReader::GetPerformanceQueryMetricsSetIndex() const
    {
        return m_pContext->m_PerformanceDataMetricsSetIndex;
    }

    uint32_t DeviceProfilerQueryDataBufferReader::GetPerformanceQueryResultSize() const
    {
        return m_pContext->m_PerformanceDataSize;
    }

    const uint8_t* DeviceProfilerQueryDataBufferReader::ReadPerformanceQueryResult()
    {
        // vkCmdCopyQueryPoolResults must not be used with Intel performance query pools.
        // Copy the data now.
        VkResult result = m_pProfiler->m_pDevice->Callbacks.GetQueryPoolResults(
            m_pProfiler->m_pDevice->Handle,
            m_pContext->m_PerformanceQueryPool,
            0, 1, m_PerformanceQueryData.size(),
            m_PerformanceQueryData.data(),
            m_PerformanceQueryData.size(), 0);

        if( result != VK_SUCCESS )
        {
            memset( m_PerformanceQueryData.data(), 0, m_PerformanceQueryData.size() );
        }

        return m_PerformanceQueryData.data();
    }

    bool DeviceProfilerQueryDataBufferReader::HasPerformanceQueryResult() const
    {
        return m_pContext->m_PerformanceDataSize > 0;
    }
}
