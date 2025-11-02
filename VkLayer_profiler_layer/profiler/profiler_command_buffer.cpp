// Copyright (c) 2019-2025 Lukasz Stalmirski
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
#include "profiler_stat_comparators.h"
#include "profiler_query_pool.h"
#include <algorithm>
#include <assert.h>

#define PROFILER_INDIRECT_ARGS_BUFFER_SIZE 65536

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
        , m_CurrentSubpassIndex( DeviceProfilerSubpassData::ImplicitSubpassIndex )
        , m_GraphicsPipeline()
        , m_ComputePipeline()
        , m_IndirectArgumentBufferList()
    {
        m_Data.m_Handle = m_Profiler.ResolveObjectHandle<VkCommandBufferHandle>( commandBuffer );
        m_Data.m_Level = level;

        // Profile the command buffer only if it will be submitted to the queue supporting graphics or compute commands
        // This is requirement of vkCmdResetQueryPool (VUID-vkCmdResetQueryPool-commandBuffer-cmdpool)
        if( !m_CommandPool.SupportsTimestampQuery() )
        {
            m_ProfilingEnabled = false;
        }

        // Initialize performance query once
        if( m_ProfilingEnabled )
        {
            m_pQueryPool = new CommandBufferQueryPool(
                m_Profiler,
                m_CommandPool.GetQueueFamilyIndex(),
                m_Level );
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
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        if( m_ProfilingEnabled )
        {
            // Contents of the command buffer did not change, but all queries will be executed again
            m_Data.m_DataValid = false;
        }

        // Secondary command buffers will be executed as well
        for( VkCommandBuffer commandBuffer : m_SecondaryCommandBuffers )
        {
            // Use direct access - m_CommandBuffers map is already locked
            m_Profiler.m_pCommandBuffers.unsafe_at( commandBuffer )->Submit();
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
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        if( m_ProfilingEnabled )
        {
            // Restore initial state
            Reset( 0 /*flags*/ );

            // Reset query pools.
            m_pQueryPool->Reset( m_CommandBuffer );

            // Make sure there is at least one query pool available.
            m_pQueryPool->PreallocateQueries( m_CommandBuffer );

            // Begin collection of vendor metrics.
            m_pQueryPool->BeginPerformanceQuery( m_CommandBuffer );

            // Send global timestamp query for the whole command buffer.
            m_Data.m_BeginTimestamp.m_Index = m_pQueryPool->WriteTimestamp( m_CommandBuffer );
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
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        if( m_ProfilingEnabled )
        {
            // Send global timestamp query for the whole command buffer.
            m_Data.m_EndTimestamp.m_Index =
                m_pQueryPool->WriteTimestamp( m_CommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );

            if( (m_pCurrentRenderPassData != nullptr) &&
                (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_RENDER_PASS_EXT) )
            {
                uint64_t lastTimestampInRenderPassIndex =
                    m_Data.m_EndTimestamp.m_Index;

                if( (m_Profiler.m_Config.m_SamplingMode == VK_PROFILER_MODE_PER_DRAWCALL_EXT) &&
                    (m_pCurrentPipelineData != nullptr) &&
                    !m_pCurrentPipelineData->m_Drawcalls.empty() &&
                    (m_pCurrentPipelineData->m_Drawcalls.back().m_EndTimestamp.m_Index != UINT64_MAX) )
                {
                    lastTimestampInRenderPassIndex =
                        m_pCurrentPipelineData->m_Drawcalls.back().m_EndTimestamp.m_Index;
                }

                // Update an ending timestamp for the previous render pass.
                m_pCurrentRenderPassData->m_EndTimestamp.m_Index = lastTimestampInRenderPassIndex;
                m_pCurrentSubpassData->m_EndTimestamp.m_Index = lastTimestampInRenderPassIndex;

                // Update pipeline end timestamp index.
                if( ( m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_PIPELINE_EXT ) &&
                    ( m_pCurrentPipelineData != nullptr ) )
                {
                    m_pCurrentPipelineData->m_EndTimestamp.m_Index = lastTimestampInRenderPassIndex;
                }

                // Clear renderpass-scoped pointers.
                m_pCurrentRenderPass = nullptr;
                m_pCurrentRenderPassData = nullptr;
                m_pCurrentSubpassData = nullptr;
                m_pCurrentPipelineData = nullptr;
            }

            // End collection of vendor metrics.
            m_pQueryPool->EndPerformanceQuery( m_CommandBuffer );

            // Perform deferred indirect argument buffer copies.
            FlushIndirectArgumentCopyLists();
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
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        if( m_ProfilingEnabled )
        {
            // The data must be collected before the command bufer is reused.
            m_Profiler.m_DataAggregator.Aggregate( this );

            // Reset data
            m_Stats = {};
            m_Data.m_RenderPasses.clear();
            m_SecondaryCommandBuffers.clear();

            m_CurrentSubpassIndex = DeviceProfilerSubpassData::ImplicitSubpassIndex;
            m_pCurrentRenderPass = nullptr;
            m_pCurrentRenderPassData = nullptr;
            m_pCurrentSubpassData = nullptr;
            m_pCurrentPipelineData = nullptr;
            m_pCurrentDrawcallData = nullptr;

            m_Data.m_DataValid = false;

            if( m_Profiler.m_Config.m_CaptureIndirectArguments )
            {
                // Reset indirect argument buffers.
                for( auto& buffer : m_IndirectArgumentBufferList )
                {
                    buffer.m_Offset = 0;
                    buffer.m_PendingCopyList.clear();
                }
            }
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
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        if( (m_ProfilingEnabled) &&
            (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_RENDER_PASS_EXT) )
        {
            PreBeginRenderPassCommonProlog();

            // Setup pointers for the new render pass.
            m_pCurrentRenderPass = &m_Profiler.GetRenderPass( pBeginInfo->renderPass );
            m_pCurrentRenderPassData = &m_Data.m_RenderPasses.emplace_back();
            m_pCurrentRenderPassData->m_Handle = m_pCurrentRenderPass->m_Handle;
            m_pCurrentRenderPassData->m_Type = m_pCurrentRenderPass->m_Type;

            // Clears issued when render pass begins
            if( m_pCurrentRenderPass->m_ClearColorAttachmentCount != 0 )
            {
                m_Stats.m_ClearColorStats.m_Count += m_pCurrentRenderPass->m_ClearColorAttachmentCount;
                m_pCurrentRenderPassData->m_ClearsColorAttachments = true;
            }

            if( m_pCurrentRenderPass->m_ClearDepthStencilAttachmentCount != 0 )
            {
                m_Stats.m_ClearDepthStencilStats.m_Count += m_pCurrentRenderPass->m_ClearDepthStencilAttachmentCount;
                m_pCurrentRenderPassData->m_ClearsDepthStencilAttachments = true;
            }

            PreBeginRenderPassCommonEpilog();
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
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        if( (m_ProfilingEnabled) &&
            (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_RENDER_PASS_EXT) )
        {
            if( (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_PIPELINE_EXT) ||
                ((m_Profiler.m_Config.m_SamplingMode == VK_PROFILER_MODE_PER_RENDER_PASS_EXT) &&
                    m_Profiler.m_Config.m_EnableRenderPassBeginEndProfiling) )
            {
                m_pCurrentRenderPassData->m_Begin.m_EndTimestamp.m_Index =
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
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

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
                m_pCurrentRenderPassData->m_End.m_BeginTimestamp.m_Index =
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
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        if( (m_ProfilingEnabled) &&
            (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_RENDER_PASS_EXT) )
        {
            if( (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_PIPELINE_EXT) ||
                ((m_Profiler.m_Config.m_SamplingMode == VK_PROFILER_MODE_PER_RENDER_PASS_EXT) &&
                    m_Profiler.m_Config.m_EnableRenderPassBeginEndProfiling) )
            {
                m_pCurrentRenderPassData->m_End.m_EndTimestamp.m_Index =
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
            m_CurrentSubpassIndex = DeviceProfilerSubpassData::ImplicitSubpassIndex;
            m_pCurrentRenderPass = nullptr;
            m_pCurrentRenderPassData = nullptr;
            m_pCurrentSubpassData = nullptr;
            m_pCurrentPipelineData = nullptr;
        }

        if( (m_ProfilingEnabled) &&
            (m_Profiler.m_Config.m_CaptureIndirectArguments) )
        {
            // Record pending indirect argument buffer copies after the render pass.
            FlushIndirectArgumentCopyLists();
        }
    }

    /***********************************************************************************\

    Function:
        PreBeginRendering

    Description:
        Marks beginning of next render pass (VK_KHR_dynamic_rendering).

    \***********************************************************************************/
    void ProfilerCommandBuffer::PreBeginRendering( const VkRenderingInfo* pRenderingInfo )
    {
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        if( (m_ProfilingEnabled) &&
            (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_RENDER_PASS_EXT) )
        {
            PreBeginRenderPassCommonProlog();

            // Setup pointers for the new render pass.
            m_pCurrentRenderPassData = &m_Data.m_RenderPasses.emplace_back();
            m_pCurrentRenderPassData->m_Handle = VK_NULL_HANDLE;
            m_pCurrentRenderPassData->m_Type = DeviceProfilerRenderPassType::eGraphics;
            m_pCurrentRenderPassData->m_Dynamic = true;

            // Helper function to accumulate common attachment operations.
            auto AccumulateAttachmentOperations = [&](
                const VkRenderingAttachmentInfo* pAttachmentInfo,
                bool& clearFlag,
                DeviceProfilerDrawcallStats::Stats& clearStats,
                DeviceProfilerDrawcallStats::Stats& resolveStats )
            {
                // Attachment operations are ignored if imageView is null.
                if( (pAttachmentInfo != nullptr) &&
                    (pAttachmentInfo->imageView != VK_NULL_HANDLE) )
                {
                    // Count clear operations only if the rendering is not resuming.
                    if( !(pRenderingInfo->flags & VK_RENDERING_RESUMING_BIT) )
                    {
                        if( pAttachmentInfo->loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR )
                        {
                            clearStats.m_Count++;
                            clearFlag = true;
                        }
                    }

                    // Count resolve operations only if the rendering will not be suspended in this command buffer.
                    if( !(pRenderingInfo->flags & VK_RENDERING_SUSPENDING_BIT) )
                    {
                        if( (pAttachmentInfo->resolveMode != VK_RESOLVE_MODE_NONE) &&
                            (pAttachmentInfo->resolveImageView != VK_NULL_HANDLE) )
                        {
                            resolveStats.m_Count++;
                            m_pCurrentRenderPassData->m_ResolvesAttachments = true;
                        }
                    }
                }
            };

            // Clears and resolves issued when rendering begins.
            for( uint32_t i = 0; i < pRenderingInfo->colorAttachmentCount; ++i )
            {
                AccumulateAttachmentOperations(
                    &pRenderingInfo->pColorAttachments[i],
                    m_pCurrentRenderPassData->m_ClearsColorAttachments,
                    m_Stats.m_ClearColorStats,
                    m_Stats.m_ResolveStats );
            }

            AccumulateAttachmentOperations(
                pRenderingInfo->pDepthAttachment,
                m_pCurrentRenderPassData->m_ClearsDepthStencilAttachments,
                m_Stats.m_ClearDepthStencilStats,
                m_Stats.m_ResolveStats );

            AccumulateAttachmentOperations(
                pRenderingInfo->pStencilAttachment,
                m_pCurrentRenderPassData->m_ClearsDepthStencilAttachments,
                m_Stats.m_ClearDepthStencilStats,
                m_Stats.m_ResolveStats );

            PreBeginRenderPassCommonEpilog();
        }
    }
    
    /***********************************************************************************\

    Function:
        PostBeginRendering

    Description:
        Marks beginning of next render pass (VK_KHR_dynamic_rendering).

    \***********************************************************************************/
    void ProfilerCommandBuffer::PostBeginRendering( const VkRenderingInfo* )
    {
        PostBeginRenderPass( nullptr, VK_SUBPASS_CONTENTS_INLINE );
    }

    /***********************************************************************************\

    Function:
        PreEndRendering

    Description:
        Marks end of current render pass (VK_KHR_dynamic_rendering).

    \***********************************************************************************/
    void ProfilerCommandBuffer::PreEndRendering()
    {
        PreEndRenderPass();
    }

    /***********************************************************************************\

    Function:
        PostEndRendering

    Description:
        Marks end of current render pass (VK_KHR_dynamic_rendering).

    \***********************************************************************************/
    void ProfilerCommandBuffer::PostEndRendering()
    {
        PostEndRenderPass();
    }

    /***********************************************************************************\

    Function:
        NextSubpass

    Description:
        Marks beginning of next render pass subpass.

    \***********************************************************************************/
    void ProfilerCommandBuffer::NextSubpass( VkSubpassContents contents )
    {
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

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
            m_pCurrentSubpassData->m_BeginTimestamp.m_Index =
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
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

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
                m_RayTracingPipeline = pipeline;
                break;
            }
        }
    }

    /***********************************************************************************\

    Function:
        BindShaders

    Description:
        Sets shader objects for the subsequent drawcalls.

    \***********************************************************************************/
    void ProfilerCommandBuffer::BindShaders( uint32_t shaderCount, const VkShaderStageFlagBits* pStages, const VkShaderEXT* pShaders )
    {
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        constexpr VkShaderStageFlags allGraphicsShaderStages =
            VK_SHADER_STAGE_ALL_GRAPHICS |
            VK_SHADER_STAGE_TASK_BIT_EXT |
            VK_SHADER_STAGE_MESH_BIT_EXT;

        constexpr VkShaderStageFlags allComputeShaderStages =
            VK_SHADER_STAGE_COMPUTE_BIT;

        // Check which pipelines have been impacted to recalculate the hashes.
        bool graphicsPipelineChanged = false;
        bool computePipelineChanged = false;

        for( uint32_t i = 0; i < shaderCount; ++i )
        {
            // Update appropriate pipeline state.
            DeviceProfilerPipeline* pPipeline = nullptr;
            DeviceProfilerPipelineType pipelineType = DeviceProfilerPipelineType::eNone;
            VkPipelineBindPoint pipelineBindPoint = {};
            VkShaderStageFlagBits stage = pStages[i];

            if( stage & allGraphicsShaderStages )
            {
                pPipeline = &m_GraphicsPipeline;
                pipelineType = DeviceProfilerPipelineType::eGraphics;
                pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                graphicsPipelineChanged = true;
            }
            else if( stage & allComputeShaderStages )
            {
                pPipeline = &m_ComputePipeline;
                pipelineType = DeviceProfilerPipelineType::eCompute;
                pipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
                computePipelineChanged = true;
            }

            if( pPipeline != nullptr )
            {
                // When shader objects are used, the pipeline bound at the corresponding bind point is unbound.
                if( !pPipeline->m_UsesShaderObjects )
                {
                    pPipeline->m_Handle = VK_NULL_HANDLE;
                    pPipeline->m_BindPoint = pipelineBindPoint;
                    pPipeline->m_Type = pipelineType;
                    pPipeline->m_ShaderTuple.m_Hash = 0;
                    pPipeline->m_ShaderTuple.m_Shaders.clear();
                    pPipeline->m_ShaderTuple.m_ShaderExecutables.clear();
                    pPipeline->m_UsesRayQuery = false;
                    pPipeline->m_UsesRayTracing = false;
                    pPipeline->m_UsesShaderObjects = true;
                }

                // Bind the shader object to the pipeline.
                VkShaderEXT shaderHandle = pShaders ? pShaders[i] : VK_NULL_HANDLE;

                bool stageBound = false;
                for( auto it = pPipeline->m_ShaderTuple.m_Shaders.begin(), end = pPipeline->m_ShaderTuple.m_Shaders.end(); it != end; ++it )
                {
                    if( it->m_Stage == stage )
                    {
                        if( shaderHandle != VK_NULL_HANDLE )
                        {
                            // Override existing shader stage with a new object.
                            *it = m_Profiler.GetShader( shaderHandle );
                            stageBound = true;
                        }
                        else
                        {
                            // Unbind the shader object from the pipeline.
                            pPipeline->m_ShaderTuple.m_Shaders.erase( it );
                        }
                        break;
                    }
                }

                if( !stageBound && (shaderHandle != VK_NULL_HANDLE) )
                {
                    // Bind the shader to a new shader stage.
                    pPipeline->m_ShaderTuple.m_Shaders.push_back( m_Profiler.GetShader( shaderHandle ) );
                }
            }
        }

        // Refresh pipelines after changing the shaders.
        if( graphicsPipelineChanged )
        {
            m_GraphicsPipeline.Finalize();
        }

        if( computePipelineChanged )
        {
            m_ComputePipeline.Finalize();
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
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        if( m_ProfilingEnabled )
        {
            const DeviceProfilerPipelineType pipelineType = drawcall.GetPipelineType();

            // Insert a timestamp in per-render pass mode if command is recorded outside of the render pass
            // and the internal render pass has changed.
            DeviceProfilerRenderPassData* pPreviousRenderPassData = m_pCurrentRenderPassData;
            DeviceProfilerSubpassData* pPreviousSubpassData = m_pCurrentSubpassData;

            // Pointer to the previous pipeline to update the end timestamp if needed.
            DeviceProfilerPipelineData* pPreviousPipelineData = nullptr;

            // Setup pipeline
            switch( pipelineType )
            {
            case DeviceProfilerPipelineType::eNone:
            case DeviceProfilerPipelineType::eDebug:
                SetupCommandBufferForStatCounting( { VK_NULL_HANDLE }, &pPreviousPipelineData );
                break;

            case DeviceProfilerPipelineType::eGraphics:
                SetupCommandBufferForStatCounting( m_GraphicsPipeline, &pPreviousPipelineData );
                break;

            case DeviceProfilerPipelineType::eCompute:
                SetupCommandBufferForStatCounting( m_ComputePipeline, &pPreviousPipelineData );
                break;

            case DeviceProfilerPipelineType::eRayTracingKHR:
                SetupCommandBufferForStatCounting( m_RayTracingPipeline, &pPreviousPipelineData );
                break;

            default: // Internal pipelines
                SetupCommandBufferForStatCounting( m_Profiler.GetPipeline( (VkPipeline)pipelineType ), &pPreviousPipelineData );
                break;
            }

            // Append drawcall to the current pipeline
            m_pCurrentDrawcallData = &m_pCurrentPipelineData->m_Drawcalls.emplace_back( drawcall );

            if( m_Profiler.m_Config.m_CaptureIndirectArguments )
            {
                // Save indirect arguments
                SaveIndirectArgs( *m_pCurrentDrawcallData );
            }

            // Increment drawcall stats
            m_Stats.AddCount( drawcall );

            if( (m_Profiler.m_Config.m_SamplingMode == VK_PROFILER_MODE_PER_DRAWCALL_EXT) ||
                ((m_Profiler.m_Config.m_SamplingMode == VK_PROFILER_MODE_PER_PIPELINE_EXT) &&
                    (m_pCurrentPipelineData != pPreviousPipelineData)) ||
                ((m_Profiler.m_Config.m_SamplingMode == VK_PROFILER_MODE_PER_RENDER_PASS_EXT) &&
                    (pPreviousRenderPassData != m_pCurrentRenderPassData)) )
            {
                // Begin timestamp query
                const uint64_t timestampIndex =
                    m_pQueryPool->WriteTimestamp( m_CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );

                // Update draw begin timestamp index.
                if( m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_DRAWCALL_EXT )
                {
                    m_pCurrentDrawcallData->m_BeginTimestamp.m_Index = timestampIndex;
                }

                // Update pipeline begin timestamp index.
                if( (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_PIPELINE_EXT) &&
                    (m_pCurrentPipelineData->m_BeginTimestamp.m_Index == UINT64_MAX) )
                {
                    m_pCurrentPipelineData->m_BeginTimestamp.m_Index = timestampIndex;

                    // Update end timestamp of the previous pipeline.
                    if( pPreviousPipelineData != nullptr )
                    {
                        if( (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_DRAWCALL_EXT) &&
                            !pPreviousPipelineData->m_Drawcalls.empty() &&
                            (pPreviousPipelineData->m_Drawcalls.back().m_EndTimestamp.m_Index != UINT64_MAX) )
                        {
                            pPreviousPipelineData->m_EndTimestamp =
                                pPreviousPipelineData->m_Drawcalls.back().m_EndTimestamp;
                        }
                        else
                        {
                            pPreviousPipelineData->m_EndTimestamp.m_Index = timestampIndex;
                        }
                    }
                }

                // Update subpass begin timestamp index.
                if( (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_RENDER_PASS_EXT) &&
                    (m_pCurrentSubpassData->m_BeginTimestamp.m_Index == UINT64_MAX) )
                {
                    m_pCurrentSubpassData->m_BeginTimestamp.m_Index = timestampIndex;

                    // Update end timestamp of the previous subpass.
                    if( pPreviousSubpassData != nullptr )
                    {
                        if( (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_PIPELINE_EXT) &&
                            (pPreviousSubpassData->m_Contents == VK_SUBPASS_CONTENTS_INLINE ||
                                pPreviousSubpassData->m_Contents == VK_SUBPASS_CONTENTS_INLINE_AND_SECONDARY_COMMAND_BUFFERS_EXT) &&
                            !pPreviousSubpassData->m_Data.empty() &&
                            (pPreviousSubpassData->m_Data.back().GetType() == DeviceProfilerSubpassDataType::ePipeline) )
                        {
                            auto& pipeline = std::get<DeviceProfilerPipelineData>( pPreviousSubpassData->m_Data.back() );
                            pPreviousSubpassData->m_EndTimestamp = pipeline.m_EndTimestamp;
                        }
                        else
                        {
                            pPreviousSubpassData->m_EndTimestamp.m_Index = timestampIndex;
                        }
                    }
                }

                // Update render pass begin timestamp index.
                if( (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_RENDER_PASS_EXT) &&
                    (m_pCurrentRenderPassData->m_BeginTimestamp.m_Index == UINT64_MAX) )
                {
                    m_pCurrentRenderPassData->m_BeginTimestamp.m_Index = timestampIndex;

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
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        if( m_ProfilingEnabled )
        {
            // End timestamp query
            if( m_Profiler.m_Config.m_SamplingMode == VK_PROFILER_MODE_PER_DRAWCALL_EXT )
            {
                assert( m_pCurrentDrawcallData );

                if (drawcall.GetPipelineType() != DeviceProfilerPipelineType::eDebug)
                {
                    // Send a timestamp query at the end of the command.
                    m_pCurrentDrawcallData->m_EndTimestamp.m_Index =
                        m_pQueryPool->WriteTimestamp(m_CommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
                }
                else
                {
                    // Debug labels have 0 duration, so there is no need for the second query.
                    m_pCurrentDrawcallData->m_EndTimestamp = m_pCurrentDrawcallData->m_BeginTimestamp;
                }

                m_pCurrentDrawcallData = nullptr;
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
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        if( m_ProfilingEnabled )
        {
            // Ensure there is a render pass and subpass with VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS flag
            SetupCommandBufferForSecondaryBuffers();

            auto& currentRenderPass = m_Data.m_RenderPasses.back();
            auto& currentSubpass = currentRenderPass.m_Subpasses.back();

            for( uint32_t i = 0; i < count; ++i )
            {
                currentSubpass.m_Data.push_back( DeviceProfilerCommandBufferData{
                    m_Profiler.ResolveObjectHandle<VkCommandBufferHandle>( pCommandBuffers[i] ),
                    VK_COMMAND_BUFFER_LEVEL_SECONDARY
                    } );

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
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        if( m_ProfilingEnabled )
        {
            // Pipeline barriers can occur only outside of the render pass, increment command buffer stats
            m_Stats.m_PipelineBarrierStats.m_Count +=
                memoryBarrierCount +
                bufferMemoryBarrierCount +
                imageMemoryBarrierCount;

            if( m_Profiler.m_Config.m_CaptureIndirectArguments )
            {
                // Flush any pending indirect argument buffer copies.
                FlushIndirectArgumentCopyLists();
            }
        }
    }

    /***********************************************************************************\

    Function:
        PipelineBarrier

    Description:
        Stores barrier statistics in currently profiled entity.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PipelineBarrier( const VkDependencyInfo* pDependencyInfo )
    {
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        if( m_ProfilingEnabled )
        {
            // Pipeline barriers can occur only outside of the render pass, increment command buffer stats
            m_Stats.m_PipelineBarrierStats.m_Count +=
                pDependencyInfo->memoryBarrierCount +
                pDependencyInfo->bufferMemoryBarrierCount +
                pDependencyInfo->imageMemoryBarrierCount;

            if( m_Profiler.m_Config.m_CaptureIndirectArguments )
            {
                // Flush any pending indirect argument buffer copies.
                FlushIndirectArgumentCopyLists();
            }
        }
    }

    /***********************************************************************************\

    Function:
        GetRequiredQueryDataBufferSize

    Description:
        Return required query data buffer size in bytes.

    \***********************************************************************************/
    uint64_t ProfilerCommandBuffer::GetRequiredQueryDataBufferSize() const
    {
        if( m_ProfilingEnabled )
        {
            return m_pQueryPool->GetRequiredBufferSize();
        }
        return 0;
    }

    /***********************************************************************************\

    Function:
        WriteQueryData

    Description:
        Writes all timestamps to the data buffer.

    \***********************************************************************************/
    void ProfilerCommandBuffer::WriteQueryData( DeviceProfilerQueryDataBufferWriter& writer ) const
    {
        if( m_ProfilingEnabled )
        {
            writer.SetContext( this );
            m_pQueryPool->WriteQueryData( writer );

            for( VkCommandBuffer secondaryCommandBuffer : m_SecondaryCommandBuffers )
            {
                m_Profiler.GetCommandBuffer( secondaryCommandBuffer ).WriteQueryData( writer );
            }
        }
    }

    /***********************************************************************************\

    Function:
        GetData

    Description:
        Reads all queried timestamps.
        Returns structure containing ordered list of timestamps and statistics.

    \***********************************************************************************/
    const DeviceProfilerCommandBufferData& ProfilerCommandBuffer::GetData( DeviceProfilerQueryDataBufferReader& reader )
    {
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        if( m_ProfilingEnabled && !m_Data.m_DataValid )
        {
            reader.SetContext( this );

            // Reset accumulated stats if buffer is being reused
            m_Data.m_Stats = m_Stats;

            // Read global timestamp values
            m_Data.m_BeginTimestamp.m_Value = reader.ReadTimestampQueryResult( m_Data.m_BeginTimestamp.m_Index );

            if( m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_RENDER_PASS_EXT )
            {
                for( auto& renderPass : m_Data.m_RenderPasses )
                {
                    // If this is a secondary command buffer and render pass starts with a nested command buffers,
                    // use the timestamp of the nested command buffer as a begin point of the render pass.
                    bool renderPassStartsWithNestedCommandBuffer = false;

                    if( ((m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_PIPELINE_EXT) ||
                        ((m_Profiler.m_Config.m_SamplingMode == VK_PROFILER_MODE_PER_RENDER_PASS_EXT) &&
                            m_Profiler.m_Config.m_EnableRenderPassBeginEndProfiling)) &&
                        (renderPass.HasBeginCommand()) )
                    {
                        // Get vkCmdBeginRenderPass time
                        renderPass.m_Begin.m_BeginTimestamp.m_Value = reader.ReadTimestampQueryResult( renderPass.m_Begin.m_BeginTimestamp.m_Index );
                        renderPass.m_Begin.m_EndTimestamp.m_Value = reader.ReadTimestampQueryResult( renderPass.m_Begin.m_EndTimestamp.m_Index );

                        // Increment clear time stats
                        uint64_t renderPassBeginDuration = GetDuration( renderPass.m_Begin );

                        if( renderPass.m_ClearsColorAttachments )
                        {
                            m_Data.m_Stats.m_ClearColorStats.AddTicks( renderPassBeginDuration );
                        }
                        if( renderPass.m_ClearsDepthStencilAttachments )
                        {
                            m_Data.m_Stats.m_ClearDepthStencilStats.AddTicks( renderPassBeginDuration );
                        }
                    }

                    const size_t subpassCount = renderPass.m_Subpasses.size();
                    for( size_t subpassIndex = 0; subpassIndex < subpassCount; ++subpassIndex )
                    {
                        auto& subpass = renderPass.m_Subpasses[subpassIndex];
                        const size_t subpassDataCount = subpass.m_Data.size();

                        // Keep track of the first and last subpass data type.
                        // If it is a secondary command buffer, the timestamp queries are allocated from another query pool and their values have already been resolved.
                        bool firstTimestampFromSecondaryCommandBuffer = false;
                        bool lastTimestampFromSecondaryCommandBuffer = false;

                        // Treat data as pipelines if subpass contents are inline-only.
                        if( subpass.m_Contents == VK_SUBPASS_CONTENTS_INLINE )
                        {
                            if( m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_PIPELINE_EXT )
                            {
                                for( size_t subpassDataIndex = 0; subpassDataIndex < subpassDataCount; ++subpassDataIndex )
                                {
                                    ResolveSubpassPipelineData(
                                        reader,
                                        subpass,
                                        subpassDataIndex );
                                }
                            }
                        }

                        // Treat data as secondary command buffers if subpass contents are secondary command buffers only.
                        else if( subpass.m_Contents == VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS )
                        {
                            for( size_t subpassDataIndex = 0; subpassDataIndex < subpassDataCount; ++subpassDataIndex )
                            {
                                ResolveSubpassSecondaryCommandBufferData(
                                    reader,
                                    subpass,
                                    subpassDataIndex,
                                    subpassDataCount,
                                    firstTimestampFromSecondaryCommandBuffer,
                                    lastTimestampFromSecondaryCommandBuffer );
                            }
                        }

                        // With VK_EXT_nested_command_buffer, it is possible to insert both command buffers and inline commands in the same subpass.
                        else if( subpass.m_Contents == VK_SUBPASS_CONTENTS_INLINE_AND_SECONDARY_COMMAND_BUFFERS_EXT )
                        {
                            for( size_t subpassDataIndex = 0; subpassDataIndex < subpassDataCount; ++subpassDataIndex )
                            {
                                auto& data = subpass.m_Data[subpassDataIndex];
                                switch( data.GetType() )
                                {
                                case DeviceProfilerSubpassDataType::ePipeline:
                                {
                                    if( m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_PIPELINE_EXT )
                                    {
                                        ResolveSubpassPipelineData(
                                            reader,
                                            subpass,
                                            subpassDataIndex );
                                    }
                                    break;
                                }
                                case DeviceProfilerSubpassDataType::eCommandBuffer:
                                {
                                    ResolveSubpassSecondaryCommandBufferData(
                                        reader,
                                        subpass,
                                        subpassDataIndex,
                                        subpassDataCount,
                                        firstTimestampFromSecondaryCommandBuffer,
                                        lastTimestampFromSecondaryCommandBuffer );
                                    break;
                                }
                                }
                            }
                        }

                        // Resolve subpass begin and end timestamps if not inherited from the secondary command buffers.
                        if( !firstTimestampFromSecondaryCommandBuffer )
                        {
                            subpass.m_BeginTimestamp.m_Value = reader.ReadTimestampQueryResult( subpass.m_BeginTimestamp.m_Index );
                        }
                        if( !lastTimestampFromSecondaryCommandBuffer )
                        {
                            subpass.m_EndTimestamp.m_Value = reader.ReadTimestampQueryResult( subpass.m_EndTimestamp.m_Index );
                        }

                        // Pass the subpass begin timestamp to render pass.
                        if( subpassIndex == 0 && firstTimestampFromSecondaryCommandBuffer )
                        {
                            renderPass.m_BeginTimestamp = subpass.m_BeginTimestamp;
                            renderPassStartsWithNestedCommandBuffer = true;
                        }
                    }

                    if( ((m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_PIPELINE_EXT) ||
                        ((m_Profiler.m_Config.m_SamplingMode == VK_PROFILER_MODE_PER_RENDER_PASS_EXT) &&
                            m_Profiler.m_Config.m_EnableRenderPassBeginEndProfiling)) &&
                        (renderPass.HasEndCommand()) )
                    {
                        // Get vkCmdEndRenderPass time
                        renderPass.m_End.m_BeginTimestamp.m_Value = reader.ReadTimestampQueryResult( renderPass.m_End.m_BeginTimestamp.m_Index );
                        renderPass.m_End.m_EndTimestamp.m_Value = reader.ReadTimestampQueryResult( renderPass.m_End.m_EndTimestamp.m_Index );

                        // Increment resolve time if resolves were done on render pass end.
                        // TODO: This isn't necessarilly correct as the resolves may happen on end of subpass.
                        if( renderPass.m_ResolvesAttachments )
                        {
                            m_Stats.m_ResolveStats.AddTicks( GetDuration( renderPass.m_End ) );
                        }
                    }

                    // Resolve timestamp queries at the beginning and end of the render pass.
                    if( !renderPassStartsWithNestedCommandBuffer )
                    {
                        renderPass.m_BeginTimestamp.m_Value = reader.ReadTimestampQueryResult( renderPass.m_BeginTimestamp.m_Index );
                    }

                    renderPass.m_EndTimestamp.m_Value = reader.ReadTimestampQueryResult( renderPass.m_EndTimestamp.m_Index );
                }
            }

            m_Data.m_EndTimestamp.m_Value = reader.ReadTimestampQueryResult( m_Data.m_EndTimestamp.m_Index );

            // Read vendor-specific data
            if( reader.HasPerformanceQueryResult() )
            {
                const uint32_t performanceQueryMetricsSetIndex = reader.GetPerformanceQueryMetricsSetIndex();
                const uint32_t performanceQueryResultSize = reader.GetPerformanceQueryResultSize();
                const uint8_t* pPerformanceQueryResult = reader.ReadPerformanceQueryResult();

                assert( m_Profiler.m_pPerformanceCounters != nullptr );
                m_Profiler.m_pPerformanceCounters->ParseReport(
                    performanceQueryMetricsSetIndex,
                    m_CommandPool.GetQueueFamilyIndex(),
                    performanceQueryResultSize,
                    pPerformanceQueryResult,
                    m_Data.m_PerformanceCounters.m_Results );

                m_Data.m_PerformanceCounters.m_MetricsSetIndex = performanceQueryMetricsSetIndex;
            }

            // Copy captured indirect argument buffer data
            m_Data.m_IndirectPayload.clear();

            if( m_Profiler.m_Config.m_CaptureIndirectArguments )
            {
                ReadIndirectArgumentBuffers( m_Data.m_IndirectPayload );
            }

            // Subsequent calls to GetData will return the same results
            m_Data.m_DataValid = true;
        }

        return m_Data;
    }

    /***********************************************************************************\

    Function:
        ResolveSubpassPipelineData

    Description:
        Read timestamps for all drawcalls in the pipeline.

    \***********************************************************************************/
    void ProfilerCommandBuffer::ResolveSubpassPipelineData( const DeviceProfilerQueryDataBufferReader& reader, DeviceProfilerSubpassData& subpass, size_t subpassDataIndex )
    {
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        auto& data = subpass.m_Data[subpassDataIndex];
        assert( data.GetType() == DeviceProfilerSubpassDataType::ePipeline );

        auto& pipeline = std::get<DeviceProfilerPipelineData>( data );
        pipeline.m_BeginTimestamp.m_Value = reader.ReadTimestampQueryResult( pipeline.m_BeginTimestamp.m_Index );
        pipeline.m_EndTimestamp.m_Value = reader.ReadTimestampQueryResult( pipeline.m_EndTimestamp.m_Index );

        if( m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_DRAWCALL_EXT )
        {
            for( auto& drawcall : pipeline.m_Drawcalls )
            {
                // Don't collect data for debug labels
                if( drawcall.GetPipelineType() != DeviceProfilerPipelineType::eDebug )
                {
                    // Update drawcall timestamps
                    drawcall.m_BeginTimestamp.m_Value = reader.ReadTimestampQueryResult( drawcall.m_BeginTimestamp.m_Index );
                    drawcall.m_EndTimestamp.m_Value = reader.ReadTimestampQueryResult( drawcall.m_EndTimestamp.m_Index );

                    // Increment drawcall stats
                    m_Data.m_Stats.AddTicks( drawcall.m_Type, GetDuration( drawcall ) );
                }
                else
                {
                    // Provide timestamps for debug commands
                    drawcall.m_BeginTimestamp.m_Value = reader.ReadTimestampQueryResult( drawcall.m_BeginTimestamp.m_Index );
                    drawcall.m_EndTimestamp.m_Value = drawcall.m_BeginTimestamp.m_Value;
                }
            }
        }
    }

    /***********************************************************************************\

    Function:
        ResolveSubpassSecondaryCommandBufferData

    Description:
        Get data from the secondary command buffer.

    \***********************************************************************************/
    void ProfilerCommandBuffer::ResolveSubpassSecondaryCommandBufferData(
        DeviceProfilerQueryDataBufferReader reader,
        DeviceProfilerSubpassData& subpass,
        size_t subpassDataIndex,
        size_t subpassDataCount,
        bool& firstTimestampFromSecondaryCommandBuffer,
        bool& lastTimestampFromSecondaryCommandBuffer )
    {
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        auto& data = subpass.m_Data[subpassDataIndex];
        assert( data.GetType() == DeviceProfilerSubpassDataType::eCommandBuffer );

        auto& commandBuffer = std::get<DeviceProfilerCommandBufferData>( data );
        VkCommandBuffer handle = commandBuffer.m_Handle;
        ProfilerCommandBuffer& profilerCommandBuffer = *m_Profiler.m_pCommandBuffers.unsafe_at( handle );

        // Collect secondary command buffer data
        commandBuffer = profilerCommandBuffer.GetData( reader );
        assert( commandBuffer.m_Handle == handle );

        // Propagate timestamps from command buffer to subpass
        if( subpassDataIndex == 0 )
        {
            subpass.m_BeginTimestamp = commandBuffer.m_BeginTimestamp;
            firstTimestampFromSecondaryCommandBuffer = true;
        }
        if( subpassDataIndex == subpassDataCount - 1 )
        {
            subpass.m_EndTimestamp = commandBuffer.m_EndTimestamp;
            lastTimestampFromSecondaryCommandBuffer = true;
        }

        // Collect secondary command buffer stats
        m_Data.m_Stats += commandBuffer.m_Stats;
    }

    /***********************************************************************************\

    Function:
        PreBeginRenderPassCommonProlog

    Description:
        Common code executed for both vkCmdBeginRenderPass and vkCmdBeginRendering
        before setting up the new render pass.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PreBeginRenderPassCommonProlog()
    {
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        // End the current render pass, if any.
        if( m_pCurrentRenderPassData != nullptr )
        {
            // Insert a timestamp query at the render pass boundary.
            const uint64_t timestampIndex =
                m_pQueryPool->WriteTimestamp( m_CommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );

            // Update an ending timestamp for the previous render pass.
            m_pCurrentRenderPassData->m_EndTimestamp.m_Index = timestampIndex;
            m_pCurrentSubpassData->m_EndTimestamp.m_Index = timestampIndex;

            // Update pipeline end timestamp index.
            if( (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_PIPELINE_EXT) &&
                (m_pCurrentPipelineData != nullptr) )
            {
                m_pCurrentPipelineData->m_EndTimestamp.m_Index = timestampIndex;
            }

            // Clear renderpass-scoped pointers.
            m_pCurrentRenderPass = nullptr;
            m_pCurrentRenderPassData = nullptr;
            m_pCurrentSubpassData = nullptr;
            m_pCurrentPipelineData = nullptr;
        }
    }

    /***********************************************************************************\

    Function:
        PreBeginRenderPassCommonEpilog

    Description:
        Common code executed for both vkCmdBeginRenderPass and vkCmdBeginRendering
        after setting up the new render pass.

    \***********************************************************************************/
    void ProfilerCommandBuffer::PreBeginRenderPassCommonEpilog()
    {
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        // Ensure there are free queries that can be used in the render pass.
        // The spec forbids resetting the pools inside the render pass scope, so they have to be allocated now.
        m_pQueryPool->PreallocateQueries( m_CommandBuffer );

        m_pCurrentRenderPassData->m_BeginTimestamp.m_Index =
            m_pQueryPool->WriteTimestamp( m_CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );

        // Record initial transitions and clears.
        if( (m_Profiler.m_Config.m_SamplingMode <= VK_PROFILER_MODE_PER_PIPELINE_EXT) ||
            ((m_Profiler.m_Config.m_SamplingMode == VK_PROFILER_MODE_PER_RENDER_PASS_EXT) &&
                m_Profiler.m_Config.m_EnableRenderPassBeginEndProfiling) )
        {
            m_pCurrentRenderPassData->m_Begin.m_BeginTimestamp = m_pCurrentRenderPassData->m_BeginTimestamp;
        }
    }

    /***********************************************************************************\

    Function:
        EndSubpass

    Description:
        Marks end of current render pass subpass.

    \***********************************************************************************/
    void ProfilerCommandBuffer::EndSubpass()
    {
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        // Render pass must be already tracked
        assert( !m_Data.m_RenderPasses.empty() );

        if( m_CurrentSubpassIndex != DeviceProfilerSubpassData::ImplicitSubpassIndex )
        {
            assert( m_pCurrentSubpassData );

            // Check if any attachments are resolved at the end of current subpass.
            // m_pCurrentRenderPass may be null in case of dynamic rendering.
            if( m_pCurrentRenderPass )
            {
                m_Stats.m_ResolveStats.m_Count +=
                    m_pCurrentRenderPass->m_Subpasses[m_CurrentSubpassIndex].m_ResolveCount;
            }

            // Send timestamp query at the end of the subpass.
            if( (m_Profiler.m_Config.m_SamplingMode == VK_PROFILER_MODE_PER_DRAWCALL_EXT) &&
                (m_pCurrentPipelineData) &&
                !(m_pCurrentPipelineData->m_Drawcalls.empty()) &&
                (m_pCurrentPipelineData->m_Drawcalls.back().m_EndTimestamp.m_Index != UINT64_MAX) )
            {
                m_pCurrentSubpassData->m_EndTimestamp =
                    m_pCurrentPipelineData->m_Drawcalls.back().m_EndTimestamp;
            }
            else
            {
                m_pCurrentSubpassData->m_EndTimestamp.m_Index =
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
        SetupCommandBufferForStatCounting

    Description:
        Returns pointer to the previous pipeline data in ppPreviousPipelineData.

        This is because appending to a vector may invalidate it, so caching previous
        state and calling this function may result in writing to a deallocated memory
        previously owned by the vector.

    \***********************************************************************************/
    void ProfilerCommandBuffer::SetupCommandBufferForStatCounting( const DeviceProfilerPipeline& pipeline, DeviceProfilerPipelineData** ppPreviousPipelineData )
    {
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        const DeviceProfilerRenderPassType renderPassType = GetRenderPassTypeFromPipelineType( pipeline.m_Type );

        // Save index of the current pipeline
        size_t previousPipelineIndex = SIZE_MAX;
        DeviceProfilerSubpassData* pPreviousSubpassData = nullptr;

        if( m_pCurrentPipelineData )
        {
            assert( m_pCurrentSubpassData );
            pPreviousSubpassData = m_pCurrentSubpassData;
            previousPipelineIndex = m_pCurrentSubpassData->m_Data.size();

            while( (previousPipelineIndex--) > 0 )
            {
                auto& data = m_pCurrentSubpassData->m_Data[previousPipelineIndex];
                if( (data.GetType() == DeviceProfilerSubpassDataType::ePipeline) &&
                    (&std::get<DeviceProfilerPipelineData>( data ) == m_pCurrentPipelineData) )
                {
                    break;
                }
            }
        }

        // Check if we're in render pass
        if( !m_pCurrentRenderPassData ||
            ((m_pCurrentRenderPassData->m_Handle == VK_NULL_HANDLE) &&
                (m_pCurrentRenderPassData->m_Dynamic == false) &&
                (m_pCurrentRenderPassData->m_Type != renderPassType)) )
        {
            m_pCurrentRenderPassData = &m_Data.m_RenderPasses.emplace_back();
            m_pCurrentRenderPassData->m_Handle = VK_NULL_HANDLE;
            m_pCurrentRenderPassData->m_Type = renderPassType;

            // Invalidate subpass pointer after changing the render pass.
            m_pCurrentSubpassData = nullptr;
        }

        // Check if current subpass allows inline commands
        if( !m_pCurrentSubpassData ||
            ((m_pCurrentSubpassData->m_Contents != VK_SUBPASS_CONTENTS_INLINE) &&
                (m_pCurrentSubpassData->m_Contents != VK_SUBPASS_CONTENTS_INLINE_AND_SECONDARY_COMMAND_BUFFERS_EXT)) )
        {
            m_pCurrentSubpassData = &m_pCurrentRenderPassData->m_Subpasses.emplace_back();
            m_pCurrentSubpassData->m_Index = m_CurrentSubpassIndex;
            m_pCurrentSubpassData->m_Contents = VK_SUBPASS_CONTENTS_INLINE;

            // Invalidate pipeline pointer after chainging the subpass.
            m_pCurrentPipelineData = nullptr;
        }

        // Check if we're in pipeline
        if( !m_pCurrentPipelineData ||
            (m_pCurrentPipelineData->m_Handle != pipeline.m_Handle) ||
            ((m_pCurrentPipelineData->m_Handle == VK_NULL_HANDLE) &&
                (m_pCurrentPipelineData->m_Type != pipeline.m_Type)) ||
            ((m_pCurrentPipelineData->m_UsesShaderObjects || pipeline.m_UsesShaderObjects) &&
                (m_pCurrentPipelineData->m_ShaderTuple.m_Hash != pipeline.m_ShaderTuple.m_Hash)) )
        {
            m_pCurrentPipelineData = &std::get<DeviceProfilerPipelineData>( m_pCurrentSubpassData->m_Data.emplace_back( pipeline ) );
        }

        // Return previous pipeline
        if( pPreviousSubpassData && previousPipelineIndex != SIZE_MAX )
        {
            (*ppPreviousPipelineData) = &std::get<DeviceProfilerPipelineData>( pPreviousSubpassData->m_Data[previousPipelineIndex] );
        }
    }

    /***********************************************************************************\

    Function:
        SetupCommandBufferForSecondaryCommandBuffers

    Description:

    \***********************************************************************************/
    void ProfilerCommandBuffer::SetupCommandBufferForSecondaryBuffers()
    {
        TipGuard tip( m_Profiler.m_pDevice->TIP, __func__ );

        // Check if we're in render pass
        if( !m_pCurrentRenderPassData )
        {
            m_pCurrentRenderPassData = &m_Data.m_RenderPasses.emplace_back();
            m_pCurrentRenderPassData->m_Handle = VK_NULL_HANDLE;
            m_pCurrentRenderPassData->m_Type = DeviceProfilerRenderPassType::eNone;
        }

        // Check if current subpass allows secondary command buffers
        if( !m_pCurrentSubpassData ||
            ((m_pCurrentSubpassData->m_Contents != VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS) &&
                (m_pCurrentSubpassData->m_Contents != VK_SUBPASS_CONTENTS_INLINE_AND_SECONDARY_COMMAND_BUFFERS_EXT)) )
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
        case DeviceProfilerPipelineType::eNone:
        case DeviceProfilerPipelineType::eDebug:
            return DeviceProfilerRenderPassType::eNone;
        case DeviceProfilerPipelineType::eGraphics:
        case DeviceProfilerPipelineType::eClearAttachments:
            return DeviceProfilerRenderPassType::eGraphics;

        case DeviceProfilerPipelineType::eCompute:
            return DeviceProfilerRenderPassType::eCompute;

        case DeviceProfilerPipelineType::eRayTracingKHR:
            return DeviceProfilerRenderPassType::eRayTracing;

        default:
            return DeviceProfilerRenderPassType::eCopy;
        }
    }

    /***********************************************************************************\

    Function:
        SaveIndirectArgs

    Description:
        Copy contents of indirect argument buffer to a temporary buffer.

    \***********************************************************************************/
    void ProfilerCommandBuffer::SaveIndirectArgs( DeviceProfilerDrawcall& drawcall )
    {
        switch( drawcall.m_Type )
        {
        case DeviceProfilerDrawcallType::eDrawIndirect:
        case DeviceProfilerDrawcallType::eDrawIndexedIndirect: {
            DeviceProfilerDrawcallDrawIndirectPayload& payload = drawcall.m_Payload.m_DrawIndirect;
            const size_t indirectPayloadSize = payload.m_DrawCount * payload.m_Stride;

            IndirectArgumentBuffer& buffer = AcquireIndirectArgumentBuffer( indirectPayloadSize );
            payload.m_IndirectArgsOffset = buffer.m_Offset;
            buffer.m_Offset += indirectPayloadSize;

            IndirectArgumentBufferCopy& copy = buffer.m_PendingCopyList.emplace_back();
            copy.m_SrcBuffer = payload.m_Buffer;
            copy.m_DstBuffer = buffer.m_Buffer;
            copy.m_Region.srcOffset = payload.m_Offset;
            copy.m_Region.dstOffset = payload.m_IndirectArgsOffset;
            copy.m_Region.size = indirectPayloadSize;

            break;
        }

        case DeviceProfilerDrawcallType::eDrawIndirectCount:
        case DeviceProfilerDrawcallType::eDrawIndexedIndirectCount: {
            DeviceProfilerDrawcallDrawIndirectCountPayload& payload = drawcall.m_Payload.m_DrawIndirectCount;
            const size_t indirectPayloadSize = sizeof( uint32_t ) + payload.m_MaxDrawCount * payload.m_Stride;

            IndirectArgumentBuffer& buffer = AcquireIndirectArgumentBuffer( indirectPayloadSize );
            payload.m_IndirectCountOffset = buffer.m_Offset;
            payload.m_IndirectArgsOffset = buffer.m_Offset + sizeof( uint32_t );
            buffer.m_Offset += indirectPayloadSize;

            IndirectArgumentBufferCopy& countCopy = buffer.m_PendingCopyList.emplace_back();
            countCopy.m_SrcBuffer = payload.m_CountBuffer;
            countCopy.m_DstBuffer = buffer.m_Buffer;
            countCopy.m_Region.srcOffset = payload.m_CountOffset;
            countCopy.m_Region.dstOffset = payload.m_IndirectCountOffset;
            countCopy.m_Region.size = sizeof( uint32_t );

            IndirectArgumentBufferCopy& argsCopy = buffer.m_PendingCopyList.emplace_back();
            argsCopy.m_SrcBuffer = payload.m_Buffer;
            argsCopy.m_DstBuffer = buffer.m_Buffer;
            argsCopy.m_Region.srcOffset = payload.m_Offset;
            argsCopy.m_Region.dstOffset = payload.m_IndirectArgsOffset;
            argsCopy.m_Region.size = indirectPayloadSize;

            break;
        }

        case DeviceProfilerDrawcallType::eDispatchIndirect: {
            DeviceProfilerDrawcallDispatchIndirectPayload& payload = drawcall.m_Payload.m_DispatchIndirect;
            const size_t indirectPayloadSize = sizeof( VkDispatchIndirectCommand );

            IndirectArgumentBuffer& buffer = AcquireIndirectArgumentBuffer( indirectPayloadSize );
            payload.m_IndirectArgsOffset = buffer.m_Offset;
            buffer.m_Offset += indirectPayloadSize;

            IndirectArgumentBufferCopy& copy = buffer.m_PendingCopyList.emplace_back();
            copy.m_SrcBuffer = payload.m_Buffer;
            copy.m_DstBuffer = buffer.m_Buffer;
            copy.m_Region.srcOffset = payload.m_Offset;
            copy.m_Region.dstOffset = payload.m_IndirectArgsOffset;
            copy.m_Region.size = indirectPayloadSize;

            break;
        }
        }
    }

    /***********************************************************************************\

    Function:
        FlushIndirectArgumentCopyLists

    Description:
        Execute all pending indirect argument buffer copies.

    \***********************************************************************************/
    void ProfilerCommandBuffer::FlushIndirectArgumentCopyLists()
    {
        std::vector<VkBufferCopy> bufferCopyRegions;

        for( IndirectArgumentBuffer& indirectArgumentBuffer : m_IndirectArgumentBufferList )
        {
            while( !indirectArgumentBuffer.m_PendingCopyList.empty() )
            {
                // Batch copies to the same buffer to save CPU cycles in the driver.
                VkBuffer srcBuffer = VK_NULL_HANDLE;

                auto firstCopy = indirectArgumentBuffer.m_PendingCopyList.begin();
                auto lastCopy = indirectArgumentBuffer.m_PendingCopyList.end();
                while( ( srcBuffer == VK_NULL_HANDLE ) && ( firstCopy != lastCopy ) )
                {
                    srcBuffer = ( firstCopy++ )->m_SrcBuffer;
                }

                if( srcBuffer == VK_NULL_HANDLE )
                {
                    // No more buffers to copy in this indirect argument buffer.
                    indirectArgumentBuffer.m_PendingCopyList.clear();
                    continue;
                }

                // Find all regions that copy from the same source buffer.
                for( auto it = firstCopy - 1; it != lastCopy; ++it )
                {
                    if( it->m_SrcBuffer == srcBuffer )
                    {
                        bufferCopyRegions.push_back( it->m_Region );
                        it->m_SrcBuffer = VK_NULL_HANDLE;
                    }
                }

                // Execute the copy.
                m_Profiler.m_pDevice->Callbacks.CmdCopyBuffer(
                    m_CommandBuffer,
                    srcBuffer,
                    indirectArgumentBuffer.m_Buffer,
                    static_cast<uint32_t>( bufferCopyRegions.size() ),
                    bufferCopyRegions.data() );

                bufferCopyRegions.clear();
            }
        }
    }

    /***********************************************************************************\

    Function:
        ReadIndirectArgumentBuffers

    Description:
        Copy captured indirect buffers to the destination buffer.

    \***********************************************************************************/
    void ProfilerCommandBuffer::ReadIndirectArgumentBuffers( std::vector<uint8_t>& dst )
    {
        for( const IndirectArgumentBuffer& indirectArgumentBuffer : m_IndirectArgumentBufferList )
        {
            const uint8_t* pIndirectData = static_cast<const uint8_t*>( indirectArgumentBuffer.m_AllocationInfo.pMappedData );
            const size_t indirectDataSize = indirectArgumentBuffer.m_Offset;

            if( indirectDataSize )
            {
                m_Profiler.m_MemoryManager.Invalidate( indirectArgumentBuffer.m_Allocation );
                dst.insert( dst.end(),
                    pIndirectData,
                    pIndirectData + indirectDataSize );
            }
        }
    }

    /***********************************************************************************\

    Function:
        AcquireIndirectArgumentBuffer

    Description:
        Get a buffer for storing indirect argument data.

    \***********************************************************************************/
    ProfilerCommandBuffer::IndirectArgumentBuffer& ProfilerCommandBuffer::AcquireIndirectArgumentBuffer( size_t size )
    {
        // Check for an existing buffer in the list.
        for( IndirectArgumentBuffer& buffer : m_IndirectArgumentBufferList )
        {
            if( buffer.m_AllocationInfo.size - buffer.m_Offset >= size )
            {
                return buffer;
            }
        }

        // Create a new buffer.
        VkBufferCreateInfo bufferCreateInfo = {};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = std::max<size_t>( size, PROFILER_INDIRECT_ARGS_BUFFER_SIZE );
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo allocationCreateInfo = {};
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

        IndirectArgumentBuffer& buffer = m_IndirectArgumentBufferList.emplace_back();
        m_Profiler.m_MemoryManager.AllocateBuffer(
            bufferCreateInfo,
            allocationCreateInfo,
            &buffer.m_Buffer,
            &buffer.m_Allocation,
            &buffer.m_AllocationInfo );

        buffer.m_Offset = 0;

        return buffer;
    }
}
