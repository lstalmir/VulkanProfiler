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

#include "profiler_trace.h"
#include "profiler_trace_event.h"
#include "profiler/profiler_data.h"

#include "VkLayer_profiler_layer.generated.h"

#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <nlohmann/json.hpp>

// Alias commonly used types
using json = nlohmann::json;

namespace Profiler
{
    /*************************************************************************\

    Function:
        DeviceProfilerTraceSerializer

    Description:
        Constructor.

    \*************************************************************************/
    DeviceProfilerTraceSerializer::DeviceProfilerTraceSerializer( float cpuTimestampPeriod, float gpuTimestampPeriod )
        : m_CPUTimestampPeriod( cpuTimestampPeriod )
        , m_GPUTimestampPeriod( gpuTimestampPeriod )
    {
    }

    /*************************************************************************\

    Function:
        Serialize

    Description:
        Write collected results to the trace file.

    \*************************************************************************/
    void DeviceProfilerTraceSerializer::Serialize( const DeviceProfilerFrameData& data ) const
    {
        json traceJson = {
            { "traceEvents", json::array() },
            { "displayTimeUnit", "ms" },
            { "otherData", json::object() } };

        // Setup array of events
        std::vector<TraceEvent> events;

        for( const auto& submitBatchData : data.m_Submits )
        {
            // Executing queue (thread)
            VkQueue commandQueue = submitBatchData.m_Handle;

            // Queue submission timestamp
            double commandQueueSubmissionTimestamp =
                (submitBatchData.m_Timestamp - data.m_Timestamp) * m_CPUTimestampPeriod;

            // Insert queue submission event
            events.push_back( { "vkQueueSubmit", "API", TraceEventPhase::eInstant,
                commandQueueSubmissionTimestamp,
                commandQueue, VK_NULL_HANDLE } );

            // Get base command buffer offset.
            // Better CPU-GPU correlation could be achieved from ETLs
            uint64_t baseCommandBufferTimestampOffset = -1;

            for( const auto& submitData : submitBatchData.m_Submits )
            {
                for( const auto& commandBufferData : submitData.m_CommandBuffers )
                {
                    if( commandBufferData.m_BeginTimestamp > 0 )
                    {
                        baseCommandBufferTimestampOffset = std::min(
                            baseCommandBufferTimestampOffset, commandBufferData.m_BeginTimestamp );
                    }
                }
            }

            if( baseCommandBufferTimestampOffset == -1 )
            {
                baseCommandBufferTimestampOffset = 0;
            }

            for( const auto& submitData : submitBatchData.m_Submits )
            {
                for( const auto& commandBufferData : submitData.m_CommandBuffers )
                {
                    // Executed command buffer
                    VkCommandBuffer commandBuffer = commandBufferData.m_Handle;

                    if( commandBufferData.m_BeginTimestamp != 0 )
                    {
                        // Insert command buffer begin event
                        events.push_back( { "VkCommandBuffer", "Command Buffers,GPU,Performance", TraceEventPhase::eDurationBegin,
                            (commandQueueSubmissionTimestamp)+((commandBufferData.m_BeginTimestamp - baseCommandBufferTimestampOffset) * m_GPUTimestampPeriod),
                            commandQueue, commandBuffer } );

                        for( const auto& renderPassData : commandBufferData.m_RenderPasses )
                        {
                            if( renderPassData.m_BeginTimestamp != 0 )
                            {
                                if( renderPassData.m_Handle )
                                {
                                    events.push_back( { "VkRenderPass", "Render Passes,GPU,Performance", TraceEventPhase::eDurationBegin,
                                        (commandQueueSubmissionTimestamp)+((renderPassData.m_BeginTimestamp - baseCommandBufferTimestampOffset) * m_GPUTimestampPeriod),
                                        commandQueue, commandBuffer } );
                                    events.push_back( { "vkCmdBeginRenderPass", "Performance", TraceEventPhase::eDurationBegin,
                                        (commandQueueSubmissionTimestamp)+((renderPassData.m_BeginTimestamp - baseCommandBufferTimestampOffset) * m_GPUTimestampPeriod),
                                        commandQueue, commandBuffer } );
                                    events.push_back( { "vkCmdBeginRenderPass", "Performance", TraceEventPhase::eDurationEnd,
                                        (commandQueueSubmissionTimestamp)+((renderPassData.m_CmdBeginEndTimestamp - baseCommandBufferTimestampOffset) * m_GPUTimestampPeriod),
                                        commandQueue, commandBuffer } );
                                }

                                for( const auto& subpassData : renderPassData.m_Subpasses )
                                {
                                    if( subpassData.m_BeginTimestamp != 0 )
                                    {
                                        if( renderPassData.m_Subpasses.size() > 1 )
                                        {
                                            events.push_back( { "Subpass", "Subpasses,GPU,Performance", TraceEventPhase::eDurationBegin,
                                                (commandQueueSubmissionTimestamp)+((subpassData.m_BeginTimestamp - baseCommandBufferTimestampOffset) * m_GPUTimestampPeriod),
                                                commandQueue, commandBuffer } );
                                        }

                                        if( subpassData.m_Contents == VK_SUBPASS_CONTENTS_INLINE )
                                        {
                                            for( const auto& pipelineData : subpassData.m_Pipelines )
                                            {
                                                if( pipelineData.m_BeginTimestamp != 0 )
                                                {
                                                    if( pipelineData.m_Handle && ((pipelineData.m_Hash & 0xFFFF) != 0) )
                                                    {
                                                        events.push_back( { "VkPipeline", "Pipelines,GPU,Performance", TraceEventPhase::eDurationBegin,
                                                            (commandQueueSubmissionTimestamp)+((pipelineData.m_BeginTimestamp - baseCommandBufferTimestampOffset) * m_GPUTimestampPeriod),
                                                            commandQueue, commandBuffer } );
                                                    }

                                                    for( const auto& drawcallData : pipelineData.m_Drawcalls )
                                                    {
                                                        std::stringstream stringBuilder;

                                                        switch( drawcallData.m_Type )
                                                        {

                                                        case DeviceProfilerDrawcallType::eDraw:
                                                        {
                                                            stringBuilder << "vkCmdDraw("
                                                                << drawcallData.m_Payload.m_Draw.m_VertexCount << ", "
                                                                << drawcallData.m_Payload.m_Draw.m_InstanceCount << ", "
                                                                << drawcallData.m_Payload.m_Draw.m_FirstVertex << ", "
                                                                << drawcallData.m_Payload.m_Draw.m_FirstInstance << ")";
                                                            break;
                                                        }

                                                        case DeviceProfilerDrawcallType::eDrawIndexed:
                                                        {
                                                            stringBuilder << "vkCmdDrawIndexed("
                                                                << drawcallData.m_Payload.m_DrawIndexed.m_IndexCount << ", "
                                                                << drawcallData.m_Payload.m_DrawIndexed.m_InstanceCount << ", "
                                                                << drawcallData.m_Payload.m_DrawIndexed.m_FirstIndex << ", "
                                                                << drawcallData.m_Payload.m_DrawIndexed.m_VertexOffset << ", "
                                                                << drawcallData.m_Payload.m_DrawIndexed.m_FirstInstance << ")";
                                                            break;
                                                        }

                                                        case DeviceProfilerDrawcallType::eDrawIndirect:
                                                        {
                                                            stringBuilder << "vkCmdDrawIndirect("
                                                                << drawcallData.m_Payload.m_DrawIndirect.m_Buffer << ", "
                                                                << drawcallData.m_Payload.m_DrawIndirect.m_Offset << ", "
                                                                << drawcallData.m_Payload.m_DrawIndirect.m_DrawCount << ", "
                                                                << drawcallData.m_Payload.m_DrawIndirect.m_Stride << ")";
                                                            break;
                                                        }

                                                        case DeviceProfilerDrawcallType::eDrawIndexedIndirect:
                                                        {
                                                            stringBuilder << "vkCmdDrawIndexedIndirect("
                                                                << drawcallData.m_Payload.m_DrawIndexedIndirect.m_Buffer << ", "
                                                                << drawcallData.m_Payload.m_DrawIndexedIndirect.m_Offset << ", "
                                                                << drawcallData.m_Payload.m_DrawIndexedIndirect.m_DrawCount << ", "
                                                                << drawcallData.m_Payload.m_DrawIndexedIndirect.m_Stride << ")";
                                                            break;
                                                        }

                                                        case DeviceProfilerDrawcallType::eDrawIndirectCount:
                                                        {
                                                            stringBuilder << "vkCmdDrawIndirectCount("
                                                                << drawcallData.m_Payload.m_DrawIndirectCount.m_Buffer << ", "
                                                                << drawcallData.m_Payload.m_DrawIndirectCount.m_Offset << ", "
                                                                << drawcallData.m_Payload.m_DrawIndirectCount.m_CountBuffer << ", "
                                                                << drawcallData.m_Payload.m_DrawIndirectCount.m_CountOffset << ", "
                                                                << drawcallData.m_Payload.m_DrawIndirectCount.m_MaxDrawCount << ", "
                                                                << drawcallData.m_Payload.m_DrawIndirectCount.m_Stride << ")";
                                                            break;
                                                        }

                                                        case DeviceProfilerDrawcallType::eDrawIndexedIndirectCount:
                                                        {
                                                            stringBuilder << "vkCmdDrawIndexedIndirectCount("
                                                                << drawcallData.m_Payload.m_DrawIndexedIndirectCount.m_Buffer << ", "
                                                                << drawcallData.m_Payload.m_DrawIndexedIndirectCount.m_Offset << ", "
                                                                << drawcallData.m_Payload.m_DrawIndexedIndirectCount.m_CountBuffer << ", "
                                                                << drawcallData.m_Payload.m_DrawIndexedIndirectCount.m_CountOffset << ", "
                                                                << drawcallData.m_Payload.m_DrawIndexedIndirectCount.m_MaxDrawCount << ", "
                                                                << drawcallData.m_Payload.m_DrawIndexedIndirectCount.m_Stride << ")";
                                                            break;
                                                        }

                                                        case DeviceProfilerDrawcallType::eDispatch:
                                                        {
                                                            stringBuilder << "vkCmdDispatch("
                                                                << drawcallData.m_Payload.m_Dispatch.m_GroupCountX << ", "
                                                                << drawcallData.m_Payload.m_Dispatch.m_GroupCountY << ", "
                                                                << drawcallData.m_Payload.m_Dispatch.m_GroupCountZ << ")";
                                                            break;
                                                        }

                                                        case DeviceProfilerDrawcallType::eDispatchIndirect:
                                                        {
                                                            stringBuilder << "vkCmdDispatchIndirect("
                                                                << drawcallData.m_Payload.m_DispatchIndirect.m_Buffer << ", "
                                                                << drawcallData.m_Payload.m_DispatchIndirect.m_Offset << ")";
                                                            break;
                                                        }

                                                        case DeviceProfilerDrawcallType::eCopyBuffer:
                                                        {
                                                            stringBuilder << "vkCmdCopyBuffer("
                                                                << drawcallData.m_Payload.m_CopyBuffer.m_SrcBuffer << ", "
                                                                << drawcallData.m_Payload.m_CopyBuffer.m_DstBuffer << ")";
                                                            break;
                                                        }

                                                        case DeviceProfilerDrawcallType::eCopyBufferToImage:
                                                        {
                                                            stringBuilder << "vkCmdCopyBufferToImage("
                                                                << drawcallData.m_Payload.m_CopyBufferToImage.m_SrcBuffer << ", "
                                                                << drawcallData.m_Payload.m_CopyBufferToImage.m_DstImage << ")";
                                                            break;
                                                        }

                                                        case DeviceProfilerDrawcallType::eCopyImage:
                                                        {
                                                            stringBuilder << "vkCmdCopyImage("
                                                                << drawcallData.m_Payload.m_CopyImage.m_SrcImage << ", "
                                                                << drawcallData.m_Payload.m_CopyImage.m_DstImage << ")";
                                                            break;
                                                        }

                                                        case DeviceProfilerDrawcallType::eCopyImageToBuffer:
                                                        {
                                                            stringBuilder << "vkCmdCopyImageToBuffer("
                                                                << drawcallData.m_Payload.m_CopyImageToBuffer.m_SrcImage << ", "
                                                                << drawcallData.m_Payload.m_CopyImageToBuffer.m_DstBuffer << ")";
                                                            break;
                                                        }

                                                        case DeviceProfilerDrawcallType::eClearAttachments:
                                                        {
                                                            stringBuilder << "vkCmdClearAttachments("
                                                                << drawcallData.m_Payload.m_ClearAttachments.m_Count << ")";
                                                            break;
                                                        }

                                                        case DeviceProfilerDrawcallType::eClearColorImage:
                                                        {
                                                            stringBuilder << "vkCmdClearColorImage("
                                                                << drawcallData.m_Payload.m_ClearColorImage.m_Image << ", { "
                                                                << drawcallData.m_Payload.m_ClearColorImage.m_Value.float32[ 0 ] << ", "
                                                                << drawcallData.m_Payload.m_ClearColorImage.m_Value.float32[ 1 ] << ", "
                                                                << drawcallData.m_Payload.m_ClearColorImage.m_Value.float32[ 2 ] << ", "
                                                                << drawcallData.m_Payload.m_ClearColorImage.m_Value.float32[ 3 ] << " })";
                                                            break;
                                                        }

                                                        case DeviceProfilerDrawcallType::eClearDepthStencilImage:
                                                        {
                                                            stringBuilder << "vkCmdClearDepthStencilImage("
                                                                << drawcallData.m_Payload.m_ClearDepthStencilImage.m_Image << ", { "
                                                                << drawcallData.m_Payload.m_ClearDepthStencilImage.m_Value.depth << ", "
                                                                << drawcallData.m_Payload.m_ClearDepthStencilImage.m_Value.stencil << " })";
                                                            break;
                                                        }

                                                        case DeviceProfilerDrawcallType::eResolveImage:
                                                        {
                                                            stringBuilder << "vkCmdResolveImage("
                                                                << drawcallData.m_Payload.m_ResolveImage.m_SrcImage << ", "
                                                                << drawcallData.m_Payload.m_ResolveImage.m_DstImage << ")";
                                                            break;
                                                        }

                                                        case DeviceProfilerDrawcallType::eBlitImage:
                                                        {
                                                            stringBuilder << "vkCmdBlitImage("
                                                                << drawcallData.m_Payload.m_BlitImage.m_SrcImage << ", "
                                                                << drawcallData.m_Payload.m_BlitImage.m_DstImage << ")";
                                                            break;
                                                        }

                                                        case DeviceProfilerDrawcallType::eFillBuffer:
                                                        {
                                                            stringBuilder << "vkCmdFillBuffer("
                                                                << drawcallData.m_Payload.m_FillBuffer.m_Buffer << ", "
                                                                << drawcallData.m_Payload.m_FillBuffer.m_Offset << ", "
                                                                << drawcallData.m_Payload.m_FillBuffer.m_Size << ", "
                                                                << drawcallData.m_Payload.m_FillBuffer.m_Data << ")";
                                                            break;
                                                        }

                                                        case DeviceProfilerDrawcallType::eUpdateBuffer:
                                                        {
                                                            stringBuilder << "vkCmdFillBuffer("
                                                                << drawcallData.m_Payload.m_UpdateBuffer.m_Buffer << ", "
                                                                << drawcallData.m_Payload.m_UpdateBuffer.m_Offset << ", "
                                                                << drawcallData.m_Payload.m_UpdateBuffer.m_Size << ")";
                                                            break;
                                                        }
                                                        }

                                                        events.push_back( { stringBuilder.str(), "Drawcalls,GPU,Performance", TraceEventPhase::eDurationBegin,
                                                            (commandQueueSubmissionTimestamp)+((drawcallData.m_BeginTimestamp - baseCommandBufferTimestampOffset) * m_GPUTimestampPeriod),
                                                            commandQueue, commandBuffer } );
                                                        events.push_back( { stringBuilder.str(), "Drawcalls,GPU,Performance", TraceEventPhase::eDurationEnd,
                                                            (commandQueueSubmissionTimestamp)+((drawcallData.m_EndTimestamp - baseCommandBufferTimestampOffset) * m_GPUTimestampPeriod),
                                                            commandQueue, commandBuffer } );
                                                    }

                                                    if( pipelineData.m_Handle && ((pipelineData.m_Hash & 0xFFFF) != 0) )
                                                    {
                                                        events.push_back( { "VkPipeline", "Pipelines,GPU,Performance", TraceEventPhase::eDurationEnd,
                                                            (commandQueueSubmissionTimestamp)+((pipelineData.m_EndTimestamp - baseCommandBufferTimestampOffset) * m_GPUTimestampPeriod),
                                                            commandQueue, commandBuffer } );
                                                    }
                                                }
                                            }
                                        }

                                        if( renderPassData.m_Subpasses.size() > 1 )
                                        {
                                            events.push_back( { "Subpass", "Subpasses,GPU,Performance", TraceEventPhase::eDurationEnd,
                                                (commandQueueSubmissionTimestamp)+((subpassData.m_EndTimestamp - baseCommandBufferTimestampOffset) * m_GPUTimestampPeriod),
                                                commandQueue, commandBuffer } );
                                        }
                                    }
                                }

                                if( renderPassData.m_Handle )
                                {
                                    events.push_back( { "vkCmdEndRenderPass", "Performance", TraceEventPhase::eDurationBegin,
                                        (commandQueueSubmissionTimestamp)+((renderPassData.m_CmdEndBeginTimestamp - baseCommandBufferTimestampOffset) * m_GPUTimestampPeriod),
                                        commandQueue, commandBuffer } );
                                    events.push_back( { "vkCmdEndRenderPass", "Performance", TraceEventPhase::eDurationEnd,
                                        (commandQueueSubmissionTimestamp)+((renderPassData.m_EndTimestamp - baseCommandBufferTimestampOffset) * m_GPUTimestampPeriod),
                                        commandQueue, commandBuffer } );
                                    events.push_back( { "VkRenderPass", "Render Passes,GPU,Performance", TraceEventPhase::eDurationEnd,
                                        (commandQueueSubmissionTimestamp)+((renderPassData.m_EndTimestamp - baseCommandBufferTimestampOffset) * m_GPUTimestampPeriod),
                                        commandQueue, commandBuffer } );
                                }
                            }
                        }

                        // Insert command buffer end event
                        events.push_back( { "VkCommandBuffer", "Command Buffers,GPU,Performance", TraceEventPhase::eDurationEnd,
                            (commandQueueSubmissionTimestamp)+((commandBufferData.m_EndTimestamp - baseCommandBufferTimestampOffset) * m_GPUTimestampPeriod),
                            commandQueue, commandBuffer } );
                    }
                }
            }
        }

        traceJson[ "traceEvents" ] = events;

        // Write JSON to file
        std::ofstream( ConstructTraceFileName() ) << traceJson;
    }

    /*************************************************************************\

    Function:
        ConstructTraceFileName

    \*************************************************************************/
    std::filesystem::path DeviceProfilerTraceSerializer::ConstructTraceFileName() const
    {
        using namespace std::chrono;

        // Get current time and date
        const auto currentTimePoint = system_clock::now();
        const auto currentTimePointInTimeT = system_clock::to_time_t( currentTimePoint );

        const auto* tm = std::localtime( &currentTimePointInTimeT );

        // Get milliseconds
        const auto ms = time_point_cast<milliseconds>(currentTimePoint).time_since_epoch() % 1000;

        // Construct output file name
        std::stringstream stringBuilder;
        stringBuilder << "ProcessName_PID_";
        stringBuilder << std::put_time( tm, "%Y-%m-%d_%H-%M-%S" ) << "_" << ms.count();
        stringBuilder << ".json";

        return stringBuilder.str();
    }
}
