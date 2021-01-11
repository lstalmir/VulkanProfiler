// Copyright (c) 2020 Lukasz Stalmirski
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

#include "profiler_command_buffer.h"
#include "profiler.h"
#include "profiler_helpers.h"
#include <algorithm>
#include <assert.h>

namespace
{
    template<typename T, typename U>
    inline static void UpdateTimestamps( T& parent, const U& child )
    {
        // If parent begin timestamp is undefined, use first defined child begin timestamp
        if( parent.m_BeginTimestamp == 0 )
        {
            parent.m_BeginTimestamp = child.m_BeginTimestamp;
        }
        // If child end timestamp is defined, it must be larger than current parent end timestamp
        if( child.m_EndTimestamp > 0 )
        {
            parent.m_EndTimestamp = child.m_EndTimestamp;
        }
    }
}

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
        , m_SecondaryCommandBuffers()
        , m_QueryPools()
        , m_QueryPoolSize( 4096 )
        , m_CurrentQueryPoolIndex( -1 )
        , m_CurrentQueryIndex( -1 )
        , m_PerformanceQueryPoolINTEL( VK_NULL_HANDLE )
        , m_Stats()
        , m_Data()
        , m_pCurrentRenderPass( nullptr )
        , m_pCurrentRenderPassData( nullptr )
        , m_CurrentSubpassIndex( -1 )
        , m_GraphicsPipeline()
        , m_ComputePipeline()
    {
        m_Data.m_Handle = commandBuffer;
        m_Data.m_Level = level;

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

        // Secondary command buffers will be executed as well
        for( VkCommandBuffer commandBuffer : m_SecondaryCommandBuffers )
        {
            // Use direct access - m_CommandBuffers map is already locked
            m_Profiler.m_CommandBuffers.at( commandBuffer ).Submit();
        }
    }

    /***********************************************************************************\

    Function:
        Begin

    Description:
        Marks beginning of command buffer.

    \***********************************************************************************/
    void ProfilerCommandBuffer::Begin( const VkCommandBufferBeginInfo* pBeginInfo )
    {
        // Restore initial state
        Reset( 0 /*flags*/ );

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

        // Move to the first query pool
        m_CurrentQueryPoolIndex++;

        if( pBeginInfo->flags & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT )
        {
            // Setup render pass and subpass for commands
            SetupCommandBufferForStatCounting( { VK_NULL_HANDLE } );
        }

        // Begin collection of vendor metrics
        if( m_PerformanceQueryPoolINTEL )
        {
            m_Profiler.m_pDevice->Callbacks.CmdResetQueryPool(
                m_CommandBuffer,
                m_PerformanceQueryPoolINTEL, 0, 1 );

            m_Profiler.m_pDevice->Callbacks.CmdBeginQuery(
                m_CommandBuffer,
                m_PerformanceQueryPoolINTEL, 0, 0 );
        }

        // Send global timestamp query for the whole command buffer
        SendTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
    }

    /***********************************************************************************\

    Function:
        End

    Description:
        Marks end of command buffer.

    \***********************************************************************************/
    void ProfilerCommandBuffer::End()
    {
        // Send global timestamp query for the whole command buffer
        SendTimestampQuery( VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );

        if( m_PerformanceQueryPoolINTEL )
        {
            m_Profiler.m_pDevice->Callbacks.CmdEndQuery(
                m_CommandBuffer,
                m_PerformanceQueryPoolINTEL, 0 );
        }
    }

    /***********************************************************************************\

    Function:
        Reset

    Description:
        Stop profiling of the command buffer.

    \***********************************************************************************/
    void ProfilerCommandBuffer::Reset( VkCommandBufferResetFlags flags )
    {
        // Reset data
        m_Stats = {};
        m_Data.m_RenderPasses.clear();
        m_SecondaryCommandBuffers.clear();

        m_CurrentSubpassIndex = -1;
        m_pCurrentRenderPass = nullptr;
        m_pCurrentRenderPassData = nullptr;

        m_CurrentQueryIndex = -1;
        m_CurrentQueryPoolIndex = -1;

        m_Dirty = false;
    }

    /***********************************************************************************\

    Function:
        PreBeginRenderPass

    Description:
        Marks beginning of next render pass.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PreBeginRenderPass( const VkRenderPassBeginInfo* pBeginInfo, VkSubpassContents )
    {
        m_pCurrentRenderPass = &m_Profiler.GetRenderPass( pBeginInfo->renderPass );

        DeviceProfilerRenderPassData profilerRenderPassData;
        profilerRenderPassData.m_Handle = pBeginInfo->renderPass;

        m_Data.m_RenderPasses.push_back( profilerRenderPassData );

        // Clears issued when render pass begins
        m_Stats.m_ClearColorCount += m_pCurrentRenderPass->m_ClearColorAttachmentCount;
        m_Stats.m_ClearDepthStencilCount += m_pCurrentRenderPass->m_ClearDepthStencilAttachmentCount;

        // Check if we're running out of current query pool
        if( m_CurrentQueryPoolIndex + 1 == m_QueryPools.size() &&
            m_CurrentQueryIndex + 1 > m_QueryPoolSize * 0.85 )
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
        SendTimestampQuery( VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );

        m_pCurrentRenderPassData = &m_Data.m_RenderPasses.back();

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
        PreCommand

    Description:
        Marks beginning of next drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PreCommand( const DeviceProfilerDrawcall& drawcall )
    {
        const DeviceProfilerPipelineType pipelineType = drawcall.GetPipelineType();

        // Setup pipeline
        switch( pipelineType )
        {
        case DeviceProfilerPipelineType::eNone:
        case DeviceProfilerPipelineType::eDebug:
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
        GetCurrentPipeline().m_Drawcalls.push_back( drawcall );

        // Increment drawcall stats
        IncrementStat( drawcall );

        // Begin timestamp query
        SendTimestampQuery( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
    }

    /***********************************************************************************\

    Function:
        PostCommand

    Description:
        Marks end of current drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PostCommand( const DeviceProfilerDrawcall& drawcall )
    {
        // End timestamp query
        // Debug labels have 0 duration, so there is no need for the second query
        if( drawcall.GetPipelineType() != DeviceProfilerPipelineType::eDebug )
        {
            SendTimestampQuery( VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );
        }
    }

    /***********************************************************************************\

    Function:
        ExecuteCommands

    Description:
        Record secondary command buffers.

    \***********************************************************************************/
    void ProfilerCommandBuffer::ExecuteCommands( uint32_t count, const VkCommandBuffer* pCommandBuffers )
    {
        // Secondary command buffers must be executed on primary command buffers
        assert( m_Level == VK_COMMAND_BUFFER_LEVEL_PRIMARY );

        // Ensure there is a render pass and subpass with VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS flag
        SetupCommandBufferForSecondaryBuffers();

        auto& currentRenderPass = m_Data.m_RenderPasses.back();
        auto& currentSubpass = currentRenderPass.m_Subpasses.back();

        for( uint32_t i = 0; i < count; ++i )
        {
            currentSubpass.m_SecondaryCommandBuffers.push_back( { pCommandBuffers[ i ], VK_COMMAND_BUFFER_LEVEL_SECONDARY } );

            // Add command buffer reference
            m_SecondaryCommandBuffers.insert( pCommandBuffers[ i ] );
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
        // Pipeline barriers can occur only outside of the render pass, increment command buffer stats
        m_Stats.m_PipelineBarrierCount += memoryBarrierCount + bufferMemoryBarrierCount + imageMemoryBarrierCount;
    }

    /***********************************************************************************\

    Function:
        GetData

    Description:
        Reads all queried timestamps. Waits if any timestamp is not available yet.
        Returns structure containing ordered list of timestamps and statistics.

    \***********************************************************************************/
    const DeviceProfilerCommandBufferData& ProfilerCommandBuffer::GetData()
    {
        if( m_Dirty && !m_QueryPools.empty() )
        {
            // Reset accumulated stats if buffer is being reused
            m_Data.m_Stats = m_Stats;
            m_Data.m_PerformanceQueryReportINTEL = {};

            // Calculate number of queried timestamps
            const uint32_t numQueries =
                (m_QueryPoolSize * m_CurrentQueryPoolIndex) +
                (m_CurrentQueryIndex + 1);

            if( numQueries > 0 )
            {
                std::vector<uint64_t> collectedQueries( numQueries );

                // Count how many queries we need to get
                uint32_t numQueriesLeft = numQueries;
                uint32_t dataOffset = 0;

                // Collect queried timestamps
                for( uint32_t i = 0; i < m_CurrentQueryPoolIndex + 1; ++i )
                {
                    const uint32_t numQueriesInPool = std::min( m_QueryPoolSize, numQueriesLeft );
                    const uint32_t dataSize = numQueriesInPool * sizeof( uint64_t );

                    assert( dataSize > 0 );

                    // Get results from next query pool
                    m_Profiler.m_pDevice->Callbacks.GetQueryPoolResults(
                        m_Profiler.m_pDevice->Handle,
                        m_QueryPools[ i ],
                        0, numQueriesInPool,
                        dataSize,
                        collectedQueries.data() + dataOffset,
                        sizeof( uint64_t ),
                        VK_QUERY_RESULT_64_BIT );

                    numQueriesLeft -= numQueriesInPool;
                    dataOffset += numQueriesInPool;
                }

                size_t currentQueryIndex = 1;

                // Read global timestamp values
                m_Data.m_BeginTimestamp = collectedQueries.front();
                m_Data.m_EndTimestamp = collectedQueries.back();

                for( auto& renderPass : m_Data.m_RenderPasses )
                {
                    // Reset accumulated cycle count if buffer is being reused
                    renderPass.m_BeginTimestamp = 0;
                    renderPass.m_EndTimestamp = 0;

                    if( renderPass.m_Handle != VK_NULL_HANDLE )
                    {
                        // Valid render pass begins with vkCmdBeginRenderPass, which should have
                        // 2 queries for initial transitions and clears.
                        assert( currentQueryIndex < collectedQueries.size() );

                        // Update render pass begin timestamp
                        renderPass.m_BeginTimestamp = collectedQueries[ currentQueryIndex ];

                        // Get vkCmdBeginRenderPass time
                        renderPass.m_Begin.m_BeginTimestamp = collectedQueries[ currentQueryIndex ];
                        renderPass.m_Begin.m_EndTimestamp = collectedQueries[ currentQueryIndex + 1 ];

                        // Move to the next query
                        currentQueryIndex += 2;
                    }

                    for( auto& subpass : renderPass.m_Subpasses )
                    {
                        // Reset accumulated cycle count if buffer is being reused
                        subpass.m_BeginTimestamp = 0;
                        subpass.m_EndTimestamp = 0;

                        if( subpass.m_Contents == VK_SUBPASS_CONTENTS_INLINE )
                        {
                            for( auto& pipeline : subpass.m_Pipelines )
                            {
                                // Reset accumulated cycle count if buffer is being reused
                                pipeline.m_BeginTimestamp = 0;
                                pipeline.m_EndTimestamp = 0;

                                for( auto& drawcall : pipeline.m_Drawcalls )
                                {
                                    // Don't collect data for debug labels
                                    if( drawcall.GetPipelineType() != DeviceProfilerPipelineType::eDebug )
                                    {
                                        // Update drawcall timestamps
                                        drawcall.m_BeginTimestamp = collectedQueries[ currentQueryIndex ];
                                        drawcall.m_EndTimestamp = collectedQueries[ currentQueryIndex + 1 ];

                                        // Each drawcall has begin and end query
                                        currentQueryIndex += 2;
                                    }
                                    else
                                    {
                                        // Provide timestamps for debug commands
                                        drawcall.m_BeginTimestamp = collectedQueries[ currentQueryIndex ];
                                        drawcall.m_EndTimestamp = drawcall.m_BeginTimestamp;

                                        // Debug drawcalls have only begin query
                                        currentQueryIndex += 1;
                                    }

                                    // Propagate timestamps from drawcall to pipeline
                                    UpdateTimestamps( pipeline, drawcall );
                                }

                                // Propagate timestamps from pipeline to subpass
                                UpdateTimestamps( subpass, pipeline );
                            }
                        }

                        else if( subpass.m_Contents == VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS )
                        {
                            for( auto& commandBuffer : subpass.m_SecondaryCommandBuffers )
                            {
                                VkCommandBuffer handle = commandBuffer.m_Handle;
                                ProfilerCommandBuffer& profilerCommandBuffer = m_Profiler.m_CommandBuffers.unsafe_at( handle );

                                // Collect secondary command buffer data
                                commandBuffer = profilerCommandBuffer.GetData();
                                assert( commandBuffer.m_Handle == handle );

                                // Include profiling time of the secondary command buffer
                                m_Data.m_ProfilerCpuOverheadNs += commandBuffer.m_ProfilerCpuOverheadNs;

                                // Propagate timestamps from command buffer to subpass
                                UpdateTimestamps( subpass, commandBuffer );

                                // Collect secondary command buffer stats
                                m_Data.m_Stats += commandBuffer.m_Stats;
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

                        // Propagate timestamps from subpass to render pass
                        UpdateTimestamps( renderPass, subpass );
                    }

                    if( renderPass.m_Handle != VK_NULL_HANDLE )
                    {
                        // Valid render pass ends with vkCmdEndRenderPass, which should have
                        // 2 queries for final transitions and resolves.
                        assert( currentQueryIndex < collectedQueries.size() );

                        // Get vkCmdEndRenderPass time
                        renderPass.m_End.m_BeginTimestamp = collectedQueries[ currentQueryIndex ];
                        renderPass.m_End.m_EndTimestamp = collectedQueries[ currentQueryIndex + 1 ];

                        // Update render pass end timestamp
                        renderPass.m_EndTimestamp = collectedQueries[ currentQueryIndex + 1 ];

                        // Move to the next query
                        currentQueryIndex += 2;
                    }
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
            m_Stats.m_ResolveCount += m_pCurrentRenderPass->m_Subpasses[ m_CurrentSubpassIndex ].m_ResolveCount;
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
        IncrementStat

    Description:
        Increment drawcall stats for given drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::IncrementStat( const DeviceProfilerDrawcall& drawcall )
    {
        switch( drawcall.m_Type )
        {
        case DeviceProfilerDrawcallType::eDraw:
        case DeviceProfilerDrawcallType::eDrawIndexed:              m_Stats.m_DrawCount++; break;
        case DeviceProfilerDrawcallType::eDrawIndirect:
        case DeviceProfilerDrawcallType::eDrawIndexedIndirect:
        case DeviceProfilerDrawcallType::eDrawIndirectCount:
        case DeviceProfilerDrawcallType::eDrawIndexedIndirectCount: m_Stats.m_DrawIndirectCount++; break;
        case DeviceProfilerDrawcallType::eDispatch:                 m_Stats.m_DispatchCount++; break;
        case DeviceProfilerDrawcallType::eDispatchIndirect:         m_Stats.m_DispatchIndirectCount++; break;
        case DeviceProfilerDrawcallType::eCopyBuffer:               m_Stats.m_CopyBufferCount++; break;
        case DeviceProfilerDrawcallType::eCopyBufferToImage:        m_Stats.m_CopyBufferToImageCount++; break;
        case DeviceProfilerDrawcallType::eCopyImage:                m_Stats.m_CopyImageCount++; break;
        case DeviceProfilerDrawcallType::eCopyImageToBuffer:        m_Stats.m_CopyImageToBufferCount++; break;
        case DeviceProfilerDrawcallType::eClearAttachments:         m_Stats.m_ClearColorCount += drawcall.m_Payload.m_ClearAttachments.m_Count; break;
        case DeviceProfilerDrawcallType::eClearColorImage:          m_Stats.m_ClearColorCount++; break;
        case DeviceProfilerDrawcallType::eClearDepthStencilImage:   m_Stats.m_ClearDepthStencilCount++; break;
        case DeviceProfilerDrawcallType::eResolveImage:             m_Stats.m_ResolveCount++; break;
        case DeviceProfilerDrawcallType::eBlitImage:                m_Stats.m_BlitImageCount++; break;
        case DeviceProfilerDrawcallType::eFillBuffer:               m_Stats.m_FillBufferCount++; break;
        case DeviceProfilerDrawcallType::eUpdateBuffer:             m_Stats.m_UpdateBufferCount++; break;
        default: assert( !"IncrementStat(...) called with unknown drawcall type" );
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

        if( m_CurrentQueryIndex == m_QueryPoolSize )
        {
            // Try to reuse next query pool
            m_CurrentQueryIndex = 0;
            m_CurrentQueryPoolIndex++;

            if( m_CurrentQueryPoolIndex == m_QueryPools.size() )
            {
                // If command buffer is not in render pass we must allocate next query pool now
                // Otherwise something went wrong in PreBeginRenderPass
                assert( !m_pCurrentRenderPass );

                AllocateQueryPool();
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
