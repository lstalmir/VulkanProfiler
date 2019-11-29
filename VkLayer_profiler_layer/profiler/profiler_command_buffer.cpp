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
        , m_CurrentPipeline( VK_NULL_HANDLE )
        , m_CurrentRenderPass( VK_NULL_HANDLE )
        , m_QueryPools()
        , m_QueryPoolSize( 4096 )
        , m_CurrentQueryPoolIndex( UINT_MAX )
        , m_CurrentQueryIndex( UINT_MAX )
    {
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
        Begin

    Description:
        Marks beginning of command buffer.

    \***********************************************************************************/
    void ProfilerCommandBuffer::Begin( const VkCommandBufferBeginInfo* pBeginInfo )
    {
        m_CurrentQueryIndex = UINT_MAX;
        m_CurrentQueryPoolIndex = UINT_MAX;

        // Resize query pool to reduce number of vkGetQueryPoolResults calls
        if( m_QueryPools.size() > 1 )
        {
            // N * m_QueryPoolSize queries were issued in previous command buffer,
            // change allocation unit to fit all these queries in one pool
            m_QueryPoolSize = (m_QueryPools.size() + 1) * m_QueryPoolSize;

            // Destroy the pools to free some memory
            Reset();
        }

        if( !m_QueryPools.empty() )
        {
            // Reset existing query pool to reuse the queries
            m_Profiler.m_Callbacks.CmdResetQueryPool(
                m_CommandBuffer,
                m_QueryPools.front(),
                0, m_QueryPoolSize );

            m_CurrentQueryPoolIndex++;
        }

        // Reset statistics
        m_Data.m_CollectedTimestamps.clear();
        m_Data.m_CopyCount = 0;
        m_Data.m_DispatchCount = 0;
        m_Data.m_DrawCount = 0;
        m_Data.m_PipelineDrawCount.clear();
        m_Data.m_RenderPassPipelineCount.clear();
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
    void ProfilerCommandBuffer::BeginRenderPass( VkRenderPass renderPass )
    {
        // Update state
        m_CurrentRenderPass = renderPass;

        if( m_Profiler.m_Mode == ProfilerMode::ePerRenderPass )
        {
            SendTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
        }

        m_Data.m_RenderPassPipelineCount.push_back( 0 );

        // Some drawcalls may appear without binding any pipeline
        m_Data.m_PipelineDrawCount.push_back( 0 );
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
        m_CurrentPipeline = VK_NULL_HANDLE;
        m_CurrentRenderPass = VK_NULL_HANDLE;

        // vkEndRenderPass marks end of render pass, pipeline and drawcall
        if( m_Profiler.m_Mode <= ProfilerMode::ePerRenderPass )
        {
            SendTimestampQuery( VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );
        }
    }

    /***********************************************************************************\

    Function:
        BindPipeline

    Description:
        Marks beginning of next render pass pipeline.

    \***********************************************************************************/
    void ProfilerCommandBuffer::BindPipeline( VkPipeline pipeline )
    {
        // Update state
        m_CurrentPipeline = pipeline;

        // vkBindPipeline marks end of pipeline and drawcall
        if( m_Profiler.m_Mode <= ProfilerMode::ePerPipeline )
        {
            SendTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
        }

        // Increment draw count in current pipeline
        m_Data.m_RenderPassPipelineCount.back()++;

        m_Data.m_PipelineDrawCount.push_back( 0 );
    }

    /***********************************************************************************\

    Function:
        Draw

    Description:
        Marks beginning of next 3d drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::Draw()
    {
        if( m_Profiler.m_Mode == ProfilerMode::ePerDrawcall )
        {
            SendTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
        }

        // Increment draw count in current pipeline
        m_Data.m_PipelineDrawCount.back()++;
    }

    /***********************************************************************************\

    Function:
        Dispatch

    Description:
        Marks beginning of next compute drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::Dispatch()
    {
        if( m_Profiler.m_Mode == ProfilerMode::ePerDrawcall )
        {
            SendTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
        }

        // Increment draw count in current pipeline
        m_Data.m_PipelineDrawCount.back()++;
    }

    /***********************************************************************************\

    Function:
        Copy

    Description:
        Marks beginning of next copy drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::Copy()
    {
        if( m_Profiler.m_Mode == ProfilerMode::ePerDrawcall )
        {
            SendTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
        }

        // Increment draw count in current pipeline
        m_Data.m_PipelineDrawCount.back()++;
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
        if( !m_QueryPools.empty() )
        {
            // Calculate number of queried timestamps
            const uint32_t numQueries =
                (m_QueryPoolSize * m_CurrentQueryPoolIndex) +
                (m_CurrentQueryIndex + 1);

            m_Data.m_CollectedTimestamps.resize( numQueries );

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
                    m_Data.m_CollectedTimestamps.data() + dataOffset,
                    sizeof( uint64_t ),
                    VK_QUERY_RESULT_64_BIT |
                    VK_QUERY_RESULT_WAIT_BIT );

                numQueriesLeft -= numQueriesInPool;
                dataOffset += numQueriesInPool;
            }
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

                VkStructure<VkQueryPoolCreateInfo> queryPoolCreateInfo;
                queryPoolCreateInfo.queryCount = m_QueryPoolSize;
                queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;

                VkResult result = m_Profiler.m_Callbacks.CreateQueryPool(
                    m_Profiler.m_Device,
                    &queryPoolCreateInfo,
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
    }
}
