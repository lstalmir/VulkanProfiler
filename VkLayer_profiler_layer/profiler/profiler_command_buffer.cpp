// Copyright (c) 2019-2021 Lukasz Stalmirski
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

namespace Profiler
{
    /***********************************************************************************\

    Function:
        ProfilerCommandBuffer

    Description:
        Constructor.

    \***********************************************************************************/
    ProfilerCommandBuffer::ProfilerCommandBuffer( DeviceProfiler& profiler, DeviceProfilerCommandPool& commandPool, VkCommandBuffer commandBuffer, VkCommandBufferLevel level )
        : m_Profiler( profiler )
        , m_CommandPool( commandPool )
        , m_CommandBuffer( commandBuffer )
        , m_Level( level )
        , m_Dirty( false )
        , m_ProfilingEnabled( true )
        , m_SecondaryCommandBuffers()
        , m_pQueryPool( nullptr )
        , m_Stats()
        , m_Data()
        , m_pCurrentRenderPass( nullptr )
        , m_pCurrentRenderPassData( nullptr )
        , m_pCurrentSubpassData( nullptr )
        , m_pCurrentPipelineData( nullptr )
        , m_pCurrentDrawcallData( nullptr )
        , m_CurrentSubpassIndex( -1 )
        , m_GraphicsPipeline()
        , m_ComputePipeline()
    {
        m_Data.m_Handle = commandBuffer;
        m_Data.m_Level = level;

        // Profile the command buffer only if it will be submitted to the queue supporting graphics or compute commands
        // This is requirement of vkCmdResetQueryPool (VUID-vkCmdResetQueryPool-commandBuffer-cmdpool)
        if( (m_CommandPool.GetCommandQueueFlags() & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == 0 )
        {
            m_ProfilingEnabled = false;
        }

        // Initialize performance query once
        if( m_ProfilingEnabled )
        {
            m_pQueryPool = new CommandBufferQueryPool( m_Profiler, m_Level );
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
        delete m_pQueryPool;
    }

    /***********************************************************************************\

    Function:
        GetCommandPool

    Description:
        Returns VkCommandPool object associated with this instance.

    \***********************************************************************************/
    DeviceProfilerCommandPool& ProfilerCommandBuffer::GetCommandPool() const
    {
        return m_CommandPool;
    }

    /***********************************************************************************\

    Function:
        GetCommandBuffer

    Description:
        Returns VkCommandBuffer object associated with this instance.

    \***********************************************************************************/
    VkCommandBuffer ProfilerCommandBuffer::GetHandle() const
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
        if( m_ProfilingEnabled )
        {
            // Contents of the command buffer did not change, but all queries will be executed again
            m_Dirty = true;
        }

        // Secondary command buffers will be executed as well
        for( VkCommandBuffer commandBuffer : m_SecondaryCommandBuffers )
        {
            // Use direct access - m_CommandBuffers map is already locked
            m_Profiler.m_pCommandBuffers.at( commandBuffer )->Submit();
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
        if( m_ProfilingEnabled )
        {
            // Restore initial state
            Reset( 0 /*flags*/ );

            if( pBeginInfo->flags & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT )
            {
                // Setup render pass and subpass for commands
                SetupCommandBufferForStatCounting( { VK_NULL_HANDLE } );
            }

            // Reset query pools.
            m_pQueryPool->Reset( m_CommandBuffer );

            // Make sure there is at least one query pool available.
            m_pQueryPool->PreallocateQueries( m_CommandBuffer );

            // Begin collection of vendor metrics.
            if( auto performanceQueryPool = m_pQueryPool->GetPerformanceQueryPoolHandle() )
            {
                m_Profiler.m_pDevice->Callbacks.CmdResetQueryPool(
                    m_CommandBuffer,
                    performanceQueryPool, 0, 1 );

                m_Profiler.m_pDevice->Callbacks.CmdBeginQuery(
                    m_CommandBuffer,
                    performanceQueryPool, 0, 0 );
            }

            // Send global timestamp query for the whole command buffer.
            m_Data.m_BeginTimestamp = m_pQueryPool->WriteTimestamp( m_CommandBuffer );
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
        if( m_ProfilingEnabled )
        {
            // Send global timestamp query for the whole command buffer.
            m_Data.m_EndTimestamp =
                m_pQueryPool->WriteTimestamp( m_CommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );

            if( (m_pCurrentRenderPassData != nullptr) &&
                (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_RENDER_PASS_EXT) )
            {
                uint64_t lastTimestampInRenderPassIndex =
                    m_Data.m_EndTimestamp;

                if( m_Profiler.m_Config.m_SamplingMode == VK_PROFILER_MODE_PER_DRAWCALL_EXT )
                {
                    lastTimestampInRenderPassIndex =
                        m_pCurrentPipelineData->m_Drawcalls.back().m_EndTimestamp;
                }

                // Update an ending timestamp for the previous render pass.
                m_pCurrentRenderPassData->m_EndTimestamp = lastTimestampInRenderPassIndex;
                m_pCurrentSubpassData->m_EndTimestamp = lastTimestampInRenderPassIndex;

                // Update pipeline end timestamp index.
                if( ( m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_PIPELINE_EXT ) &&
                    ( m_pCurrentPipelineData != nullptr ) )
                {
                    m_pCurrentPipelineData->m_EndTimestamp = lastTimestampInRenderPassIndex;
                }

                // Clear renderpass-scoped pointers.
                m_pCurrentRenderPass = nullptr;
                m_pCurrentRenderPassData = nullptr;
                m_pCurrentSubpassData = nullptr;
                m_pCurrentPipelineData = nullptr;
            }

            // End collection of vendor metrics.
            if( auto performanceQueryPool = m_pQueryPool->GetPerformanceQueryPoolHandle() )
            {
                m_Profiler.m_pDevice->Callbacks.CmdEndQuery(
                    m_CommandBuffer,
                    performanceQueryPool, 0 );
            }

            // Copy query results to the buffers.
            m_pQueryPool->ResolveTimestampsGpu( m_CommandBuffer );
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
        if( m_ProfilingEnabled )
        {
            // Reset data
            m_Stats = {};
            m_Data.m_RenderPasses.clear();
            m_SecondaryCommandBuffers.clear();

            m_CurrentSubpassIndex = -1;
            m_pCurrentRenderPass = nullptr;
            m_pCurrentRenderPassData = nullptr;
            m_pCurrentSubpassData = nullptr;
            m_pCurrentPipelineData = nullptr;
            m_pCurrentDrawcallData = nullptr;

            m_Dirty = false;
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
        if( (m_ProfilingEnabled) &&
            (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_RENDER_PASS_EXT) )
        {
            // End the current render pass, if any.
            if( m_pCurrentRenderPassData != nullptr )
            {
                // Insert a timestamp query at the render pass boundary.
                const uint64_t timestampIndex =
                    m_pQueryPool->WriteTimestamp( m_CommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );

                // Update an ending timestamp for the previous render pass.
                m_pCurrentRenderPassData->m_EndTimestamp = timestampIndex;
                m_pCurrentSubpassData->m_EndTimestamp = timestampIndex;

                // Update pipeline end timestamp index.
                if( (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_PIPELINE_EXT) &&
                    (m_pCurrentPipelineData != nullptr) )
                {
                    m_pCurrentPipelineData->m_EndTimestamp = timestampIndex;
                }

                // Clear renderpass-scoped pointers.
                m_pCurrentRenderPass = nullptr;
                m_pCurrentRenderPassData = nullptr;
                m_pCurrentSubpassData = nullptr;
                m_pCurrentPipelineData = nullptr;
            }

            // Setup pointers for the new render pass.
            m_pCurrentRenderPass = &m_Profiler.GetRenderPass( pBeginInfo->renderPass );
            m_pCurrentRenderPassData = &m_Data.m_RenderPasses.emplace_back();
            m_pCurrentRenderPassData->m_Handle = pBeginInfo->renderPass;
            m_pCurrentRenderPassData->m_Type = m_pCurrentRenderPass->m_Type;

            // Clears issued when render pass begins
            m_Stats.m_ClearColorCount += m_pCurrentRenderPass->m_ClearColorAttachmentCount;
            m_Stats.m_ClearDepthStencilCount += m_pCurrentRenderPass->m_ClearDepthStencilAttachmentCount;

            // Ensure there are free queries that can be used in the render pass.
            // The spec forbids resetting the pools inside the render pass scope, so they have to be allocated now.
            m_pQueryPool->PreallocateQueries( m_CommandBuffer );

            m_pCurrentRenderPassData->m_BeginTimestamp =
                m_pQueryPool->WriteTimestamp( m_CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );

            // Record initial transitions and clears.
            if( (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_PIPELINE_EXT) ||
                ((m_Profiler.m_Config.m_SamplingMode == VK_PROFILER_MODE_PER_RENDER_PASS_EXT) &&
                    m_Profiler.m_Config.m_EnableRenderPassBeginEndProfiling) )
            {
                m_pCurrentRenderPassData->m_Begin.m_BeginTimestamp = m_pCurrentRenderPassData->m_BeginTimestamp;
            }
        }
    }

    /***********************************************************************************\

    Function:
        PostBeginRenderPass

    Description:
        Marks beginning of next render pass.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PostBeginRenderPass( const VkRenderPassBeginInfo*, VkSubpassContents contents )
    {
        if( (m_ProfilingEnabled) &&
            (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_RENDER_PASS_EXT) )
        {
            if( (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_PIPELINE_EXT) ||
                ((m_Profiler.m_Config.m_SamplingMode == VK_PROFILER_MODE_PER_RENDER_PASS_EXT) &&
                    m_Profiler.m_Config.m_EnableRenderPassBeginEndProfiling) )
            {
                m_pCurrentRenderPassData->m_Begin.m_EndTimestamp =
                    m_pQueryPool->WriteTimestamp( m_CommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );
            }

            // Begin first subpass
            NextSubpass( contents );
        }
    }

    /***********************************************************************************\

    Function:
        EndRenderPass

    Description:
        Marks end of current render pass.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PreEndRenderPass()
    {
        if( (m_ProfilingEnabled) &&
            (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_RENDER_PASS_EXT) )
        {
            // End currently profiled subpass
            EndSubpass();

            // Record final transitions and resolves
            if( (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_PIPELINE_EXT) ||
                ((m_Profiler.m_Config.m_SamplingMode == VK_PROFILER_MODE_PER_RENDER_PASS_EXT) &&
                    m_Profiler.m_Config.m_EnableRenderPassBeginEndProfiling) )
            {
                m_pCurrentRenderPassData->m_End.m_BeginTimestamp =
                    m_pQueryPool->WriteTimestamp( m_CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
            }
        }
    }

    /***********************************************************************************\

    Function:
        PostEndRenderPass

    Description:
        Marks end of current render pass.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PostEndRenderPass()
    {
        if( (m_ProfilingEnabled) &&
            (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_RENDER_PASS_EXT) )
        {
            if( (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_PIPELINE_EXT) ||
                ((m_Profiler.m_Config.m_SamplingMode == VK_PROFILER_MODE_PER_RENDER_PASS_EXT) &&
                    m_Profiler.m_Config.m_EnableRenderPassBeginEndProfiling) )
            {
                m_pCurrentRenderPassData->m_End.m_EndTimestamp =
                    m_pQueryPool->WriteTimestamp( m_CommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );

                // Use the end timestamp of the end command.
                m_pCurrentRenderPassData->m_EndTimestamp = m_pCurrentRenderPassData->m_End.m_EndTimestamp;
            }
            else
            {
                // Use the end timestamp of the last subpass in this render pass.
                m_pCurrentRenderPassData->m_EndTimestamp = m_pCurrentRenderPassData->m_Subpasses.back().m_EndTimestamp;
            }

            // No more subpasses in this render pass.
            m_CurrentSubpassIndex = -1;
            m_pCurrentRenderPass = nullptr;
            m_pCurrentRenderPassData = nullptr;
            m_pCurrentSubpassData = nullptr;
            m_pCurrentPipelineData = nullptr;
        }
    }

    /***********************************************************************************\

    Function:
        NextSubpass

    Description:
        Marks beginning of next render pass subpass.

    \***********************************************************************************/
    void ProfilerCommandBuffer::NextSubpass( VkSubpassContents contents )
    {
        if( (m_ProfilingEnabled) &&
            (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_RENDER_PASS_EXT) )
        {
            // End currently profiled subpass before beginning new one.
            EndSubpass();

            // Setup pointers for the next subpass.
            m_pCurrentSubpassData = &m_pCurrentRenderPassData->m_Subpasses.emplace_back();
            m_pCurrentSubpassData->m_Index = ++m_CurrentSubpassIndex;
            m_pCurrentSubpassData->m_Contents = contents;

            // Write begin timestamp of the subpass.
            m_pCurrentSubpassData->m_BeginTimestamp =
                m_pQueryPool->WriteTimestamp( m_CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
        }
    }

    /***********************************************************************************\

    Function:
        BindPipeline

    Description:
        Marks beginning of next render pass pipeline.

    \***********************************************************************************/
    void ProfilerCommandBuffer::BindPipeline( const DeviceProfilerPipeline& pipeline )
    {
        if( m_ProfilingEnabled )
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
    }

    /***********************************************************************************\

    Function:
        PreCommand

    Description:
        Marks beginning of next drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PreCommand( const DeviceProfilerDrawcall& drawcall )
    {
        if( m_ProfilingEnabled )
        {
            const DeviceProfilerPipelineType pipelineType = drawcall.GetPipelineType();

            bool pipelineChanged = false;

            // Insert a timestamp in per-render pass mode if command is recorded outside of the render pass
            // and the internal render pass has changed.
            DeviceProfilerRenderPassData* pPreviousRenderPassData = m_pCurrentRenderPassData;
            DeviceProfilerSubpassData* pPreviousSubpassData = m_pCurrentSubpassData;

            // Save pointer to the previous pipeline to update the end timestamp if needed.
            DeviceProfilerPipelineData* pPreviousPipelineData = m_pCurrentPipelineData;

            // Setup pipeline
            switch( pipelineType )
            {
            case DeviceProfilerPipelineType::eNone:
            case DeviceProfilerPipelineType::eDebug:
                pipelineChanged = SetupCommandBufferForStatCounting( { VK_NULL_HANDLE } );
                break;

            case DeviceProfilerPipelineType::eGraphics:
                pipelineChanged = SetupCommandBufferForStatCounting( m_GraphicsPipeline );
                break;

            case DeviceProfilerPipelineType::eCompute:
                pipelineChanged = SetupCommandBufferForStatCounting( m_ComputePipeline );
                break;

            default: // Internal pipelines
                pipelineChanged = SetupCommandBufferForStatCounting( m_Profiler.GetPipeline( (VkPipeline)pipelineType ) );
                break;
            }

            // Append drawcall to the current pipeline
            m_pCurrentDrawcallData = &m_pCurrentPipelineData->m_Drawcalls.emplace_back( drawcall );

            // Increment drawcall stats
            IncrementStat( drawcall );

            if( (m_Profiler.m_Config.m_SamplingMode == VK_PROFILER_MODE_PER_DRAWCALL_EXT) ||
                ((m_Profiler.m_Config.m_SamplingMode == VK_PROFILER_MODE_PER_PIPELINE_EXT) &&
                    (pipelineChanged)) ||
                ((m_Profiler.m_Config.m_SamplingMode == VK_PROFILER_MODE_PER_RENDER_PASS_EXT) &&
                    (pPreviousRenderPassData != m_pCurrentRenderPassData)) )
            {
                // Begin timestamp query
                const uint64_t timestampIndex =
                    m_pQueryPool->WriteTimestamp( m_CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );

                // Update draw begin timestamp index.
                if( m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_DRAWCALL_EXT )
                {
                    m_pCurrentDrawcallData->m_BeginTimestamp = timestampIndex;
                }

                // Update pipeline begin timestamp index.
                if( (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_PIPELINE_EXT) &&
                    (m_pCurrentPipelineData->m_BeginTimestamp == UINT64_MAX) )
                {
                    m_pCurrentPipelineData->m_BeginTimestamp = timestampIndex;

                    // Update end timestamp of the previous pipeline.
                    if( pPreviousPipelineData != nullptr )
                    {
                        if( m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_DRAWCALL_EXT )
                        {
                            pPreviousPipelineData->m_EndTimestamp =
                                pPreviousPipelineData->m_Drawcalls.back().m_EndTimestamp;
                        }
                        else
                        {
                            pPreviousPipelineData->m_EndTimestamp = timestampIndex;
                        }
                    }
                }

                // Update subpass begin timestamp index.
                if( (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_RENDER_PASS_EXT) &&
                    (m_pCurrentSubpassData->m_BeginTimestamp == UINT64_MAX) )
                {
                    m_pCurrentSubpassData->m_BeginTimestamp = timestampIndex;

                    // Update end timestamp of the previous subpass.
                    if( pPreviousSubpassData != nullptr )
                    {
                        if( (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_PIPELINE_EXT) &&
                            (pPreviousSubpassData->m_Contents == VK_SUBPASS_CONTENTS_INLINE) )
                        {
                            pPreviousSubpassData->m_EndTimestamp =
                                pPreviousSubpassData->m_Pipelines.back().m_EndTimestamp;
                        }
                        else
                        {
                            pPreviousSubpassData->m_EndTimestamp = timestampIndex;
                        }
                    }
                }

                // Update render pass begin timestamp index.
                if( (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_RENDER_PASS_EXT) &&
                    (m_pCurrentRenderPassData->m_BeginTimestamp == UINT64_MAX) )
                {
                    m_pCurrentRenderPassData->m_BeginTimestamp = timestampIndex;

                    // Update end timestamp of the previous render pass.
                    if( pPreviousRenderPassData != nullptr )
                    {
                        pPreviousRenderPassData->m_EndTimestamp = pPreviousSubpassData->m_EndTimestamp;
                    }
                }
            }
        }
    }

    /***********************************************************************************\

    Function:
        PostCommand

    Description:
        Marks end of current drawcall.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PostCommand( const DeviceProfilerDrawcall& drawcall )
    {
        if( m_ProfilingEnabled )
        {
            // End timestamp query
            // Debug labels have 0 duration, so there is no need for the second query
            if( (m_Profiler.m_Config.m_SamplingMode == VK_PROFILER_MODE_PER_DRAWCALL_EXT) &&
                (drawcall.GetPipelineType() != DeviceProfilerPipelineType::eDebug) )
            {
                assert( m_pCurrentDrawcallData );
                m_pCurrentDrawcallData->m_EndTimestamp =
                    m_pQueryPool->WriteTimestamp( m_CommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );
            }
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
        if( m_ProfilingEnabled )
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
        if( m_ProfilingEnabled )
        {
            // Pipeline barriers can occur only outside of the render pass, increment command buffer stats
            m_Stats.m_PipelineBarrierCount += memoryBarrierCount + bufferMemoryBarrierCount + imageMemoryBarrierCount;
        }
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
        if( m_ProfilingEnabled &&
            m_Dirty )
        {
            // Copy query results to the buffers.
            m_pQueryPool->ResolveTimestampsCpu();

            // Reset accumulated stats if buffer is being reused
            m_Data.m_Stats = m_Stats;
            m_Data.m_PerformanceQueryReportINTEL = {};

            // Read global timestamp values
            m_Data.m_BeginTimestamp = m_pQueryPool->GetTimestampData( m_Data.m_BeginTimestamp );

            if( m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_RENDER_PASS_EXT )
            {
                for( auto& renderPass : m_Data.m_RenderPasses )
                {
                    // Update render pass begin timestamp
                    renderPass.m_BeginTimestamp = m_pQueryPool->GetTimestampData( renderPass.m_BeginTimestamp );

                    if( ((m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_PIPELINE_EXT) ||
                        ((m_Profiler.m_Config.m_SamplingMode == VK_PROFILER_MODE_PER_RENDER_PASS_EXT) &&
                            m_Profiler.m_Config.m_EnableRenderPassBeginEndProfiling)) &&
                        (renderPass.m_Handle != VK_NULL_HANDLE) )
                    {
                        // Get vkCmdBeginRenderPass time
                        renderPass.m_Begin.m_BeginTimestamp = m_pQueryPool->GetTimestampData( renderPass.m_Begin.m_BeginTimestamp );
                        renderPass.m_Begin.m_EndTimestamp = m_pQueryPool->GetTimestampData( renderPass.m_Begin.m_EndTimestamp );
                    }

                    for( auto& subpass : renderPass.m_Subpasses )
                    {
                        if( subpass.m_Contents == VK_SUBPASS_CONTENTS_INLINE )
                        {
                            subpass.m_BeginTimestamp = m_pQueryPool->GetTimestampData( subpass.m_BeginTimestamp );

                            if( m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_PIPELINE_EXT )
                            {
                                for( auto& pipeline : subpass.m_Pipelines )
                                {
                                    pipeline.m_BeginTimestamp = m_pQueryPool->GetTimestampData( pipeline.m_BeginTimestamp );

                                    if( m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_DRAWCALL_EXT )
                                    {
                                        for( auto& drawcall : pipeline.m_Drawcalls )
                                        {
                                            // Don't collect data for debug labels
                                            if( drawcall.GetPipelineType() != DeviceProfilerPipelineType::eDebug )
                                            {
                                                // Update drawcall timestamps
                                                drawcall.m_BeginTimestamp = m_pQueryPool->GetTimestampData( drawcall.m_BeginTimestamp );
                                                drawcall.m_EndTimestamp = m_pQueryPool->GetTimestampData( drawcall.m_EndTimestamp );
                                            }
                                            else
                                            {
                                                // Provide timestamps for debug commands
                                                drawcall.m_BeginTimestamp = m_pQueryPool->GetTimestampData( drawcall.m_BeginTimestamp );
                                                drawcall.m_EndTimestamp = drawcall.m_BeginTimestamp;
                                            }
                                        }
                                    }

                                    pipeline.m_EndTimestamp = m_pQueryPool->GetTimestampData( pipeline.m_EndTimestamp );
                                }
                            }

                            subpass.m_EndTimestamp = m_pQueryPool->GetTimestampData( subpass.m_EndTimestamp );
                        }

                        else if( subpass.m_Contents == VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS )
                        {
                            for( auto& commandBuffer : subpass.m_SecondaryCommandBuffers )
                            {
                                VkCommandBuffer handle = commandBuffer.m_Handle;
                                ProfilerCommandBuffer& profilerCommandBuffer = *m_Profiler.m_pCommandBuffers.unsafe_at( handle );

                                // Collect secondary command buffer data
                                commandBuffer = profilerCommandBuffer.GetData();
                                assert( commandBuffer.m_Handle == handle );

                                // Include profiling time of the secondary command buffer
                                m_Data.m_ProfilerCpuOverheadNs += commandBuffer.m_ProfilerCpuOverheadNs;

                                // Propagate timestamps from command buffer to subpass
                                subpass.m_BeginTimestamp = commandBuffer.m_BeginTimestamp;
                                subpass.m_EndTimestamp = commandBuffer.m_EndTimestamp;

                                // Collect secondary command buffer stats
                                m_Data.m_Stats += commandBuffer.m_Stats;
                            }
                        }
                        else
                        {
                            ProfilerPlatformFunctions::WriteDebug(
                                "%s - Unsupported VkSubpassContents enum value (%u)\n",
                                __FUNCTION__,
                                subpass.m_Contents );
                        }
                    }

                    if( ((m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_PIPELINE_EXT) ||
                        ((m_Profiler.m_Config.m_SamplingMode == VK_PROFILER_MODE_PER_RENDER_PASS_EXT) &&
                            m_Profiler.m_Config.m_EnableRenderPassBeginEndProfiling)) &&
                        (renderPass.m_Handle != VK_NULL_HANDLE) )
                    {
                        // Get vkCmdEndRenderPass time
                        renderPass.m_End.m_BeginTimestamp = m_pQueryPool->GetTimestampData( renderPass.m_End.m_BeginTimestamp );
                        renderPass.m_End.m_EndTimestamp = m_pQueryPool->GetTimestampData( renderPass.m_End.m_EndTimestamp );
                    }

                    renderPass.m_EndTimestamp = m_pQueryPool->GetTimestampData( renderPass.m_EndTimestamp );
                }
            }

            volatile const uint64_t endTimestampIndex = m_Data.m_EndTimestamp;

            m_Data.m_EndTimestamp = m_pQueryPool->GetTimestampData( endTimestampIndex );

            if( m_Data.m_EndTimestamp < m_Data.m_BeginTimestamp )
            {
                __debugbreak();
            }

            // Read vendor-specific data
            if( auto performanceQueryPool = m_pQueryPool->GetPerformanceQueryPoolHandle() )
            {
                const size_t reportSize = m_Profiler.m_MetricsApiINTEL.GetReportSize();

                m_Data.m_PerformanceQueryReportINTEL.resize( reportSize );

                VkResult result = m_Profiler.m_pDevice->Callbacks.GetQueryPoolResults(
                    m_Profiler.m_pDevice->Handle,
                    performanceQueryPool,
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
        EndSubpass

    Description:
        Marks end of current render pass subpass.

    \***********************************************************************************/
    void ProfilerCommandBuffer::EndSubpass()
    {
        // Render pass must be already tracked
        assert( m_pCurrentRenderPass );
        assert( !m_Data.m_RenderPasses.empty() );

        if( m_CurrentSubpassIndex != -1 )
        {
            assert( m_pCurrentSubpassData );

            // Check if any attachments are resolved at the end of current subpass
            m_Stats.m_ResolveCount += m_pCurrentRenderPass->m_Subpasses[ m_CurrentSubpassIndex ].m_ResolveCount;

            // Send timestamp query at the end of the subpass.
            if( (m_Profiler.m_Config.m_SamplingMode == VK_PROFILER_MODE_PER_DRAWCALL_EXT) &&
                (m_pCurrentPipelineData) &&
                !(m_pCurrentPipelineData->m_Drawcalls.empty()) )
            {
                m_pCurrentSubpassData->m_EndTimestamp =
                    m_pCurrentPipelineData->m_Drawcalls.back().m_EndTimestamp;
            }
            else
            {
                m_pCurrentSubpassData->m_EndTimestamp =
                    m_pQueryPool->WriteTimestamp( m_CommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );
            }

            // Update timestamp of the last pipeline in the subpass.
            if( (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_PIPELINE_EXT) &&
                (m_pCurrentPipelineData) )
            {
                m_pCurrentPipelineData->m_EndTimestamp = m_pCurrentSubpassData->m_EndTimestamp;
            }

            // Clear per-subpass pointers.
            m_pCurrentSubpassData = nullptr;
            m_pCurrentPipelineData = nullptr;
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
        case DeviceProfilerDrawcallType::eDrawIndexed:
            m_Stats.m_DrawCount++;
            break;
        case DeviceProfilerDrawcallType::eDrawIndirect:
        case DeviceProfilerDrawcallType::eDrawIndexedIndirect:
        case DeviceProfilerDrawcallType::eDrawIndirectCount:
        case DeviceProfilerDrawcallType::eDrawIndexedIndirectCount:
            m_Stats.m_DrawIndirectCount++;
            break;
        case DeviceProfilerDrawcallType::eDispatch:
            m_Stats.m_DispatchCount++;
            break;
        case DeviceProfilerDrawcallType::eDispatchIndirect:
            m_Stats.m_DispatchIndirectCount++;
            break;
        case DeviceProfilerDrawcallType::eCopyBuffer:
            m_Stats.m_CopyBufferCount++;
            break;
        case DeviceProfilerDrawcallType::eCopyBufferToImage:
            m_Stats.m_CopyBufferToImageCount++;
            break;
        case DeviceProfilerDrawcallType::eCopyImage:
            m_Stats.m_CopyImageCount++;
            break;
        case DeviceProfilerDrawcallType::eCopyImageToBuffer:
            m_Stats.m_CopyImageToBufferCount++;
            break;
        case DeviceProfilerDrawcallType::eClearAttachments:
            m_Stats.m_ClearColorCount += drawcall.m_Payload.m_ClearAttachments.m_Count;
            break;
        case DeviceProfilerDrawcallType::eClearColorImage:
            m_Stats.m_ClearColorCount++;
            break;
        case DeviceProfilerDrawcallType::eClearDepthStencilImage:
            m_Stats.m_ClearDepthStencilCount++;
            break;
        case DeviceProfilerDrawcallType::eResolveImage:
            m_Stats.m_ResolveCount++;
            break;
        case DeviceProfilerDrawcallType::eBlitImage:
            m_Stats.m_BlitImageCount++;
            break;
        case DeviceProfilerDrawcallType::eFillBuffer:
            m_Stats.m_FillBufferCount++;
            break;
        case DeviceProfilerDrawcallType::eUpdateBuffer:
            m_Stats.m_UpdateBufferCount++;
            break;
        case DeviceProfilerDrawcallType::eBeginDebugLabel:
        case DeviceProfilerDrawcallType::eEndDebugLabel:
        case DeviceProfilerDrawcallType::eInsertDebugLabel:
            break;
        default:
            assert( !"IncrementStat(...) called with unknown drawcall type" );
        }
    }

    /***********************************************************************************\

    Function:
        SetupCommandBufferForStatCounting

    Description:

    Returns:
        True, if the pipeline has changed.

    \***********************************************************************************/
    bool ProfilerCommandBuffer::SetupCommandBufferForStatCounting( const DeviceProfilerPipeline& pipeline )
    {
        const DeviceProfilerRenderPassType renderPassType = GetRenderPassTypeFromPipelineType( pipeline.m_Type );

        // Check if we're in render pass
        if( !m_pCurrentRenderPassData ||
            (m_pCurrentRenderPassData->m_Type != renderPassType) )
        {
            m_pCurrentRenderPassData = &m_Data.m_RenderPasses.emplace_back();
            m_pCurrentRenderPassData->m_Handle = VK_NULL_HANDLE;
            m_pCurrentRenderPassData->m_Type = renderPassType;

            // Invalidate subpass pointer after changing the render pass.
            m_pCurrentSubpassData = nullptr;
        }

        // Check if current subpass allows inline commands
        if( !m_pCurrentSubpassData ||
            (m_pCurrentSubpassData->m_Contents != VK_SUBPASS_CONTENTS_INLINE) )
        {
            m_pCurrentSubpassData = &m_pCurrentRenderPassData->m_Subpasses.emplace_back();
            m_pCurrentSubpassData->m_Index = m_CurrentSubpassIndex;
            m_pCurrentSubpassData->m_Contents = VK_SUBPASS_CONTENTS_INLINE;
        }

        // Check if we're in pipeline
        if( !m_pCurrentPipelineData ||
            (m_pCurrentPipelineData->m_Handle != pipeline.m_Handle) )
        {
            m_pCurrentPipelineData = &m_pCurrentSubpassData->m_Pipelines.emplace_back( pipeline );
            return true;
        }

        return false;
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
            m_pCurrentRenderPassData = &m_Data.m_RenderPasses.emplace_back();
            m_pCurrentRenderPassData->m_Handle = VK_NULL_HANDLE;
            m_pCurrentRenderPassData->m_Type = DeviceProfilerRenderPassType::eNone;
        }

        // Check if current subpass allows secondary command buffers
        if( !m_pCurrentSubpassData ||
            (m_pCurrentSubpassData->m_Contents != VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS) )
        {
            m_pCurrentSubpassData = &m_pCurrentRenderPassData->m_Subpasses.emplace_back();
            m_pCurrentSubpassData->m_Index = m_CurrentSubpassIndex;
            m_pCurrentSubpassData->m_Contents = VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
        }
    }

    /***********************************************************************************\

    Function:
        GetRenderPassTypeFromPipelineType

    Description:

    \***********************************************************************************/
    DeviceProfilerRenderPassType ProfilerCommandBuffer::GetRenderPassTypeFromPipelineType(
        DeviceProfilerPipelineType pipelineType ) const
    {
        switch( pipelineType )
        {
        case DeviceProfilerPipelineType::eGraphics:
            return DeviceProfilerRenderPassType::eGraphics;
        case DeviceProfilerPipelineType::eCompute:
            return DeviceProfilerRenderPassType::eCompute;
        default:
            return DeviceProfilerRenderPassType::eCopy;
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
