#include "profiler_command_buffer.h"
#include "profiler.h"
#include "profiler_helpers.h"
#include <algorithm>

namespace Profiler
{
    /***********************************************************************************\

    Function:
        ProfilerCommandBuffer

    Description:
        Constructor.

    \***********************************************************************************/
    ProfilerCommandBuffer::ProfilerCommandBuffer( Profiler& profiler, VkCommandBuffer commandBuffer )
        : m_Profiler( profiler )
        , m_CommandBuffer( commandBuffer )
        , m_Dirty( false )
        , m_QueryPools()
        , m_QueryPoolSize( 4096 )
        , m_CurrentQueryPoolIndex( UINT_MAX )
        , m_CurrentQueryIndex( UINT_MAX )
        , m_PerformanceQueryPoolINTEL( VK_NULL_HANDLE )
        , m_Data()
    {
        m_Data.m_Handle = commandBuffer;

        // Initialize performance query once
        if( m_Profiler.m_MetricsApiINTEL.IsAvailable() )
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

    /***********************************************************************************\

    Function:
        ~ProfilerCommandBuffer

    Description:
        Destructor.

    \***********************************************************************************/
    ProfilerCommandBuffer::~ProfilerCommandBuffer()
    {
        Reset();
    }

    /***********************************************************************************\

    Function:
        GetCommandBuffer

    Description:
        Returns VkCommandBuffer object associated with this instance.

    \***********************************************************************************/
    VkCommandBuffer ProfilerCommandBuffer::GetCommandBuffer() const
    {
        return m_CommandBuffer;
    }

    /***********************************************************************************\

    Function:
        Submit

    Description:
        Dirty cached profiling data.

    \***********************************************************************************/
    void ProfilerCommandBuffer::Submit()
    {
        // Contents of the command buffer did not change, but all queries will be executed again
        m_Dirty = true;
    }

    /***********************************************************************************\

    Function:
        Begin

    Description:
        Marks beginning of command buffer.

    \***********************************************************************************/
    void ProfilerCommandBuffer::Begin( const VkCommandBufferBeginInfo* pBeginInfo )
    {
        m_CurrentQueryIndex = UINT_MAX;
        m_CurrentQueryPoolIndex = UINT_MAX;

        // Resize query pool to reduce number of vkGetQueryPoolResults calls
        //if( m_QueryPools.size() > 1 )
        //{
        //    // N * m_QueryPoolSize queries were issued in previous command buffer,
        //    // change allocation unit to fit all these queries in one pool
        //    m_QueryPoolSize = (m_QueryPools.size() + 1) * m_QueryPoolSize;
        //
        //    // Destroy the pools to free some memory
        //    Reset();
        //}

        if( m_QueryPools.empty() )
        {
            // Allocate initial query pool
            AllocateQueryPool();
        }
        else
        {
            // Reset existing query pool to reuse the queries
            m_Profiler.m_pDevice->Callbacks.CmdResetQueryPool(
                m_CommandBuffer,
                m_QueryPools.front(),
                0, m_QueryPoolSize );
        }

        m_CurrentQueryPoolIndex++;

        // Reset statistics
        m_Data.Clear();
        m_Dirty = true;

        if( pBeginInfo->flags & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT )
        {
            __debugbreak();
        }

        if( m_PerformanceQueryPoolINTEL )
        {
            m_Profiler.m_pDevice->Callbacks.CmdResetQueryPool(
                m_CommandBuffer,
                m_PerformanceQueryPoolINTEL, 0, 1 );

            m_Profiler.m_pDevice->Callbacks.CmdBeginQuery(
                m_CommandBuffer,
                m_PerformanceQueryPoolINTEL, 0, 0 );
        }
    }

    /***********************************************************************************\

    Function:
        End

    Description:
        Marks end of command buffer.

    \***********************************************************************************/
    void ProfilerCommandBuffer::End()
    {
        if( m_PerformanceQueryPoolINTEL )
        {
            m_Profiler.m_pDevice->Callbacks.CmdEndQuery(
                m_CommandBuffer,
                m_PerformanceQueryPoolINTEL, 0 );
        }
    }

    /***********************************************************************************\

    Function:
        PreBeginRenderPass

    Description:
        Marks beginning of next render pass.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PreBeginRenderPass( const VkRenderPassBeginInfo* pBeginInfo )
    {
        ProfilerRenderPass profilerRenderPass;
        profilerRenderPass.m_Handle = pBeginInfo->renderPass;
        profilerRenderPass.Clear();

        // Begin first subpass
        ProfilerSubpass firstSubpass;
        firstSubpass.m_Handle = 0;
        firstSubpass.Clear();

        profilerRenderPass.m_Subregions.push_back( firstSubpass );

        m_Data.m_Subregions.push_back( profilerRenderPass );

        // Ensure there is a render pass and pipeline bound
        SetupCommandBufferForStatCounting();

        // Clears issued when render pass begins
        // TODO: Read clear count from render pass create info
        m_Data.IncrementStat<STAT_CLEAR_IMPLICIT_COUNT>( pBeginInfo->clearValueCount );

        // Check if we're running out of current query pool
        if( m_CurrentQueryPoolIndex + 1 == m_QueryPools.size() &&
            m_CurrentQueryPoolIndex + 1 > m_QueryPoolSize * 0.85 )
        {
            AllocateQueryPool();
        }

        // Record initial transitions and clears
        SendTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
    }

    /***********************************************************************************\

    Function:
        PostBeginRenderPass

    Description:
        Marks beginning of next render pass.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PostBeginRenderPass()
    {
        SendTimestampQuery( VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );
    }

    /***********************************************************************************\

    Function:
        EndRenderPass

    Description:
        Marks end of current render pass.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PreEndRenderPass()
    {
        // Record final transitions and resolves
        SendTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
    }

    /***********************************************************************************\

    Function:
        PostEndRenderPass

    Description:
        Marks end of current render pass.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PostEndRenderPass()
    {
        SendTimestampQuery( VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );
    }

    /***********************************************************************************\

    Function:
        NextSubpass

    Description:
        Marks beginning of next render pass subpass.

    \***********************************************************************************/
    void ProfilerCommandBuffer::NextSubpass( VkSubpassContents )
    {
        // Render pass must be already tracked
        assert( !m_Data.m_Subregions.empty() );

        ProfilerRenderPass& currentRenderPass = m_Data.m_Subregions.back();

        const uint64_t nextSubpassIndex = currentRenderPass.m_Subregions.size();

        ProfilerSubpass nextSubpass;
        nextSubpass.m_Handle = nextSubpassIndex;
        nextSubpass.Clear();

        currentRenderPass.m_Subregions.push_back( nextSubpass );
    }

    /***********************************************************************************\

    Function:
        BindPipeline

    Description:
        Marks beginning of next render pass pipeline.

    \***********************************************************************************/
    void ProfilerCommandBuffer::BindPipeline( ProfilerPipeline pipeline )
    {
        // Render pass must be already tracked
        assert( !m_Data.m_Subregions.empty() );

        ProfilerRenderPass& currentRenderPass = m_Data.m_Subregions.back();

        // Render pass must have at least one subpass
        assert( !currentRenderPass.m_Subregions.empty() );

        ProfilerSubpass& currentSubpass = currentRenderPass.m_Subregions.back();

        pipeline.Clear();

        // Register new pipeline to current subpass
        currentSubpass.m_Subregions.push_back( pipeline );
    }

    /***********************************************************************************\

    Function:
        PreDraw

    Description:
        Marks beginning of next 3d drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PreDraw()
    {
        SendTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
    }

    /***********************************************************************************\

    Function:
        PostDraw

    Description:
        Marks end of current 3d drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PostDraw()
    {
        SendTimestampQuery( VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );

        // Increment draw count in current pipeline
        m_Data.IncrementStat<STAT_DRAW_COUNT>();
    }

    /***********************************************************************************\

    Function:
        PreDrawIndirect

    Description:
        Marks beginning of next indirect 3d drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PreDrawIndirect()
    {
        SendTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
    }

    /***********************************************************************************\

    Function:
        PostDrawIndirect

    Description:
        Marks end of current indirect 3d drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PostDrawIndirect()
    {
        SendTimestampQuery( VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );

        // Increment draw count in current pipeline
        m_Data.IncrementStat<STAT_DRAW_INDIRECT_COUNT>();
    }

    /***********************************************************************************\

    Function:
        PreDispatch

    Description:
        Marks beginning of next compute drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PreDispatch()
    {
        SendTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
    }

    /***********************************************************************************\

    Function:
        PostDispatch

    Description:
        Marks end of current compute drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PostDispatch()
    {
        SendTimestampQuery( VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );

        // Increment draw count in current pipeline
        m_Data.IncrementStat<STAT_DISPATCH_COUNT>();
    }

    /***********************************************************************************\

    Function:
        PreDispatchIndirect

    Description:
        Marks beginning of next indirect compute drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PreDispatchIndirect()
    {
        SendTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
    }

    /***********************************************************************************\

    Function:
        PostDispatchIndirect

    Description:
        Marks end of current indirect compute drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PostDispatchIndirect()
    {
        SendTimestampQuery( VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );

        // Increment draw count in current pipeline
        m_Data.IncrementStat<STAT_DISPATCH_INDIRECT_COUNT>();
    }

    /***********************************************************************************\

    Function:
        PreCopy

    Description:
        Marks beginning of next copy drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PreCopy()
    {
        SendTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
    }

    /***********************************************************************************\

    Function:
        PostCopy

    Description:
        Marks end of current copy drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PostCopy()
    {
        SendTimestampQuery( VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );

        // Ensure there is a render pass and pipeline bound
        SetupCommandBufferForStatCounting();

        // Increment draw count in current pipeline
        m_Data.IncrementStat<STAT_COPY_COUNT>();
    }

    /***********************************************************************************\

    Function:
        PreClear

    Description:
        Marks beginning of next clear drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PreClear()
    {
        SendTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
    }

    /***********************************************************************************\

    Function:
        PostClear

    Description:
        Marks end of current clear drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PostClear( uint32_t attachmentCount )
    {
        SendTimestampQuery( VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );

        // Ensure there is a render pass and pipeline bound
        SetupCommandBufferForStatCounting();

        // Increment draw count in current pipeline
        m_Data.IncrementStat<STAT_CLEAR_COUNT>( attachmentCount );
    }

    /***********************************************************************************\

    Function:
        OnPipelineBarrier

    Description:
        Stores barrier statistics in currently profiled entity.

    \***********************************************************************************/
    void ProfilerCommandBuffer::OnPipelineBarrier(
        uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers,
        uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers,
        uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers )
    {
        // Ensure there is a render pass and pipeline bound
        SetupCommandBufferForStatCounting();

        // Increment barrier count
        m_Data.IncrementStat<STAT_BARRIER_COUNT>(
            memoryBarrierCount +
            bufferMemoryBarrierCount +
            imageMemoryBarrierCount );
    }

    /***********************************************************************************\

    Function:
        GetData

    Description:
        Reads all queried timestamps. Waits if any timestamp is not available yet.
        Returns structure containing ordered list of timestamps and statistics.

    \***********************************************************************************/
    ProfilerCommandBufferData ProfilerCommandBuffer::GetData()
    {
        if( m_Dirty && !m_QueryPools.empty() )
        {
            // Calculate number of queried timestamps
            const uint32_t numQueries =
                (m_QueryPoolSize * m_CurrentQueryPoolIndex) +
                (m_CurrentQueryIndex + 1);

            std::vector<uint64_t> collectedQueries( numQueries );

            // Count how many queries we need to get
            uint32_t numQueriesLeft = numQueries;
            uint32_t dataOffset = 0;

            // Collect queried timestamps
            for( uint32_t i = 0; i < m_CurrentQueryPoolIndex + 1; ++i )
            {
                const uint32_t numQueriesInPool = min( m_QueryPoolSize, numQueriesLeft );
                const uint32_t dataSize = numQueriesInPool * sizeof( uint64_t );

                // Get results from next query pool
                VkResult result = m_Profiler.m_pDevice->Callbacks.GetQueryPoolResults(
                    m_Profiler.m_pDevice->Handle,
                    m_QueryPools[i],
                    0, numQueriesInPool,
                    dataSize,
                    collectedQueries.data() + dataOffset,
                    sizeof( uint64_t ),
                    VK_QUERY_RESULT_64_BIT |
                    VK_QUERY_RESULT_WAIT_BIT );

                numQueriesLeft -= numQueriesInPool;
                dataOffset += numQueriesInPool;
            }

            // Update timestamp stats for each profiled entity
            if( collectedQueries.size() > 1 )
            {
                size_t currentQueryIndex = 1;

                // Update command buffer begin timestamp
                m_Data.m_Stats.m_BeginTimestamp = collectedQueries[ currentQueryIndex - 1 ];
                // Reset accumulated cycle count if buffer is being reused
                m_Data.m_Stats.m_TotalTicks = 0;

                for( auto& renderPass : m_Data.m_Subregions )
                {
                    // Update render pass begin timestamp
                    renderPass.m_Stats.m_BeginTimestamp = collectedQueries[ currentQueryIndex - 1 ];
                    // Reset accumulated cycle count if buffer is being reused
                    renderPass.m_Stats.m_TotalTicks = 0;

                    if( renderPass.m_Handle != VK_NULL_HANDLE )
                    {
                        // Valid render pass begins with vkCmdBeginRenderPass, which should have
                        // 2 queries for initial transitions and clears.
                        assert( currentQueryIndex < collectedQueries.size() );

                        // Get vkCmdBeginRenderPass time
                        renderPass.m_BeginTicks = collectedQueries[ currentQueryIndex ] - collectedQueries[ currentQueryIndex - 1 ];
                        currentQueryIndex += 2;
                    }

                    for( auto& subpass : renderPass.m_Subregions )
                    {
                        // Update subpass begin timestamp
                        subpass.m_Stats.m_BeginTimestamp = collectedQueries[ currentQueryIndex - 1 ];
                        // Reset accumulated cycle count if buffer is being reused
                        subpass.m_Stats.m_TotalTicks = 0;

                        for( auto& pipeline : subpass.m_Subregions )
                        {
                            // Update pipeline begin timestamp
                            pipeline.m_Stats.m_BeginTimestamp = collectedQueries[ currentQueryIndex - 1 ];
                            // Reset accumulated cycle count if buffer is being reused
                            pipeline.m_Stats.m_TotalTicks = 0;

                            for( auto& drawcall : pipeline.m_Subregions )
                            {
                                // Update drawcall time
                                drawcall.m_Ticks = collectedQueries[ currentQueryIndex ] - collectedQueries[ currentQueryIndex - 1 ];
                                // Update pipeline time
                                pipeline.m_Stats.m_TotalTicks += drawcall.m_Ticks;
                                // Each drawcall has begin and end query
                                currentQueryIndex += 2;
                            }

                            // Update subpass time
                            subpass.m_Stats.m_TotalTicks += pipeline.m_Stats.m_TotalTicks;
                        }

                        // Update render pass time
                        renderPass.m_Stats.m_TotalTicks += subpass.m_Stats.m_TotalTicks;
                    }

                    if( renderPass.m_Handle != VK_NULL_HANDLE )
                    {
                        // Valid render pass ends with vkCmdEndRenderPass, which should have
                        // 2 queries for final transitions and resolves.
                        assert( currentQueryIndex < collectedQueries.size() );

                        // Get vkCmdEndRenderPass time
                        renderPass.m_EndTicks = collectedQueries[ currentQueryIndex ] - collectedQueries[ currentQueryIndex - 1 ];
                        currentQueryIndex += 2;
                    }

                    // Update command buffer time
                    m_Data.m_Stats.m_TotalTicks += renderPass.m_Stats.m_TotalTicks;
                }
            }

            // Read vendor-specific data
            if( m_PerformanceQueryPoolINTEL )
            {
                const size_t reportSize = m_Profiler.m_MetricsApiINTEL.GetReportSize();

                m_Data.tmp.resize( reportSize );

                VkResult result = m_Profiler.m_pDevice->Callbacks.GetQueryPoolResults(
                    m_Profiler.m_pDevice->Handle,
                    m_PerformanceQueryPoolINTEL,
                    0, 1, reportSize,
                    m_Data.tmp.data(),
                    reportSize, 0 );

                assert( result == VK_SUCCESS );
            }

            // Subsequent calls to GetData will return the same results
            m_Dirty = false;
        }
        return m_Data;
    }

    /***********************************************************************************\

    Function:
        Reset

    Description:
        Destroy query pools used by this instance.

    \***********************************************************************************/
    void ProfilerCommandBuffer::Reset()
    {
        // Destroy allocated query pools
        for( auto& pool : m_QueryPools )
        {
            m_Profiler.m_pDevice->Callbacks.DestroyQueryPool(
                m_Profiler.m_pDevice->Handle, pool, nullptr );
        }

        m_QueryPools.clear();
    }

    /***********************************************************************************\

    Function:
        AllocateQueryPool

    Description:

    \***********************************************************************************/
    void ProfilerCommandBuffer::AllocateQueryPool()
    {
        VkQueryPool queryPool = VK_NULL_HANDLE;

        VkQueryPoolCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        info.queryType = VK_QUERY_TYPE_TIMESTAMP;
        info.queryCount = m_QueryPoolSize;

        // Allocate new query pool
        VkResult result = m_Profiler.m_pDevice->Callbacks.CreateQueryPool(
            m_Profiler.m_pDevice->Handle, &info, nullptr, &queryPool );

        if( result != VK_SUCCESS )
        {
            // Allocation failed
            return;
        }

        // Pools must be reset before first use
        m_Profiler.m_pDevice->Callbacks.CmdResetQueryPool(
            m_CommandBuffer, queryPool, 0, m_QueryPoolSize );

        m_QueryPools.push_back( queryPool );
    }

    /***********************************************************************************\

    Function:
        SendTimestampQuery

    Description:
        Send new timestamp query to the command buffer associated with this instance.

    \***********************************************************************************/
    void ProfilerCommandBuffer::SendTimestampQuery( VkPipelineStageFlagBits stage )
    {
        // Allocate query from the pool
        m_CurrentQueryIndex++;

        if( m_CurrentQueryIndex == m_QueryPoolSize || m_QueryPools.empty() )
        {
            // Try to reuse next query pool
            m_CurrentQueryIndex = 0;
            m_CurrentQueryPoolIndex++;

            if( m_CurrentQueryPoolIndex == m_QueryPools.size() )
            {
                __debugbreak();
            }
        }

        // Send the query
        m_Profiler.m_pDevice->Callbacks.CmdWriteTimestamp(
            m_CommandBuffer,
            stage,
            m_QueryPools[m_CurrentQueryPoolIndex],
            m_CurrentQueryIndex );
    }

    /***********************************************************************************\

    Function:
        SetupCommandBufferForStatCounting

    Description:

    \***********************************************************************************/
    void ProfilerCommandBuffer::SetupCommandBufferForStatCounting()
    {
        // Check if we're in render pass
        if( m_Data.m_Subregions.empty() )
        {
            ProfilerRenderPass nullRenderPass;
            nullRenderPass.m_Handle = VK_NULL_HANDLE;
            nullRenderPass.Clear();

            // Each render pass must have at least one subpass
            ProfilerSubpass nullSubpass;
            nullSubpass.m_Handle = VK_NULL_HANDLE;
            nullSubpass.Clear();

            nullRenderPass.m_Subregions.push_back( nullSubpass );

            m_Data.m_Subregions.push_back( nullRenderPass );
        }

        // Check if we're in pipeline
        if( m_Data.m_Subregions.back().m_Subregions[ 0 ].m_Subregions.empty() )
        {
            ProfilerPipeline nullPipeline;
            nullPipeline.m_Handle = VK_NULL_HANDLE;
            nullPipeline.Clear();

            ProfilerRenderPass& currentRenderPass = m_Data.m_Subregions.back();

            // Each render pass must have at least one subpass
            assert( !currentRenderPass.m_Subregions.empty() );

            ProfilerSubpass& currentSubpass = currentRenderPass.m_Subregions.back();

            currentSubpass.m_Subregions.push_back( nullPipeline );
        }
    }
}
