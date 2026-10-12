// Copyright (c) 2022-2024 Lukasz Stalmirski
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
    /***********************************************************************************\

    Function:
        DeviceProfilerQueryDataBuffer

    Description:
        Constructor.

    \***********************************************************************************/
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
            m_Buffer = VK_NULL_HANDLE;
            m_Allocation = VK_NULL_HANDLE;
            m_AllocationInfo = {};

            // Fallback to CPU allocation.
            m_pCpuAllocation = malloc( bufferCreateInfo.size );

            if( m_pCpuAllocation != nullptr )
            {
                m_AllocationInfo.size = bufferCreateInfo.size;
                m_AllocationInfo.pMappedData = m_pCpuAllocation;
            }
        }
    }

    /***********************************************************************************\

    Function:
        ~DeviceProfilerQueryDataBuffer

    Description:
        Destructor.

    \***********************************************************************************/
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

    /***********************************************************************************\

    Function:
        FallbackToCpuAllocation

    Description:
        Releases GPU allocation and allocates a CPU memory for the query data.

    \***********************************************************************************/
    void DeviceProfilerQueryDataBuffer::FallbackToCpuAllocation()
    {
        if( m_Buffer != VK_NULL_HANDLE )
        {
            size_t bufferSize = m_AllocationInfo.size;
            m_Profiler.m_MemoryManager.FreeBuffer(
                m_Buffer,
                m_Allocation );

            m_Buffer = VK_NULL_HANDLE;
            m_Allocation = VK_NULL_HANDLE;
            m_AllocationInfo = {};

            m_pCpuAllocation = malloc( bufferSize );

            if( m_pCpuAllocation != nullptr )
            {
                m_AllocationInfo.size = bufferSize;
                m_AllocationInfo.pMappedData = m_pCpuAllocation;
            }
        }
    }

    /***********************************************************************************\

    Function:
        IsValid

    Description:
        Checks if either GPU or CPU allocation is valid.

    \***********************************************************************************/
    bool DeviceProfilerQueryDataBuffer::IsValid() const
    {
        return m_Buffer != VK_NULL_HANDLE || m_pCpuAllocation != nullptr;
    }

    /***********************************************************************************\

    Function:
        UsesGpuAllocation

    Description:
        Checks whether the buffer uses GPU allocation for storing the data.

    \***********************************************************************************/
    bool DeviceProfilerQueryDataBuffer::UsesGpuAllocation() const
    {
        return m_Buffer != VK_NULL_HANDLE;
    }

    /***********************************************************************************\

    Function:
        GetGpuBuffer

    Description:
        Returns a handle to the GPU allocation.

    \***********************************************************************************/
    VkBuffer DeviceProfilerQueryDataBuffer::GetGpuBuffer() const
    {
        return m_Buffer;
    }

    /***********************************************************************************\

    Function:
        GetCpuBuffer

    Description:
        Returns a pointer to the CPU allocation.

    \***********************************************************************************/
    uint8_t* DeviceProfilerQueryDataBuffer::GetCpuBuffer() const
    {
        return reinterpret_cast<uint8_t*>(m_pCpuAllocation);
    }

    /***********************************************************************************\

    Function:
        GetMappedData

    Description:
        Returns a pointer to a CPU-visible memory, which is either a mapped GPU allocation
        or the CPU allocation.

    \***********************************************************************************/
    const uint8_t* DeviceProfilerQueryDataBuffer::GetMappedData() const
    {
        return reinterpret_cast<const uint8_t*>(m_AllocationInfo.pMappedData);
    }

    /***********************************************************************************\

    Function:
        GetContexts

    Description:
        Returns all contexts stored in the buffer.

    \***********************************************************************************/
    const DeviceProfilerQueryDataBuffer::ContextMap& DeviceProfilerQueryDataBuffer::GetContexts() const
    {
        return m_Contexts;
    }

    /***********************************************************************************\

    Function:
        CreateContext

    Description:
        Creates a query data context and associates it with the provided handle.

    \***********************************************************************************/
    DeviceProfilerQueryDataContext* DeviceProfilerQueryDataBuffer::CreateContext( uint64_t handle )
    {
        return &m_Contexts.emplace( handle, DeviceProfilerQueryDataContext() ).first->second;
    }

    /***********************************************************************************\

    Function:
        GetContext

    Description:
        Returns the query data context associated with the provided handle, or nullptr
        if none was found.

    \***********************************************************************************/
    const DeviceProfilerQueryDataContext* DeviceProfilerQueryDataBuffer::GetContext( uint64_t handle ) const
    {
        auto contextIt = m_Contexts.find( handle );
        if( contextIt != m_Contexts.cend() )
        {
            return &contextIt->second;
        }
        return nullptr;
    }

    /***********************************************************************************\

    Function:
        DeviceProfilerQueryDataBufferWriter

    Description:
        Constructor.

    \***********************************************************************************/
    DeviceProfilerQueryDataBufferWriter::DeviceProfilerQueryDataBufferWriter(
        DeviceProfiler& profiler,
        DeviceProfilerQueryDataBuffer& dataBuffer,
        VkCommandBuffer copyCommandBuffer )
        : m_pProfiler( &profiler )
        , m_pData( &dataBuffer )
        , m_pContext( nullptr )
        , m_CommandBuffer( copyCommandBuffer )
        , m_GpuBuffer( m_pData->GetGpuBuffer() )
        , m_CpuBuffer( m_pData->GetCpuBuffer() )
        , m_DataOffset( 0 )
    {
    }

    /***********************************************************************************\

    Function:
        SetContext

    Description:
        Sets the command buffer context for the subsequent write operations.

    \***********************************************************************************/
    uint64_t DeviceProfilerQueryDataBufferWriter::NextContext()
    {
        uint64_t contextHandle = m_pProfiler->m_RandomGenerator.GetInteger();
        m_pContext = m_pData->CreateContext( contextHandle );
        m_pContext->m_TimestampDataOffset = m_DataOffset;
        return contextHandle;
    }

    /***********************************************************************************\

    Function:
        SetCopyCommandBuffer

    Description:
        Sets the command buffer to be used for copying the query pool results to the
        data buffer.

    \***********************************************************************************/
    void DeviceProfilerQueryDataBufferWriter::SetCopyCommandBuffer( VkCommandBuffer commandBuffer )
    {
        m_CommandBuffer = commandBuffer;
    }

    /***********************************************************************************\

    Function:
        WriteTimestampQueryResults

    Description:
        If a command buffer is available, the function copies the query pool results
        using vkCmdCopyQueryPoolResults command. Otherwise, copies the query results
        immediatelly to the CPU allocation of the data buffer.

    \***********************************************************************************/
    void DeviceProfilerQueryDataBufferWriter::WriteTimestampQueryResults( VkQueryPool queryPool, uint32_t queryOffset, uint32_t queryCount )
    {
        uint32_t dataSize = (queryCount * sizeof( uint64_t ));

        if( m_CommandBuffer != VK_NULL_HANDLE )
        {
            m_pProfiler->m_pDevice->Callbacks.CmdCopyQueryPoolResults(
                m_CommandBuffer,
                queryPool,
                queryOffset,
                queryCount,
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
                queryOffset,
                queryCount,
                dataSize,
                m_CpuBuffer + m_DataOffset,
                sizeof( uint64_t ),
                VK_QUERY_RESULT_64_BIT );
        }

        m_pContext->m_TimestampDataSize += dataSize;
        m_DataOffset += dataSize;
    }

    /***********************************************************************************\

    Function:
        WritePerformanceQueryResults

    Description:
        Associates performance query report with the current command buffer context.
        No data is copied at time of calling this function due to spec limitations.

    \***********************************************************************************/
    void DeviceProfilerQueryDataBufferWriter::WritePerformanceQueryResults( VkQueryPool queryPool, uint32_t metricsSetIndex, uint32_t queueFamilyIndex )
    {
        assert( m_pProfiler->m_pPerformanceCounters != nullptr );
        const uint32_t dataSize = m_pProfiler->m_pPerformanceCounters->GetReportSize(
            metricsSetIndex,
            queueFamilyIndex );

        m_pContext->m_PerformanceDataSize = dataSize;
        m_pContext->m_PerformanceDataMetricsSetIndex = metricsSetIndex;
        m_pContext->m_PerformanceQueryPool = queryPool;
    }

    /***********************************************************************************\

    Function:
        WriteCachedQueryResults

    Description:
        Copy cached query results from another data buffer into the current context.

        This allows to avoid overwriting the query results in the data buffer when the
        same secondary command buffer is executed multiple times in the same parent
        command buffer.

    \***********************************************************************************/
    void DeviceProfilerQueryDataBufferWriter::WriteCachedQueryResults( const DeviceProfilerQueryDataBuffer& buffer )
    {
        std::vector<VkBufferCopy> bufferCopyRegions;

        for( const auto& [contextHandle, contextData] : buffer.GetContexts() )
        {
            // Append cached timestamp query results at the end of the current context's timestamp data.
            if( m_CommandBuffer != VK_NULL_HANDLE )
            {
                VkBufferCopy& bufferCopyRegion = bufferCopyRegions.emplace_back();
                bufferCopyRegion.size = contextData.m_TimestampDataSize;
                bufferCopyRegion.srcOffset = contextData.m_TimestampDataOffset;
                bufferCopyRegion.dstOffset = m_DataOffset;
            }
            else
            {
                // ... CPU path?
            }

            // Save the updated context info in the current data buffer.
            DeviceProfilerQueryDataContext* pNewContext = m_pData->CreateContext( contextHandle );
            pNewContext->m_TimestampDataOffset = m_DataOffset;
            pNewContext->m_TimestampDataSize = contextData.m_TimestampDataSize;
            pNewContext->m_PerformanceDataSize = contextData.m_PerformanceDataSize;
            pNewContext->m_PerformanceDataMetricsSetIndex = contextData.m_PerformanceDataMetricsSetIndex;
            pNewContext->m_PerformanceQueryPool = contextData.m_PerformanceQueryPool;

            m_DataOffset += contextData.m_TimestampDataSize;
        }

        if( !bufferCopyRegions.empty() )
        {
            m_pProfiler->m_pDevice->Callbacks.CmdCopyBuffer(
                m_CommandBuffer,
                buffer.GetGpuBuffer(),
                m_GpuBuffer,
                static_cast<uint32_t>( bufferCopyRegions.size() ),
                bufferCopyRegions.data() );
        }
    }

    /***********************************************************************************\

    Function:
        DeviceProfilerQueryDataBufferReader

    Description:
        Constructor.

    \***********************************************************************************/
    DeviceProfilerQueryDataBufferReader::DeviceProfilerQueryDataBufferReader( const DeviceProfiler& profiler, const DeviceProfilerQueryDataBuffer& dataBuffer )
        : m_pProfiler( &profiler )
        , m_pData( &dataBuffer )
        , m_pContext( nullptr )
        , m_pMappedData( m_pData->GetMappedData() )
        , m_pMappedTimestampQueryData( nullptr )
        , m_PerformanceQueryData( 0 )
    {}

    /***********************************************************************************\

    Function:
        SetContext

    Description:
        Sets the command buffer context for the subsequent read operations.

    \***********************************************************************************/
    void DeviceProfilerQueryDataBufferReader::SetContext( uint64_t handle )
    {
        m_pContext = m_pData->GetContext( handle );
        m_pMappedTimestampQueryData = reinterpret_cast<const uint64_t*>(m_pMappedData + m_pContext->m_TimestampDataOffset);
        m_PerformanceQueryData.resize( m_pContext->m_PerformanceDataSize );
    }

    /***********************************************************************************\

    Function:
        ReadTimestampQueryResult

    Description:
        Returns timestamp query value at the given index. The indices are counted
        independently for each command buffer context.

    \***********************************************************************************/
    uint64_t DeviceProfilerQueryDataBufferReader::ReadTimestampQueryResult( uint64_t queryIndex ) const
    {
        return m_pMappedTimestampQueryData[ queryIndex ];
    }

    /***********************************************************************************\

    Function:
        GetPerformanceQueryMetricsSetIndex

    Description:
        Returns index of the metrics set used to collect the performance query report.
        It is only valid to call this function if HasPerformanceQueryResult returns true
        for the current context.

    \***********************************************************************************/
    uint32_t DeviceProfilerQueryDataBufferReader::GetPerformanceQueryMetricsSetIndex() const
    {
        return m_pContext->m_PerformanceDataMetricsSetIndex;
    }

    /***********************************************************************************\

    Function:
        GetPerformanceQueryResultSize

    Description:
        Returns byte size of the performance query report collected for the current
        context. It is only valid to call this function if HasPerformanceQueryResult
        returns true for the current context.

    \***********************************************************************************/
    uint32_t DeviceProfilerQueryDataBufferReader::GetPerformanceQueryResultSize() const
    {
        return m_pContext->m_PerformanceDataSize;
    }

    /***********************************************************************************\

    Function:
        ReadPerformanceQueryResult

    Description:
        Returns pointer to the performance query report collected for the current
        context. It is only valid to call this function if HasPerformanceQueryResult
        returns true for the current context.

        Because of the spec limitations, the performance query results are always
        collected on CPU when this function is called.

    \***********************************************************************************/
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

    /***********************************************************************************\

    Function:
        HasPerformanceQueryResult

    Description:
        Returns true if there is a performance query report collected for the current
        command buffer context.

    \***********************************************************************************/
    bool DeviceProfilerQueryDataBufferReader::HasPerformanceQueryResult() const
    {
        return m_pContext->m_PerformanceDataSize > 0;
    }
}
