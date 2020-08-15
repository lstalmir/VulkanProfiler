#include "profiler_command_buffer.h"
#include "profiler.h"
#include "profiler_helpers.h"
#include <algorithm>

#ifndef _DEBUG
#define NDEBUG
#endif
#include <assert.h>

namespace Profiler
{
    /***********************************************************************************\

    Function:
        ProfilerCommandBuffer

    Description:
        Constructor.

    \***********************************************************************************/
    ProfilerCommandBuffer::ProfilerCommandBuffer( DeviceProfiler& profiler, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkCommandBufferLevel level )
        : m_Profiler( profiler )
        , m_CommandPool( commandPool )
        , m_CommandBuffer( commandBuffer )
        , m_Level( level )
        , m_Dirty( false )
        , m_QueryPools()
        , m_QueryPoolSize( 4096 )
        , m_CurrentQueryPoolIndex( -1 )
        , m_CurrentQueryIndex( -1 )
        , m_PerformanceQueryPoolINTEL( VK_NULL_HANDLE )
        , m_Data()
        , m_pCurrentRenderPass( nullptr )
        , m_pCurrentRenderPassData( nullptr )
        , m_CurrentSubpassIndex( -1 )
    {
        m_Data.m_Handle = commandBuffer;

        // Initialize performance query once
        if( (m_Level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) && (m_Profiler.m_MetricsApiINTEL.IsAvailable()) )
        {
            VkQueryPoolCreateInfoINTEL intelCreateInfo = {};
            intelCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO_INTEL;
            intelCreateInfo.performanceCountersSampling = VK_QUERY_POOL_SAMPLING_MODE_MANUAL_INTEL;

            VkQueryPoolCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
            createInfo.pNext = &intelCreateInfo;
            createInfo.queryType = VK_QUERY_TYPE_PERFORMANCE_QUERY_INTEL;
            createInfo.queryCount = 1;

            VkResult result = m_Profiler.m_pDevice->Callbacks.CreateQueryPool(
                m_Profiler.m_pDevice->Handle,
                &createInfo,
                nullptr,
                &m_PerformanceQueryPoolINTEL );

            assert( result == VK_SUCCESS );
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
        GetCommandPool

    Description:
        Returns VkCommandPool object associated with this instance.

    \***********************************************************************************/
    VkCommandPool ProfilerCommandBuffer::GetCommandPool() const
    {
        return m_CommandPool;
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
        CpuScopedTimestampCounter<std::chrono::nanoseconds, true /*overwrite*/> counter( m_Data.m_ProfilerCPUOverheadNs );

        m_CurrentQueryIndex = -1;
        m_CurrentQueryPoolIndex = -1;

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

        // Reset data
        m_Data.m_PerformanceQueryReportINTEL.clear();
        m_Data.m_RenderPasses.clear();
        m_Data.m_Stats = {};
        m_Data.m_Ticks = {};
        m_Dirty = true;

        m_CurrentSubpassIndex = -1;
        m_pCurrentRenderPass = nullptr;
        m_pCurrentRenderPassData = nullptr;

        if( pBeginInfo->flags & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT )
        {
            // Setup render pass and subpass for commands
            SetupCommandBufferForStatCounting( { VK_NULL_HANDLE } );
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
        CpuScopedTimestampCounter<std::chrono::nanoseconds> counter( m_Data.m_ProfilerCPUOverheadNs );

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
    void ProfilerCommandBuffer::PreBeginRenderPass( const VkRenderPassBeginInfo* pBeginInfo, VkSubpassContents )
    {
        CpuScopedTimestampCounter<std::chrono::nanoseconds> counter( m_Data.m_ProfilerCPUOverheadNs );

        m_pCurrentRenderPass = &m_Profiler.GetRenderPass( pBeginInfo->renderPass );

        DeviceProfilerRenderPassData profilerRenderPassData;
        profilerRenderPassData.m_Handle = pBeginInfo->renderPass;

        // Clears issued when render pass begins
        profilerRenderPassData.m_Stats.m_ClearColorCount += m_pCurrentRenderPass->m_ClearColorAttachmentCount;
        profilerRenderPassData.m_Stats.m_ClearDepthStencilCount += m_pCurrentRenderPass->m_ClearDepthStencilAttachmentCount;

        m_Data.m_RenderPasses.push_back( profilerRenderPassData );

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
    void ProfilerCommandBuffer::PostBeginRenderPass( const VkRenderPassBeginInfo*, VkSubpassContents contents )
    {
        {
            // Keep overhead counter in separate block, NextSubpass has own counter
            CpuScopedTimestampCounter<std::chrono::nanoseconds> counter( m_Data.m_ProfilerCPUOverheadNs );

            SendTimestampQuery( VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );

            m_pCurrentRenderPassData = &m_Data.m_RenderPasses.back();
        }

        // Begin first subpass
        NextSubpass( contents );
    }

    /***********************************************************************************\

    Function:
        EndRenderPass

    Description:
        Marks end of current render pass.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PreEndRenderPass()
    {
        CpuScopedTimestampCounter<std::chrono::nanoseconds> counter( m_Data.m_ProfilerCPUOverheadNs );

        // End currently profiled subpass
        EndSubpass();

        // No more subpasses in this render pass
        m_CurrentSubpassIndex = -1;
        m_pCurrentRenderPass = nullptr;
        m_pCurrentRenderPassData = nullptr;

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
        CpuScopedTimestampCounter<std::chrono::nanoseconds> counter( m_Data.m_ProfilerCPUOverheadNs );
        SendTimestampQuery( VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );
    }

    /***********************************************************************************\

    Function:
        NextSubpass

    Description:
        Marks beginning of next render pass subpass.

    \***********************************************************************************/
    void ProfilerCommandBuffer::NextSubpass( VkSubpassContents contents )
    {
        CpuScopedTimestampCounter<std::chrono::nanoseconds> counter( m_Data.m_ProfilerCPUOverheadNs );

        // End currently profiled subpass before beginning new one
        EndSubpass();

        m_CurrentSubpassIndex++;

        DeviceProfilerSubpassData nextSubpass;
        nextSubpass.m_Index = m_CurrentSubpassIndex;
        nextSubpass.m_Contents = contents;

        DeviceProfilerRenderPassData& currentRenderPass = m_Data.m_RenderPasses.back();
        currentRenderPass.m_Subpasses.push_back( nextSubpass );
    }

    /***********************************************************************************\

    Function:
        BindPipeline

    Description:
        Marks beginning of next render pass pipeline.

    \***********************************************************************************/
    void ProfilerCommandBuffer::BindPipeline( const DeviceProfilerPipeline& pipeline )
    {
        CpuScopedTimestampCounter<std::chrono::nanoseconds> counter( m_Data.m_ProfilerCPUOverheadNs );

        switch( pipeline.m_BindPoint )
        {
        case VK_PIPELINE_BIND_POINT_GRAPHICS:
            m_GraphicsPipeline = pipeline;
            break;

        case VK_PIPELINE_BIND_POINT_COMPUTE:
            m_ComputePipeline = pipeline;
            break;

        case VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR:
            ProfilerPlatformFunctions::WriteDebug(
                "%s - VK_KHR_ray_tracing extension not supported\n",
                __FUNCTION__ );
            break;
        }
    }

    /***********************************************************************************\

    Function:
        PreDraw

    Description:
        Marks beginning of next 3d drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PreDraw( const DeviceProfilerDrawcall& drawcall )
    {
        CpuScopedTimestampCounter<std::chrono::nanoseconds> counter( m_Data.m_ProfilerCPUOverheadNs );

        const DeviceProfilerPipelineType pipelineType = drawcall.GetPipelineType();

        // Setup pipeline
        switch( pipelineType )
        {
        case DeviceProfilerPipelineType::eNone:
            SetupCommandBufferForStatCounting( { VK_NULL_HANDLE } );
            break;

        case DeviceProfilerPipelineType::eGraphics:
            SetupCommandBufferForStatCounting( m_GraphicsPipeline );
            break;

        case DeviceProfilerPipelineType::eCompute:
            SetupCommandBufferForStatCounting( m_ComputePipeline );
            break;

        default: // Internal pipelines
            SetupCommandBufferForStatCounting( m_Profiler.GetPipeline( (VkPipeline)pipelineType ) );
            break;
        }

        // Append drawcall to the current pipeline
        GetCurrentPipeline().template AppendDrawcall<true /*explicit*/>( drawcall );

        // Begin timestamp query
        SendTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
    }

    /***********************************************************************************\

    Function:
        PostDraw

    Description:
        Marks end of current drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PostDraw()
    {
        CpuScopedTimestampCounter<std::chrono::nanoseconds> counter( m_Data.m_ProfilerCPUOverheadNs );
        SendTimestampQuery( VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );
    }

    /***********************************************************************************\

    Function:
        DebugLabel

    Description:
        Inserts debug label into the command buffer.

    \***********************************************************************************/
    void ProfilerCommandBuffer::DebugLabel( const char* pName, const float color[ 4 ] )
    {
        CpuScopedTimestampCounter<std::chrono::nanoseconds> counter( m_Data.m_ProfilerCPUOverheadNs );

        // Ensure there is a render pass and subpass with VK_SUBPASS_CONTENTS_INLINE flag
        SetupCommandBufferForStatCounting( { VK_NULL_HANDLE } );

        // Setup debug label drawcall
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eDebugLabel;
        // Extend lifetime of name string to be able to print it later
        drawcall.m_Payload.m_DebugLabel.m_pName = strdup( pName );
        // Copy label color
        std::memcpy( drawcall.m_Payload.m_DebugLabel.m_Color, color, 16 );

        GetCurrentPipeline().template AppendDrawcall<true /*explicit*/>( drawcall );
    }

    /***********************************************************************************\

    Function:
        ExecuteCommands

    Description:
        Record secondary command buffers.

    \***********************************************************************************/
    void ProfilerCommandBuffer::ExecuteCommands( uint32_t count, const VkCommandBuffer* pCommandBuffers )
    {
        CpuScopedTimestampCounter<std::chrono::nanoseconds> counter( m_Data.m_ProfilerCPUOverheadNs );

        // Ensure there is a render pass and subpass with VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS flag
        SetupCommandBufferForSecondaryBuffers();

        auto& currentRenderPass = m_Data.m_RenderPasses.back();
        auto& currentSubpass = currentRenderPass.m_Subpasses.back();

        for( uint32_t i = 0; i < count; ++i )
        {
            currentSubpass.m_SecondaryCommandBuffers.push_back( { pCommandBuffers[ i ], VK_COMMAND_BUFFER_LEVEL_SECONDARY } );
        }
    }

    /***********************************************************************************\

    Function:
        OnPipelineBarrier

    Description:
        Stores barrier statistics in currently profiled entity.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PipelineBarrier(
        uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers,
        uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers,
        uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers )
    {
        CpuScopedTimestampCounter<std::chrono::nanoseconds> counter( m_Data.m_ProfilerCPUOverheadNs );

        // Pipeline barriers can occur only outside of the render pass, increment command buffer stats
        m_Data.m_Stats.m_PipelineBarrierCount
            += memoryBarrierCount + bufferMemoryBarrierCount + imageMemoryBarrierCount;
    }

    /***********************************************************************************\

    Function:
        GetData

    Description:
        Reads all queried timestamps. Waits if any timestamp is not available yet.
        Returns structure containing ordered list of timestamps and statistics.

    \***********************************************************************************/
    DeviceProfilerCommandBufferData ProfilerCommandBuffer::GetData()
    {
        if( m_Dirty && !m_QueryPools.empty() )
        {
            CpuScopedTimestampCounter<std::chrono::nanoseconds> counter( m_Data.m_ProfilerCPUOverheadNs );

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
                VkResult result = m_Profiler.m_pDevice->Callbacks.GetQueryPoolResults(
                    m_Profiler.m_pDevice->Handle,
                    m_QueryPools[i],
                    0, numQueriesInPool,
                    dataSize,
                    collectedQueries.data() + dataOffset,
                    sizeof( uint64_t ),
                    VK_QUERY_RESULT_64_BIT );

                assert( result == VK_SUCCESS );

                numQueriesLeft -= numQueriesInPool;
                dataOffset += numQueriesInPool;
            }

            // Update timestamp stats for each profiled entity
            if( collectedQueries.size() > 1 )
            {
                size_t currentQueryIndex = 1;

                // Reset accumulated cycle count if buffer is being reused
                m_Data.m_Ticks = 0;

                for( auto& renderPass : m_Data.m_RenderPasses )
                {
                    // Reset accumulated cycle count if buffer is being reused
                    renderPass.m_Ticks = 0;

                    if( renderPass.m_Handle != VK_NULL_HANDLE )
                    {
                        // Valid render pass begins with vkCmdBeginRenderPass, which should have
                        // 2 queries for initial transitions and clears.
                        assert( currentQueryIndex < collectedQueries.size() );

                        // Get vkCmdBeginRenderPass time
                        renderPass.m_BeginTicks = collectedQueries[ currentQueryIndex ] - collectedQueries[ currentQueryIndex - 1 ];
                        currentQueryIndex += 2;

                        // Include this time in total render pass time
                        renderPass.m_Ticks += renderPass.m_BeginTicks;
                    }

                    for( auto& subpass : renderPass.m_Subpasses )
                    {
                        // Reset accumulated cycle count if buffer is being reused
                        subpass.m_Ticks = 0;

                        if( subpass.m_Contents == VK_SUBPASS_CONTENTS_INLINE )
                        {
                            for( auto& pipeline : subpass.m_Pipelines )
                            {
                                // Reset accumulated cycle count if buffer is being reused
                                pipeline.m_Ticks = 0;

                                for( auto& drawcall : pipeline.m_Drawcalls )
                                {
                                    // Don't collect data for debug labels
                                    if( drawcall.m_Type != DeviceProfilerDrawcallType::eDebugLabel )
                                    {
                                        // Update drawcall time
                                        drawcall.m_Ticks = collectedQueries[ currentQueryIndex ] - collectedQueries[ currentQueryIndex - 1 ];
                                        // Update pipeline time
                                        pipeline.m_Ticks += drawcall.m_Ticks;
                                        // Each drawcall has begin and end query
                                        currentQueryIndex += 2;
                                    }
                                }

                                // Update subpass stats
                                subpass.m_Stats.Add( pipeline.m_Stats );
                                // Update subpass time
                                subpass.m_Ticks += pipeline.m_Ticks;
                            }
                        }
                        else if( subpass.m_Contents == VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS )
                        {
                            for( auto& commandBuffer : subpass.m_SecondaryCommandBuffers )
                            {
                                VkCommandBuffer handle = commandBuffer.m_Handle;

                                // Collect secondary command buffer data
                                commandBuffer = m_Profiler.GetCommandBuffer( handle ).GetData();
                                assert( commandBuffer.m_Handle == handle );

                                // Update subpass stats
                                subpass.m_Stats.Add( commandBuffer.m_Stats );
                                // Update subpass time
                                subpass.m_Ticks += commandBuffer.m_Ticks;
                            }

                            // Move to the next query
                            currentQueryIndex++;
                        }
                        else
                        {
                            ProfilerPlatformFunctions::WriteDebug(
                                "%s - Unsupported VkSubpassContents enum value (%u)\n",
                                __FUNCTION__,
                                subpass.m_Contents );
                        }

                        // Update render pass stats
                        renderPass.m_Stats.Add( subpass.m_Stats );
                        // Update render pass time
                        renderPass.m_Ticks += subpass.m_Ticks;
                    }

                    if( renderPass.m_Handle != VK_NULL_HANDLE )
                    {
                        // Valid render pass ends with vkCmdEndRenderPass, which should have
                        // 2 queries for final transitions and resolves.
                        assert( currentQueryIndex < collectedQueries.size() );

                        // Get vkCmdEndRenderPass time
                        renderPass.m_EndTicks = collectedQueries[ currentQueryIndex ] - collectedQueries[ currentQueryIndex - 1 ];
                        currentQueryIndex += 2;

                        // Include this time in total render pass time
                        renderPass.m_Ticks += renderPass.m_EndTicks;
                    }

                    // Update command buffer stats
                    m_Data.m_Stats.Add( renderPass.m_Stats );
                    // Update command buffer time
                    m_Data.m_Ticks += renderPass.m_Ticks;
                }
            }

            // Read vendor-specific data
            if( m_PerformanceQueryPoolINTEL )
            {
                const size_t reportSize = m_Profiler.m_MetricsApiINTEL.GetReportSize();

                m_Data.m_PerformanceQueryReportINTEL.resize( reportSize );

                VkResult result = m_Profiler.m_pDevice->Callbacks.GetQueryPoolResults(
                    m_Profiler.m_pDevice->Handle,
                    m_PerformanceQueryPoolINTEL,
                    0, 1, reportSize,
                    m_Data.m_PerformanceQueryReportINTEL.data(),
                    reportSize, 0 );

                if( result != VK_SUCCESS )
                {
                    // No data available
                    m_Data.m_PerformanceQueryReportINTEL.clear();
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
        CpuScopedTimestampCounter<std::chrono::nanoseconds> counter( m_Data.m_ProfilerCPUOverheadNs );

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
        EndSubpass

    Description:
        Marks end of current render pass subpass.

    \***********************************************************************************/
    void ProfilerCommandBuffer::EndSubpass()
    {
        // Render pass must be already tracked
        assert( m_pCurrentRenderPass );
        assert( !m_Data.m_RenderPasses.empty() );

        DeviceProfilerRenderPassData& currentRenderPassData = m_Data.m_RenderPasses.back();

        if( m_CurrentSubpassIndex != -1 )
        {
            // Check if any attachments are resolved at the end of current subpass
            currentRenderPassData.m_Stats.m_ResolveCount +=
                m_pCurrentRenderPass->m_Subpasses[ m_CurrentSubpassIndex ].m_ResolveCount;
        }

        // Send new timestamp query after secondary command buffer subpass to subtract
        // time spent in the command buffer from the next subpass
        if( !currentRenderPassData.m_Subpasses.empty() &&
            currentRenderPassData.m_Subpasses.back().m_Contents == VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS )
        {
            SendTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
        }
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

            assert( m_CurrentQueryPoolIndex < m_QueryPools.size() );
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
    void ProfilerCommandBuffer::SetupCommandBufferForStatCounting( const DeviceProfilerPipeline& pipeline )
    {
        // Check if we're in render pass
        if( !m_pCurrentRenderPassData )
        {
            m_Data.m_RenderPasses.push_back( { VK_NULL_HANDLE } );
            m_pCurrentRenderPassData = &m_Data.m_RenderPasses.back();
        }

        // Check if current subpass allows inline commands
        if( m_pCurrentRenderPassData->m_Subpasses.empty() ||
            m_pCurrentRenderPassData->m_Subpasses.back().m_Contents != VK_SUBPASS_CONTENTS_INLINE )
        {
            m_pCurrentRenderPassData->m_Subpasses.push_back( { m_CurrentSubpassIndex, VK_SUBPASS_CONTENTS_INLINE } );
        }

        DeviceProfilerSubpassData& currentSubpass = m_pCurrentRenderPassData->m_Subpasses.back();

        // Check if we're in pipeline
        if( currentSubpass.m_Pipelines.empty() ||
            (currentSubpass.m_Pipelines.back().m_Handle != pipeline.m_Handle) )
        {
            currentSubpass.m_Pipelines.push_back( pipeline );
        }
    }

    /***********************************************************************************\

    Function:
        SetupCommandBufferForSecondaryCommandBuffers

    Description:

    \***********************************************************************************/
    void ProfilerCommandBuffer::SetupCommandBufferForSecondaryBuffers()
    {
        // Check if we're in render pass
        if( !m_pCurrentRenderPassData )
        {
            m_Data.m_RenderPasses.push_back( { VK_NULL_HANDLE } );
            m_pCurrentRenderPassData = &m_Data.m_RenderPasses.back();
        }

        // Check if current subpass allows secondary command buffers
        if( m_pCurrentRenderPassData->m_Subpasses.empty() ||
            m_pCurrentRenderPassData->m_Subpasses.back().m_Contents != VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS )
        {
            m_pCurrentRenderPassData->m_Subpasses.push_back( { m_CurrentSubpassIndex, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS } );
        }
    }

    /***********************************************************************************\

    Function:
        GetCurrentPipeline

    Description:
        Get currently profiled pipeline.

    \***********************************************************************************/
    DeviceProfilerPipelineData& ProfilerCommandBuffer::GetCurrentPipeline()
    {
        assert( m_pCurrentRenderPassData );
        assert( !m_pCurrentRenderPassData->m_Subpasses.empty() );

        DeviceProfilerSubpassData& currentSubpass = m_pCurrentRenderPassData->m_Subpasses.back();

        assert( !currentSubpass.m_Pipelines.empty() );
        assert( currentSubpass.m_Contents == VK_SUBPASS_CONTENTS_INLINE );

        return currentSubpass.m_Pipelines.back();
    }
}
