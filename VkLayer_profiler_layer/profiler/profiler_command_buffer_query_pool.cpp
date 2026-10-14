// Copyright (c) 2022-2025 Lukasz Stalmirski
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
#include "profiler_query_pool.h"
#include "profiler_layer_objects/VkDevice_object.h"

#include <assert.h>

namespace Profiler
{
    /***********************************************************************************\

    Function:
        CommandBufferQueryPoolAllocator

    Description:
        Constructor.

    \***********************************************************************************/
    CommandBufferQueryPoolAllocator::CommandBufferQueryPoolAllocator()
    {
        ResetMembers();
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initializes the query allocator.

    \***********************************************************************************/
    VkResult CommandBufferQueryPoolAllocator::Initialize( DeviceProfiler& profiler, VkQueryType queryType, uint32_t queryPoolSize, uint32_t queryAllocationSize )
    {
        m_pProfiler = &profiler;
        m_pDevice = profiler.m_pDevice;

        m_QueryType = queryType;
        m_QueryPoolSize = queryPoolSize;
        m_QueryAllocationSize = queryAllocationSize;

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Destroys the query allocator.

    \***********************************************************************************/
    void CommandBufferQueryPoolAllocator::Destroy()
    {
        std::scoped_lock lock( m_Mutex );

        for( QueryPool& queryPool : m_QueryPools )
        {
            assert( m_pDevice != nullptr );
            m_pDevice->Callbacks.DestroyQueryPool(
                m_pDevice->Handle,
                queryPool.m_Handle,
                nullptr );
        }

        ResetMembers();
    }

    /***********************************************************************************\

    Function:
        Allocate

    Description:
        Suballocates a range of queries from the query pool.

    \***********************************************************************************/
    bool CommandBufferQueryPoolAllocator::Allocate( CommandBufferQueryPoolAllocation& allocation )
    {
        std::scoped_lock lock( m_Mutex );

        auto ReserveQueryPool = [&]( QueryPool& queryPool, uint32_t chunkIndex )
        {
            queryPool.m_Allocations[chunkIndex] = true;
            allocation.m_QueryPool = queryPool.m_Handle;
            allocation.m_QueryOffset = chunkIndex * m_QueryAllocationSize;
            allocation.m_QueryCount = m_QueryAllocationSize;
        };

        // Try to suballocate from an existing query pool.
        for( QueryPool& queryPool : m_QueryPools )
        {
            for( uint32_t i = 0; i < queryPool.m_Allocations.size(); ++i )
            {
                if( !queryPool.m_Allocations[i] )
                {
                    ReserveQueryPool( queryPool, i );
                    return true;
                }
            }
        }

        // Create a new query pool if no existing pool has free space.
        VkQueryPoolCreateInfo queryPoolCreateInfo = {};
        queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolCreateInfo.queryType = m_QueryType;
        queryPoolCreateInfo.queryCount = m_QueryPoolSize;

        VkQueryPool queryPoolHandle = VK_NULL_HANDLE;
        VkResult result = m_pDevice->Callbacks.CreateQueryPool(
            m_pDevice->Handle,
            &queryPoolCreateInfo,
            nullptr,
            &queryPoolHandle );

        if( result == VK_SUCCESS )
        {
            try
            {
                QueryPool queryPool = {};
                queryPool.m_Handle = queryPoolHandle;
                queryPool.m_Allocations.resize( m_QueryPoolSize / m_QueryAllocationSize, false );
                m_QueryPools.push_back( std::move( queryPool ) );

                ReserveQueryPool( m_QueryPools.back(), 0 );
                return true;
            }
            catch( const std::bad_alloc& )
            {
                // Failed to allocate memory for the query pool state.
                m_pDevice->Callbacks.DestroyQueryPool(
                    m_pDevice->Handle,
                    queryPoolHandle,
                    nullptr );

                return false;
            }
        }

        return false;
    }

    /***********************************************************************************\

    Function:
        Free

    Description:
        Frees a previously allocated range of queries.

    \***********************************************************************************/
    void CommandBufferQueryPoolAllocator::Free( const CommandBufferQueryPoolAllocation& allocation )
    {
        std::scoped_lock lock( m_Mutex );

        for( QueryPool& queryPool : m_QueryPools )
        {
            if( queryPool.m_Handle == allocation.m_QueryPool )
            {
                queryPool.m_Allocations[allocation.m_QueryOffset / m_QueryAllocationSize] = false;
                return;
            }
        }
    }

    /***********************************************************************************\

    Function:
        ResetMembers

    Description:
        Resets the members to their default values.

    \***********************************************************************************/
    void CommandBufferQueryPoolAllocator::ResetMembers()
    {
        m_pProfiler = nullptr;
        m_pDevice = nullptr;

        m_QueryType = {};
        m_QueryPoolSize = m_scDefaultQueryPoolSize;
        m_QueryAllocationSize = m_scDefaultAllocationSize;

        m_QueryPools.clear();
    }

    /***********************************************************************************\

    Function:
        CommandBufferQueryPool

    Description:
        Constructor.

    \***********************************************************************************/
    CommandBufferQueryPool::CommandBufferQueryPool( DeviceProfiler& profiler, uint32_t queueFamilyIndex, VkCommandBufferLevel level )
        : m_Profiler( profiler )
        , m_Device( *profiler.m_pDevice )
        , m_TimestampAllocator( profiler.m_TimestampQueryAllocator )
        , m_CommandBufferLevel( level )
        , m_QueueFamilyIndex( queueFamilyIndex )
        , m_pPerformanceCounters( profiler.m_pPerformanceCounters.get() )
        , m_QueryPools( 0 )
        , m_CurrentQueryIndex( UINT32_MAX )
        , m_AbsQueryIndex( UINT64_MAX )
        , m_PerformanceQueryPool( VK_NULL_HANDLE )
        , m_PerformanceQueryMetricsSetIndex( UINT32_MAX )
    {
    }

    /***********************************************************************************\

    Function:
        ~CommandBufferQueryPool

    Description:
        Destructor.

    \***********************************************************************************/
    CommandBufferQueryPool::~CommandBufferQueryPool()
    {
        for( auto& allocation : m_QueryPools )
        {
            m_TimestampAllocator.Free( allocation );
        }

        if( m_PerformanceQueryPool != VK_NULL_HANDLE )
        {
            m_Device.Callbacks.DestroyQueryPool(
                m_Device.Handle,
                m_PerformanceQueryPool,
                nullptr );
        }
    }

    /***********************************************************************************\

    Function:
        GetPerformanceQueryMetricsSetIndex

    Description:
        Returns the metrics set index this query pool collected in the last call to
        BeginPerformanceQuery.

    \***********************************************************************************/
    uint32_t CommandBufferQueryPool::GetPerformanceQueryMetricsSetIndex() const
    {
        return m_PerformanceQueryMetricsSetIndex;
    }

    /***********************************************************************************\

    Function:
        GetTimestampQueryCount

    Description:
        Returns total number of timestamp queries inserted by this query pool.

    \***********************************************************************************/
    uint64_t CommandBufferQueryPool::GetTimestampQueryCount() const
    {
        return (m_AbsQueryIndex + 1);
    }

    /***********************************************************************************\

    Function:
        GetRequiredBufferSize

    Description:
        Returns size in bytes required to store all data queried by this pool.

    \***********************************************************************************/
    uint64_t CommandBufferQueryPool::GetRequiredBufferSize() const
    {
        // Performance query report doesn't have to be included in the reported size,
        // beacuse the data can't be copied on the GPU.
        return GetTimestampQueryCount() * sizeof( uint64_t );
    }

    /***********************************************************************************\

    Function:
        Reset

    Description:
        Resets the query pools and timestamp query allocator.

    \***********************************************************************************/
    void CommandBufferQueryPool::Reset()
    {
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        m_AbsQueryIndex = UINT64_MAX;
        m_CurrentQueryIndex = UINT32_MAX;

        for( CommandBufferQueryPoolAllocation& allocation : m_QueryPools )
        {
            m_TimestampAllocator.Free( allocation );
        }

        m_QueryPools.clear();
    }

    /***********************************************************************************\

    Function:
        BeginPerformanceQuery

    Description:
        Begins collection of performance metrics in the currently selected set.
        Available only on INTEL.

    \***********************************************************************************/
    void CommandBufferQueryPool::BeginPerformanceQuery( VkCommandBuffer commandBuffer )
    {
        if( (m_pPerformanceCounters != nullptr) &&
            (m_CommandBufferLevel == VK_COMMAND_BUFFER_LEVEL_PRIMARY) )
        {
            const VkProfilerPerformanceCountersSamplingModeEXT samplingMode =
                m_pPerformanceCounters->GetSamplingMode();

            if( samplingMode == VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_QUERY_EXT )
            {
                AllocatePerformanceQueryPool();

                // Check if there is performance query extension is available.
                if( (m_PerformanceQueryPool != VK_NULL_HANDLE) &&
                    (m_PerformanceQueryMetricsSetIndex != UINT32_MAX) )
                {
                    m_Device.Callbacks.CmdResetQueryPool(
                        commandBuffer,
                        m_PerformanceQueryPool, 0, 1 );

                    m_Device.Callbacks.CmdBeginQuery(
                        commandBuffer,
                        m_PerformanceQueryPool, 0, 0 );
                }
            }
        }
    }

    /***********************************************************************************\

    Function:
        EndPerformanceQuery

    Description:
        Ends collection of performance metrics in the currently selected set.
        Available only on INTEL.

    \***********************************************************************************/
    void CommandBufferQueryPool::EndPerformanceQuery( VkCommandBuffer commandBuffer )
    {
        // Check if any performance metrics has been collected.
        if( (m_PerformanceQueryPool != VK_NULL_HANDLE) &&
            (m_PerformanceQueryMetricsSetIndex != UINT32_MAX) )
        {
            m_Device.Callbacks.CmdEndQuery(
                commandBuffer,
                m_PerformanceQueryPool, 0 );
        }
    }

    /***********************************************************************************\

    Function:
        ResolveTimestampsGpu

    Description:
        Copies timestamp query data from all the pools to the timestamp query buffer
        using the provided command buffer.

    \***********************************************************************************/
    void CommandBufferQueryPool::WriteQueryData( DeviceProfilerQueryDataBufferWriter& writer ) const
    {
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        // Copy data from the full query pools.
        const CommandBufferQueryPoolAllocation* pQueryPoolAllocation = m_QueryPools.data();

        uint64_t queryCountLeft = m_AbsQueryIndex + 1;
        while( queryCountLeft > 0 )
        {
            uint32_t writeCount = static_cast<uint32_t>(
                std::min<uint64_t>( queryCountLeft, pQueryPoolAllocation->m_QueryCount ) );

            writer.WriteTimestampQueryResults( pQueryPoolAllocation->m_QueryPool, pQueryPoolAllocation->m_QueryOffset, writeCount );

            queryCountLeft -= writeCount;
            pQueryPoolAllocation++;
        }

        // Copy data from the performance query pool.
        if( m_PerformanceQueryPool != VK_NULL_HANDLE )
        {
            assert( m_pPerformanceCounters != nullptr );
            uint32_t performanceQueryMetricsSetIndex = m_PerformanceQueryMetricsSetIndex;

            // If the performance query pools are reusable, the profiler can select a different metrics set
            // without re-recording the command buffer. Grab the index to the current metrics set in such case.
            if( m_pPerformanceCounters->SupportsQueryPoolReuse() )
            {
                performanceQueryMetricsSetIndex = m_pPerformanceCounters->GetActiveMetricsSetIndex();
            }

            if( performanceQueryMetricsSetIndex != UINT32_MAX )
            {
                writer.WritePerformanceQueryResults( m_PerformanceQueryPool, performanceQueryMetricsSetIndex, m_QueueFamilyIndex );
            }
        }
    }

    /***********************************************************************************\

    Function:
        ResetQueryPools

    Description:
        Resets the query pools.

    \***********************************************************************************/
    void CommandBufferQueryPool::ResetQueryPools( VkCommandBuffer commandBuffer ) const
    {
        const CommandBufferQueryPoolAllocation* pQueryPoolAllocation = m_QueryPools.data();

        uint64_t queryCountLeft = m_AbsQueryIndex + 1;
        while( queryCountLeft > 0 )
        {
            uint32_t resetCount = static_cast<uint32_t>(
                std::min<uint64_t>( queryCountLeft, pQueryPoolAllocation->m_QueryCount ) );

            m_Device.Callbacks.CmdResetQueryPool(
                commandBuffer,
                pQueryPoolAllocation->m_QueryPool,
                pQueryPoolAllocation->m_QueryOffset,
                resetCount );

            queryCountLeft -= resetCount;
            pQueryPoolAllocation++;
        }
    }

    /***********************************************************************************\

    Function:
        WriteTimestamp

    Description:
        Writes timestamp query to the provided command buffer at the given stage.
        Returns index to the written timestamp query.

    \***********************************************************************************/
    uint64_t CommandBufferQueryPool::WriteTimestamp( VkCommandBuffer commandBuffer, VkPipelineStageFlagBits stage )
    {
        if( !EnsureTimestampQueryPoolSpace() )
        {
            return UINT64_MAX;
        }

        m_AbsQueryIndex++;
        m_CurrentQueryIndex++;

        // Send the query
        m_Device.Callbacks.CmdWriteTimestamp(
            commandBuffer,
            stage,
            m_QueryPools.back().m_QueryPool,
            m_QueryPools.back().m_QueryOffset + m_CurrentQueryIndex );

        // Return index to the allocated query.
        return m_AbsQueryIndex;
    }

    /***********************************************************************************\

    Function:
        EnsureQueryPoolSpace

    Description:
        Allocates a new timestamp query pool if needed.

    \***********************************************************************************/
    bool CommandBufferQueryPool::EnsureTimestampQueryPoolSpace()
    {
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        if( ( m_QueryPools.empty() ) ||
            ( m_CurrentQueryIndex + 1 >= m_QueryPools.back().m_QueryCount ) )
        {
            CommandBufferQueryPoolAllocation allocation;
            if( !m_TimestampAllocator.Allocate( allocation ) )
            {
                // Allocation failed.
                return false;
            }

            try
            {
                m_QueryPools.push_back( std::move( allocation ) );
            }
            catch( const std::bad_alloc& )
            {
                // push_back failed.
                m_TimestampAllocator.Free( allocation );
                return false;
            }

            // Start from the beginning of the new query pool.
            m_CurrentQueryIndex = UINT32_MAX;
        }

        return true;
    }

    /***********************************************************************************\

    Function:
        AllocatePerformanceQueryPool

    Description:
        Allocates a new performance query pool.

    \***********************************************************************************/
    void CommandBufferQueryPool::AllocatePerformanceQueryPool()
    {
        assert( m_pPerformanceCounters != nullptr );
        assert( m_pPerformanceCounters->GetSamplingMode() == VK_PROFILER_PERFORMANCE_COUNTERS_SAMPLING_MODE_QUERY_EXT );

        // Try to reuse the existing query pool if possible.
        bool canReuseCurrentQueryPool = ( m_PerformanceQueryPool != VK_NULL_HANDLE );

        // If the current metrics set has changed, it's possible to reuse the query pool
        // only if the provider supports it.
        const uint32_t activeMetricsSetIndex = m_pPerformanceCounters->GetActiveMetricsSetIndex();
        if( !m_pPerformanceCounters->AreMetricsSetsCompatible(
                m_PerformanceQueryMetricsSetIndex,
                activeMetricsSetIndex ) )
        {
            canReuseCurrentQueryPool &= m_pPerformanceCounters->SupportsQueryPoolReuse();
        }

        // Allocate new query pool if needed.
        if( !canReuseCurrentQueryPool )
        {
            if( m_PerformanceQueryPool != VK_NULL_HANDLE )
            {
                m_Device.Callbacks.DestroyQueryPool(
                    m_Device.Handle,
                    m_PerformanceQueryPool,
                    nullptr );

                m_PerformanceQueryPool = VK_NULL_HANDLE;
            }

            m_pPerformanceCounters->CreateQueryPool(
                m_QueueFamilyIndex, 1,
                &m_PerformanceQueryPool );
        }

        // Save the metrics set index for post-processing.
        m_PerformanceQueryMetricsSetIndex = activeMetricsSetIndex;
    }
}
