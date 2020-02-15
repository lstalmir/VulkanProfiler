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
        , m_CurrentPipeline( { VK_NULL_HANDLE } )
        , m_CurrentRenderPass( VK_NULL_HANDLE )
        , m_QueryPools()
        , m_QueryPoolSize( 4096 )
        , m_CurrentQueryPoolIndex( UINT_MAX )
        , m_CurrentQueryIndex( UINT_MAX )
    {
        m_Data.m_Handle = commandBuffer;
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

        if( !m_QueryPools.empty() )
        {
            //assert( m_QueryPools.size() == 1 );

            for( auto queryPool : m_QueryPools )
            {
                // Reset existing query pool to reuse the queries
                m_Profiler.m_Callbacks.CmdResetQueryPool(
                    m_CommandBuffer,
                    queryPool,
                    0, m_QueryPoolSize );
            }

            m_CurrentQueryPoolIndex++;
        }

        // Reset statistics
        m_Data.Clear();

        m_Dirty = true;
        m_RunningQuery = false;

        if( pBeginInfo->flags & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT )
        {
            __debugbreak();
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
    }

    /***********************************************************************************\

    Function:
        BeginRenderPass

    Description:
        Marks beginning of next render pass.

    \***********************************************************************************/
    void ProfilerCommandBuffer::BeginRenderPass( const VkRenderPassBeginInfo* pBeginInfo )
    {
        // Update state
        m_CurrentRenderPass = pBeginInfo->renderPass;

        if( m_Profiler.m_Config.m_Mode == ProfilerMode::ePerRenderPass )
        {
            SendTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
        }

        ProfilerRenderPass profilerRenderPass;
        profilerRenderPass.m_Handle = pBeginInfo->renderPass;
        profilerRenderPass.Clear();

        m_Data.m_Subregions.push_back( profilerRenderPass );

        // Temporary pipeline for storing stats before actual pipeline is bound
        ProfilerPipeline profilerPipeline;
        profilerPipeline.m_Handle = VK_NULL_HANDLE;
        profilerPipeline.Clear();

        m_Data.m_Subregions.back().m_Subregions.push_back( profilerPipeline );

        // Clears issued when render pass begins
        m_Data.IncrementStat<STAT_CLEAR_IMPLICIT_COUNT>( pBeginInfo->clearValueCount );
    }

    /***********************************************************************************\

    Function:
        EndRenderPass

    Description:
        Marks end of current render pass.

    \***********************************************************************************/
    void ProfilerCommandBuffer::EndRenderPass()
    {
        // Update state
        m_CurrentPipeline = { VK_NULL_HANDLE };
        m_CurrentRenderPass = VK_NULL_HANDLE;

        // vkEndRenderPass marks end of render pass and pipeline
        if( m_Profiler.m_Config.m_Mode == ProfilerMode::ePerRenderPass ||
            m_Profiler.m_Config.m_Mode == ProfilerMode::ePerPipeline && m_RunningQuery )
        {
            SendTimestampQuery( VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );
        }

        if( m_Profiler.m_Config.m_Mode == ProfilerMode::ePerPipeline )
        {
            m_RunningQuery = false;
        }
    }

    /***********************************************************************************\

    Function:
        BindPipeline

    Description:
        Marks beginning of next render pass pipeline.

    \***********************************************************************************/
    void ProfilerCommandBuffer::BindPipeline( ProfilerPipeline pipeline )
    {
        // Update state
        m_CurrentPipeline = pipeline;
        m_CurrentPipeline.Clear();

        // vkBindPipeline marks end of pipeline and drawcall
        if( m_Profiler.m_Config.m_Mode == ProfilerMode::ePerPipeline )
        {
            SendTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
        }

        // Register new pipeline to current render pass
        m_Data.m_Subregions.back().m_Subregions.push_back( m_CurrentPipeline );
    }

    /***********************************************************************************\

    Function:
        Draw

    Description:
        Marks beginning of next 3d drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::Draw()
    {
        if( m_Profiler.m_Config.m_Mode == ProfilerMode::ePerDrawcall )
        {
            SendTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
        }

        // Increment draw count in current pipeline
        m_Data.IncrementStat<STAT_DRAW_COUNT>();
    }

    /***********************************************************************************\

    Function:
        Dispatch

    Description:
        Marks beginning of next compute drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::Dispatch()
    {
        if( m_Profiler.m_Config.m_Mode == ProfilerMode::ePerDrawcall )
        {
            SendTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
        }

        // Increment dispatch count
        m_Data.IncrementStat<STAT_DISPATCH_COUNT>();
    }

    /***********************************************************************************\

    Function:
        Copy

    Description:
        Marks beginning of next copy drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::Copy()
    {
        if( m_Profiler.m_Config.m_Mode == ProfilerMode::ePerDrawcall )
        {
            SendTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
        }

        // Check if we're in render pass
        if( m_Data.m_Subregions.empty() )
        {
            ProfilerRenderPass nullRenderPass;
            nullRenderPass.m_Handle = VK_NULL_HANDLE;
            nullRenderPass.Clear();

            m_Data.m_Subregions.push_back( nullRenderPass );
        }

        // Check if we're in pipeline
        if( m_Data.m_Subregions.back().m_Subregions.empty() )
        {
            ProfilerPipeline nullPipeline;
            nullPipeline.m_Handle = VK_NULL_HANDLE;
            nullPipeline.Clear();

            m_Data.m_Subregions.back().m_Subregions.push_back( nullPipeline );
        }

        // Increment draw count in current pipeline
        m_Data.IncrementStat<STAT_COPY_COUNT>();
    }

    /***********************************************************************************\

    Function:
        Clear

    Description:
        Marks beginning of next clear drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::Clear( uint32_t attachmentCount )
    {
        if( m_Profiler.m_Config.m_Mode == ProfilerMode::ePerDrawcall )
        {
            SendTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
        }

        // Check if we're in render pass
        if( m_Data.m_Subregions.empty() )
        {
            ProfilerRenderPass nullRenderPass;
            nullRenderPass.m_Handle = VK_NULL_HANDLE;
            nullRenderPass.Clear();

            m_Data.m_Subregions.push_back( nullRenderPass );
        }

        // Check if we're in pipeline
        if( m_Data.m_Subregions.back().m_Subregions.empty() )
        {
            ProfilerPipeline nullPipeline;
            nullPipeline.m_Handle = VK_NULL_HANDLE;
            nullPipeline.Clear();

            m_Data.m_Subregions.back().m_Subregions.push_back( nullPipeline );
        }

        // Increment draw count in current pipeline
        m_Data.IncrementStat<STAT_CLEAR_COUNT>( attachmentCount );
    }

    /***********************************************************************************\

    Function:
        Barrier

    Description:
        Stores barrier statistics in currently profiled entity.

    \***********************************************************************************/
    void ProfilerCommandBuffer::Barrier(
        uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers,
        uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers,
        uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers )
    {
        // Check if we're in render pass
        if( m_Data.m_Subregions.empty() )
        {
            ProfilerRenderPass nullRenderPass;
            nullRenderPass.m_Handle = VK_NULL_HANDLE;
            nullRenderPass.Clear();

            m_Data.m_Subregions.push_back( nullRenderPass );
        }

        // Check if we're in pipeline
        if( m_Data.m_Subregions.back().m_Subregions.empty() )
        {
            ProfilerPipeline nullPipeline;
            nullPipeline.m_Handle = VK_NULL_HANDLE;
            nullPipeline.Clear();

            m_Data.m_Subregions.back().m_Subregions.push_back( nullPipeline );
        }

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
                const uint32_t numQueriesInPool = std::min( m_QueryPoolSize, numQueriesLeft );
                const uint32_t dataSize = numQueriesInPool * sizeof( uint64_t );

                // Get results from next query pool
                VkResult result = m_Profiler.m_Callbacks.GetQueryPoolResults(
                    m_Profiler.m_Device,
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

                if( m_Profiler.m_Config.m_Mode <= ProfilerMode::ePerRenderPass )
                {
                    for( auto& renderPass : m_Data.m_Subregions )
                    {
                        // Update render pass begin timestamp
                        renderPass.m_Stats.m_BeginTimestamp = collectedQueries[ currentQueryIndex - 1 ];

                        if( m_Profiler.m_Config.m_Mode <= ProfilerMode::ePerPipeline )
                        {
                            for( auto& pipeline : renderPass.m_Subregions )
                            {
                                // Update pipeline begin timestamp
                                pipeline.m_Stats.m_BeginTimestamp = collectedQueries[ currentQueryIndex - 1 ];

                                if( m_Profiler.m_Config.m_Mode == ProfilerMode::ePerDrawcall )
                                {
                                    for( auto& drawcall : pipeline.m_Subregions )
                                    {
                                        // Update drawcall time
                                        drawcall.m_Ticks = collectedQueries[ currentQueryIndex ] - collectedQueries[ currentQueryIndex - 1 ];
                                        // Update pipeline time
                                        pipeline.m_Stats.m_TotalTicks += drawcall.m_Ticks;
                                        currentQueryIndex++;
                                    }
                                }

                                if( m_Profiler.m_Config.m_Mode == ProfilerMode::ePerPipeline &&
                                    pipeline.m_Handle != VK_NULL_HANDLE )
                                {
                                    // Update pipeline time
                                    pipeline.m_Stats.m_TotalTicks = collectedQueries[ currentQueryIndex ] - pipeline.m_Stats.m_BeginTimestamp;
                                    currentQueryIndex++;

                                    // TMP
                                    // TODO: Detect timestamp disjoints
                                    if( pipeline.m_Stats.m_TotalTicks > 1000000 )
                                    {
                                        pipeline.m_Stats.m_TotalTicks = 0;
                                    }
                                }

                                // Update render pass time
                                renderPass.m_Stats.m_TotalTicks += pipeline.m_Stats.m_TotalTicks;
                            }
                        }

                        if( m_Profiler.m_Config.m_Mode == ProfilerMode::ePerRenderPass &&
                            renderPass.m_Handle != VK_NULL_HANDLE )
                        {
                            // Update render pass time
                            renderPass.m_Stats.m_TotalTicks += collectedQueries[ currentQueryIndex ] - renderPass.m_Stats.m_BeginTimestamp;
                            currentQueryIndex++;
                        }

                        // Update command buffer time
                        m_Data.m_Stats.m_TotalTicks += renderPass.m_Stats.m_TotalTicks;
                    }
                }
                else
                {
                    // Update command buffer time
                    m_Data.m_Stats.m_TotalTicks = collectedQueries[ currentQueryIndex ] - m_Data.m_Stats.m_BeginTimestamp;
                }
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
            m_Profiler.m_Callbacks.DestroyQueryPool( m_Profiler.m_Device, pool, nullptr );
        }

        m_QueryPools.clear();
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
                // Allocate new query pool
                VkQueryPool queryPool = VK_NULL_HANDLE;

                VkQueryPoolCreateInfo queryPoolCreateInfo;
                ClearStructure( &queryPoolCreateInfo, VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO );

                queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
                queryPoolCreateInfo.queryCount = m_QueryPoolSize;

                VkResult result = m_Profiler.m_Callbacks.CreateQueryPool(
                    m_Profiler.m_Device,
                    &(VkQueryPoolCreateInfo)queryPoolCreateInfo,
                    nullptr,
                    &queryPool );

                if( result != VK_SUCCESS )
                {
                    // Failed to allocate new query pool
                    return;
                }

                m_QueryPools.push_back( queryPool );
            }
        }

        // Send the query
        m_Profiler.m_Callbacks.CmdWriteTimestamp(
            m_CommandBuffer,
            stage,
            m_QueryPools[m_CurrentQueryPoolIndex],
            m_CurrentQueryIndex );

        m_RunningQuery = true;
    }
}
