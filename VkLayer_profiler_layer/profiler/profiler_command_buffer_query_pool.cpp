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
        CommandBufferQueryPool

    Description:
        Constructor.

    \***********************************************************************************/
    CommandBufferQueryPool::CommandBufferQueryPool( DeviceProfiler& profiler, uint32_t queueFamilyIndex, VkCommandBufferLevel level )
        : m_Profiler( profiler )
        , m_Device( *profiler.m_pDevice )
        , m_CommandBufferLevel( level )
        , m_QueueFamilyIndex( queueFamilyIndex )
        , m_pPerformanceCounters( profiler.m_pPerformanceCounters.get() )
        , m_QueryPools( 0 )
        , m_QueryPoolSize( 32768 )
        , m_CurrentQueryPoolIndex( 0 )
        , m_CurrentQueryIndex( UINT32_MAX )
        , m_AbsQueryIndex( UINT64_MAX )
        , m_PerformanceQueryPool( VK_NULL_HANDLE )
        , m_PerformanceQueryMetricsSetIndex( UINT32_MAX )
        , m_PerformanceQueryStreamMarkerValue( 0 )
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
        for( VkQueryPool queryPool : m_QueryPools )
        {
            m_Device.Callbacks.DestroyQueryPool(
                m_Device.Handle,
                queryPool,
                nullptr );
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
        GetPerformanceQueryStreamMarkerValue

    Description:
        Returns the stream marker value associated with the last performance query
        in case of collecting the counters in the stream mode.

    \***********************************************************************************/
    uint32_t CommandBufferQueryPool::GetPerformanceQueryStreamMarkerValue() const
    {
        return m_PerformanceQueryStreamMarkerValue;
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
        PreallocateQueries

    Description:
        Makes sure there is enough space in the timestamp query pools for the following
        commands. A new pool is allocated when the current pool is used in >80%.

    \***********************************************************************************/
    void CommandBufferQueryPool::PreallocateQueries( VkCommandBuffer commandBuffer )
    {
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        if( (m_QueryPools.empty()) ||
            ((m_CurrentQueryPoolIndex == m_QueryPools.size()) &&
                (m_CurrentQueryIndex != UINT32_MAX) &&
                (m_CurrentQueryIndex >= (m_QueryPoolSize * 0.8f))) )
        {
            AllocateQueryPool( commandBuffer );
        }
    }

    /***********************************************************************************\

    Function:
        Reset

    Description:
        Resets the query pools and timestamp query allocator.

    \***********************************************************************************/
    void CommandBufferQueryPool::Reset( VkCommandBuffer commandBuffer )
    {
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        // Reset the full query pools.
        for( uint32_t queryPoolIndex = 0; queryPoolIndex < m_CurrentQueryPoolIndex; ++queryPoolIndex )
        {
            m_Device.Callbacks.CmdResetQueryPool(
                commandBuffer,
                m_QueryPools[ queryPoolIndex ],
                0, m_QueryPoolSize );
        }

        // Reset the last query pool.
        if( m_CurrentQueryIndex != UINT32_MAX )
        {
            m_Device.Callbacks.CmdResetQueryPool(
                commandBuffer,
                m_QueryPools[ m_CurrentQueryPoolIndex ],
                0, (m_CurrentQueryIndex + 1) );
        }

        m_AbsQueryIndex = UINT64_MAX;
        m_CurrentQueryIndex = UINT32_MAX;
        m_CurrentQueryPoolIndex = 0;

        m_PerformanceQueryStreamMarkerValue = 0;
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
        if( m_pPerformanceCounters )
        {
            const DeviceProfilerPerformanceCountersSamplingMode samplingMode =
                m_pPerformanceCounters->GetSamplingMode();

            if( samplingMode == DeviceProfilerPerformanceCountersSamplingMode::eQuery )
            {
                AllocatePerformanceQueryPool();

                // Check if there is performance query extension is available.
                if( ( m_PerformanceQueryPool != VK_NULL_HANDLE ) &&
                    ( m_PerformanceQueryMetricsSetIndex != UINT32_MAX ) )
                {
                    m_Device.Callbacks.CmdResetQueryPool(
                        commandBuffer,
                        m_PerformanceQueryPool, 0, 1 );

                    m_Device.Callbacks.CmdBeginQuery(
                        commandBuffer,
                        m_PerformanceQueryPool, 0, 0 );
                }
            }

            if( samplingMode == DeviceProfilerPerformanceCountersSamplingMode::eStream )
            {
                if( m_CommandBufferLevel == VK_COMMAND_BUFFER_LEVEL_PRIMARY )
                {
                    // Write marker if the counters are collected in the stream mode.
                    m_PerformanceQueryStreamMarkerValue = m_pPerformanceCounters->InsertCommandBufferStreamMarker( commandBuffer );
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
        for( uint32_t queryPoolIndex = 0; queryPoolIndex < m_CurrentQueryPoolIndex; ++queryPoolIndex )
        {
            writer.WriteTimestampQueryResults( m_QueryPools[ queryPoolIndex ], m_QueryPoolSize );
        }

        // Copy data from the last query pool.
        if( m_CurrentQueryIndex != UINT32_MAX )
        {
            writer.WriteTimestampQueryResults( m_QueryPools[ m_CurrentQueryPoolIndex ], m_CurrentQueryIndex + 1 );
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

        // Copy data from the performance counters stream.
        if( m_PerformanceQueryStreamMarkerValue != 0 )
        {
            writer.WritePerformanceQueryStreamMarker( m_PerformanceQueryStreamMarkerValue );
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
        // Allocate query from the pool
        m_AbsQueryIndex++;
        m_CurrentQueryIndex++;

        if( m_CurrentQueryIndex == m_QueryPoolSize )
        {
            // Try to reuse next query pool
            m_CurrentQueryIndex = 0;
            m_CurrentQueryPoolIndex++;

            if( m_CurrentQueryPoolIndex == m_QueryPools.size() )
            {
                AllocateQueryPool( commandBuffer );
            }
        }

        // Send the query
        m_Device.Callbacks.CmdWriteTimestamp(
            commandBuffer,
            stage,
            m_QueryPools[ m_CurrentQueryPoolIndex ],
            m_CurrentQueryIndex );

        // Return index to the allocated query.
        return m_AbsQueryIndex;
    }

    /***********************************************************************************\

    Function:
        AllocateQueryPool

    Description:
        Allocates a new timestamp query pool.

    \***********************************************************************************/
    void CommandBufferQueryPool::AllocateQueryPool( VkCommandBuffer commandBuffer )
    {
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        VkQueryPoolCreateInfo queryPoolCreateInfo = {};
        queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queryPoolCreateInfo.queryCount = m_QueryPoolSize;

        VkQueryPool queryPool = VK_NULL_HANDLE;
        VkResult result = m_Device.Callbacks.CreateQueryPool(
            m_Device.Handle,
            &queryPoolCreateInfo,
            nullptr,
            &queryPool );

        if( result == VK_SUCCESS )
        {
            assert( queryPool != VK_NULL_HANDLE );
            m_QueryPools.push_back( queryPool );

            // Pools must be reset before first use
            m_Device.Callbacks.CmdResetQueryPool( commandBuffer, queryPool, 0, m_QueryPoolSize );
        }
    }

    /***********************************************************************************\

    Function:
        AllocatePerformanceQueryPool

    Description:
        Allocates a new performance query pool.

    \***********************************************************************************/
    void CommandBufferQueryPool::AllocatePerformanceQueryPool()
    {
        if( ( m_CommandBufferLevel == VK_COMMAND_BUFFER_LEVEL_PRIMARY ) &&
            ( m_pPerformanceCounters != nullptr ) &&
            ( m_pPerformanceCounters->GetSamplingMode() == DeviceProfilerPerformanceCountersSamplingMode::eQuery ) )
        {
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
}
