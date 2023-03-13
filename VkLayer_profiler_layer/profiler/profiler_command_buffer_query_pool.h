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

#pragma once
#include "profiler_data.h"
#include "profiler_counters.h"
#include "profiler_query_pool.h"
#include "profiler_layer_objects/VkDevice_object.h"

#include "intel/profiler_metrics_api.h"

#include <vulkan/vk_layer.h>

#include <vector>

namespace Profiler
{
    class DeviceProfiler;

    /***********************************************************************************\

    Class:
        CommandBufferQueryPool

    Description:
        Wrapper for set of VkQueryPools used by a single command buffer.

    \***********************************************************************************/
    class CommandBufferQueryPool
    {
    public:
        CommandBufferQueryPool( DeviceProfiler&, VkCommandBufferLevel );
        ~CommandBufferQueryPool();

        CommandBufferQueryPool( const CommandBufferQueryPool& ) = delete;

        uint32_t GetPerformanceQueryMetricsSetIndex() const { return m_PerformanceQueryMetricsSetIndexINTEL; }

        PROFILER_FORCE_INLINE void PreallocateQueries( VkCommandBuffer commandBuffer )
        {
            if( ( m_pQueryPools.empty() ) ||
                ( ( m_CurrentQueryPoolIndex == m_pQueryPools.size() ) &&
                    ( m_CurrentQueryIndex != UINT32_MAX ) &&
                    ( m_CurrentQueryIndex >= ( m_QueryPoolSize * 0.8f ) ) ) )
            {
                AllocateQueryPool( commandBuffer );
            }
        }

        PROFILER_FORCE_INLINE void Reset( VkCommandBuffer commandBuffer )
        {
            // Reset the full query pools.
            for( uint32_t queryPoolIndex = 0; queryPoolIndex < m_CurrentQueryPoolIndex; ++queryPoolIndex )
            {
                m_Device.Callbacks.CmdResetQueryPool(
                    commandBuffer,
                    m_pQueryPools[ queryPoolIndex ]->GetQueryPoolHandle(),
                    0, m_QueryPoolSize );
            }

            // Reset the last query pool.
            if( m_CurrentQueryIndex != UINT32_MAX )
            {
                m_Device.Callbacks.CmdResetQueryPool(
                    commandBuffer,
                    m_pQueryPools[ m_CurrentQueryPoolIndex ]->GetQueryPoolHandle(),
                    0, (m_CurrentQueryIndex + 1) );
            }

            m_CurrentQueryIndex = UINT32_MAX;
            m_CurrentQueryPoolIndex = 0;
        }

        PROFILER_FORCE_INLINE void BeginPerformanceQuery( VkCommandBuffer commandBuffer )
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

        PROFILER_FORCE_INLINE void EndPerformanceQuery( VkCommandBuffer commandBuffer )
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

        PROFILER_FORCE_INLINE void ResolveTimestampsGpu( VkCommandBuffer commandBuffer )
        {
            // Copy data from the full query pools.
            for( uint32_t queryPoolIndex = 0; queryPoolIndex < m_CurrentQueryPoolIndex; ++queryPoolIndex )
            {
                m_pQueryPools[ queryPoolIndex ]->ResolveQueryDataGpu( commandBuffer, m_QueryPoolSize );
            }

            // Copy data from the last query pool.
            if( m_CurrentQueryIndex != UINT32_MAX )
            {
                m_pQueryPools[ m_CurrentQueryPoolIndex ]->ResolveQueryDataGpu( commandBuffer, m_CurrentQueryIndex + 1 );
            }
        }

        PROFILER_FORCE_INLINE void ResolveTimestampsCpu()
        {
            // Copy data from the full query pools.
            for( uint32_t queryPoolIndex = 0; queryPoolIndex < m_CurrentQueryPoolIndex; ++queryPoolIndex )
            {
                m_pQueryPools[ queryPoolIndex ]->ResolveQueryDataCpu( m_QueryPoolSize );
            }

            // Copy data from the last query pool.
            if( m_CurrentQueryIndex != UINT32_MAX )
            {
                m_pQueryPools[ m_CurrentQueryPoolIndex ]->ResolveQueryDataCpu( m_CurrentQueryIndex + 1 );
            }
        }

        PROFILER_FORCE_INLINE uint64_t WriteTimestamp( VkCommandBuffer commandBuffer, VkPipelineStageFlagBits stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT )
        {
            // Allocate query from the pool
            m_CurrentQueryIndex++;

            if( m_CurrentQueryIndex == m_QueryPoolSize )
            {
                // Try to reuse next query pool
                m_CurrentQueryIndex = 0;
                m_CurrentQueryPoolIndex++;

                if( m_CurrentQueryPoolIndex == m_pQueryPools.size() )
                {
                    AllocateQueryPool( commandBuffer );
                }
            }

            // Send the query
            m_Device.Callbacks.CmdWriteTimestamp(
                commandBuffer,
                stage,
                m_pQueryPools[ m_CurrentQueryPoolIndex ]->GetQueryPoolHandle(),
                m_CurrentQueryIndex );

            // Return index to the allocated query.
            return ( static_cast<uint64_t>( m_CurrentQueryPoolIndex ) << 32 ) |
                   ( static_cast<uint64_t>( m_CurrentQueryIndex ) & 0xFFFFFFFF );
        }

        PROFILER_FORCE_INLINE uint64_t GetTimestampData( uint64_t query ) const
        {
            const uint32_t queryPoolIndex = static_cast<uint32_t>( query >> 32 );
            const uint32_t queryIndex = static_cast<uint32_t>( query & 0xFFFFFFFF );

            return m_pQueryPools[ queryPoolIndex ]->GetQueryData( queryIndex );
        }

        PROFILER_FORCE_INLINE std::vector<VkProfilerPerformanceCounterResultEXT> GetPerformanceQueryData() const
        {
            // Check if any performance metrics has been collected.
            if( (m_PerformanceQueryPoolINTEL != VK_NULL_HANDLE) &&
                (m_PerformanceQueryMetricsSetIndexINTEL != UINT32_MAX) )
            {
                // Allocate temporary space for the raw report.
                const size_t reportSize = m_MetricsApiINTEL.GetReportSize( m_PerformanceQueryMetricsSetIndexINTEL );
                std::vector<char> report( reportSize );

                VkResult result = m_Device.Callbacks.GetQueryPoolResults(
                    m_Device.Handle,
                    m_PerformanceQueryPoolINTEL,
                    0, 1, reportSize,
                    report.data(),
                    reportSize, 0 );

                if( result == VK_SUCCESS )
                {
                    // Process metrics for the command buffer.
                    return m_MetricsApiINTEL.ParseReport(
                        m_PerformanceQueryMetricsSetIndexINTEL,
                        report.data(),
                        report.size() );
                }
            }

            // No performance metrics collected.
            return {};
        }

    protected:
        DeviceProfiler&                  m_Profiler;
        VkDevice_Object&                 m_Device;

        ProfilerMetricsApi_INTEL&        m_MetricsApiINTEL;

        std::vector<TimestampQueryPool*> m_pQueryPools;
        uint32_t                         m_QueryPoolSize;
        uint32_t                         m_CurrentQueryPoolIndex;
        uint32_t                         m_CurrentQueryIndex;

        VkQueryPool                      m_PerformanceQueryPoolINTEL;
        uint32_t                         m_PerformanceQueryMetricsSetIndexINTEL;

        PROFILER_FORCE_INLINE void AllocateQueryPool( VkCommandBuffer commandBuffer )
        {
            auto* pQueryPool = m_pQueryPools.emplace_back(
                new TimestampQueryPool( m_Profiler, m_QueryPoolSize ) );

            // Pools must be reset before first use
            m_Device.Callbacks.CmdResetQueryPool(
                commandBuffer, pQueryPool->GetQueryPoolHandle(), 0, m_QueryPoolSize );
        }
    };
}
