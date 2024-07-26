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
    CommandBufferQueryPool::CommandBufferQueryPool( DeviceProfiler& profiler, VkCommandBufferLevel level )
        : m_Profiler( profiler )
        , m_Device( *profiler.m_pDevice )
        , m_MetricsApiINTEL( profiler.m_MetricsApiINTEL )
        , m_QueryPools( 0 )
        , m_QueryPoolSize( 32768 )
        , m_CurrentQueryPoolIndex( 0 )
        , m_CurrentQueryIndex( UINT32_MAX )
        , m_AbsQueryIndex( UINT64_MAX )
        , m_PerformanceQueryPoolINTEL( VK_NULL_HANDLE )
        , m_PerformanceQueryMetricsSetIndexINTEL( UINT32_MAX )
    {
        // Initialize performance query once
        if( (level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) &&
            (m_MetricsApiINTEL.IsAvailable()) )
        {
            VkQueryPoolCreateInfoINTEL intelCreateInfo = {};
            intelCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO_INTEL;
            intelCreateInfo.performanceCountersSampling = VK_QUERY_POOL_SAMPLING_MODE_MANUAL_INTEL;

            VkQueryPoolCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
            createInfo.pNext = &intelCreateInfo;
            createInfo.queryType = VK_QUERY_TYPE_PERFORMANCE_QUERY_INTEL;
            createInfo.queryCount = 1;

            m_Device.Callbacks.CreateQueryPool(
                m_Device.Handle,
                &createInfo,
                nullptr,
                &m_PerformanceQueryPoolINTEL );
        }
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

        if( m_PerformanceQueryPoolINTEL != VK_NULL_HANDLE )
        {
            m_Device.Callbacks.DestroyQueryPool(
                m_Device.Handle,
                m_PerformanceQueryPoolINTEL,
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
        return m_PerformanceQueryMetricsSetIndexINTEL;
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
        m_PerformanceQueryMetricsSetIndexINTEL = m_MetricsApiINTEL.GetActiveMetricsSetIndex();

        // Check if there is performance query extension is available.
        if( (m_PerformanceQueryPoolINTEL != VK_NULL_HANDLE) &&
            (m_PerformanceQueryMetricsSetIndexINTEL != UINT32_MAX) )
        {
            m_Device.Callbacks.CmdResetQueryPool(
                commandBuffer,
                m_PerformanceQueryPoolINTEL, 0, 1 );

            m_Device.Callbacks.CmdBeginQuery(
                commandBuffer,
                m_PerformanceQueryPoolINTEL, 0, 0 );
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
        if( (m_PerformanceQueryPoolINTEL != VK_NULL_HANDLE) &&
            (m_PerformanceQueryMetricsSetIndexINTEL != UINT32_MAX) )
        {
            m_Device.Callbacks.CmdEndQuery(
                commandBuffer,
                m_PerformanceQueryPoolINTEL, 0 );
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
        if( (m_PerformanceQueryPoolINTEL != VK_NULL_HANDLE) &&
            (m_PerformanceQueryMetricsSetIndexINTEL != UINT32_MAX) )
        {
            writer.WritePerformanceQueryResults( m_PerformanceQueryPoolINTEL, m_PerformanceQueryMetricsSetIndexINTEL );
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
}
